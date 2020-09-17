import sys
import time
import random
import numpy as np
from array import array
from termcolor import colored
from pexpect import *
from xmodem import XMODEM
import serial
import os
import zlib
import socket
from rumboot.edclManager import edclmanager
from rumboot.chipDb import ChipDb
from rumboot.ImageFormatDb import ImageFormatDb
from rumboot.resetSeq import ResetSeqFactory
from rumboot.cmdline import arghelper
from rumboot.terminal import terminal
from rumboot.xfer import *
from shutil import copyfile
import argparse
import rumboot_xrun
import rumboot
from parse import *
import atexit
import threading
import time
import subprocess
import ctypes
import inspect
import time
import io

MDL_1888TX018 = 0
MDL_1888BM018 = 1

model = MDL_1888TX018

MODE_XMODEM = 0
MODE_EDCL = 1

write_mode = MODE_EDCL
read_mode = MODE_XMODEM

splpath = "./../../../../spl/u-boot-spl-dtb.rbi"

if model == MDL_1888TX018:
    chip = "mm7705"
    rstmsg = "\x00boot: Waiting for a valid image @ 0x40000"
    rstdev = "/dev/ttyACM1"
    ttydev = "/dev/ttyUSB6"
    baudrate = 1000000
    splbase = 0x40000

    # Name m25p32, size 0x400000, page size 0x100, erase size 0x10000
    sf_dev0 = "sf00"
    sf_addr0 = 0x200000
    sf_size0 = 0x10000

    #Nand: chipsize=0x010000000,writesize=0x800,erasesize=0x20000
    nand_dev0 = "nand0"
    nand_addr0 = 0x100000
    nand_size0 = 0x20000

    # Name mmc0@0x3C064000,block length read 0x200,block length write 0x200,erase size(x512 byte) 0x1
    mmc_dev0 = "mmc0"
    mmc_addr0 = 0x500000
    mmc_size0 = 0x4000

    #  CFI conformant size: 010000000 erasesize: 00040000 writesize: 00000001\r\n'
    nor_dev0 = "nor0"
    nor_addr0 = 0x0
    nor_size0 = 0x40000

    #  CFI conformant size: 010000000 erasesize: 00040000 writesize: 00000001\r\n'
    nor_dev1 = "nor1"
    nor_addr1 = 0x500000
    nor_size1 = 0x40000

elif model == MDL_1888BM018:
    chip = "oi10"
    rstmsg = "boot: host: Hit 'X' for X-Modem upload"
    rstcmd = '"printf "22:05" |nc 192.168.10.239 6722'
    ttydev = "/dev/ttyUSB7"
    baudrate = 115200
    splbase = 0x80020000

    """ mb150-02.dts: для работы пары sf00 и sf10 вытаскиваем mmc:
    mmc0: mmc0@D002C000 {
        status = "disabled";
    mmc1: mmc1@D003C000 {
        status = "disabled";
    spi0: spi@D002B000 {
        // status = "disabled";
    spi1: spi@D003B000 {
        // status = "disabled"; """
    sf_dev0 = "sf00"
    sf_addr0 = 0x200000
    sf_size0 = 0x10000

    sf_dev1 = "sf10"
    sf_addr1 = 0x200000
    sf_size1 = 0x10000

    mmc_dev0 = "mmc0"
    mmc_addr0 = 0x100000
    mmc_size0 = 0x8000

    mmc_dev1 = "mmc1"
    mmc_addr1 = 0x200000
    mmc_size1 = 0x8000

wr_file = "tmpwr"
rd_file = "tmprd"

edcl_buf_size = 4096
ser = None

do_randfile = 1
do_wr = 1
do_rd = 1
do_cmp = 1

buf_ptr = [0, 0]
sync_ptr = 0
buf_len = 0

def intialize_programmer(target_chip=None, signal_sender=None):
    resets  = ResetSeqFactory("rumboot.resetseq")
    formats = ImageFormatDb("rumboot.images")
    chips   = ChipDb("rumboot.chips")
    c       = chips[target_chip] 

    parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter,
                                     description="rumboot-factorytest {} - RumBoot Board Testing Framework\n".format(rumboot.__version__) +
                                    rumboot.__copyright__)
    helper = arghelper()

    helper.add_terminal_opts(parser)
    helper.add_resetseq_options(parser, resets)

    opts = parser.parse_args()

    dump_path = os.path.dirname(__file__) + "/romdumps/"

    helper.detect_terminal_options(opts, c)

    print("Detected chip:    %s (%s)" % (c.name, c.part))
    if c == None:
        print("ERROR: Failed to auto-detect chip type")
        return 1
    if opts.baud == None:
        opts.baud = [ c.baudrate ]

    reset = resets[opts.reset[0]](opts)
    term = terminal(opts.port[0], opts.baud[0])
    term.set_chip(c)

    try:
        romdump = open(dump_path + c.romdump, "r")
        term.add_dumps({'rom' : romdump})
    except:
        pass

    if opts.log:
        term.logstream = opts.log

    print("Reset method:               %s" % (reset.name))
    print("Baudrate:                   %d bps" % int(opts.baud[0]))
    print("Port:                       %s" % opts.port[0])
    if opts.edcl and c.edcl != None:
        term.xfer.selectTransport("edcl")
    print("Preferred data transport:   %s" % term.xfer.how)
    print("--- --- --- --- --- --- ")

    return term, reset

def send(cmd):
    global term
    #print(cmd)
    cmd += "\n"
    term.ser.write(cmd.encode('utf-8'))

def expect(msg):
    global ser
    msg += "\r\n"
    while True:
        r = term.ser.readline()
        if r:
            print(r)
            if r == msg.encode('utf-8'):
                return

def select(dev): # список устройств и выбор нужного,выбор буфера можно не делать
    send("list")
    expect("completed")
    send("select %s" % dev)
    expect("Device %s selected" % dev)
    send("bufsel0")
    expect("selected")

def get_buf_ptr():
    global buf_ptr, sync_ptr, buf_len
    send("bufptr") #"buffers 0x56474 0x55474 sync 0x55464 length 0x1000"
    while True:
        r = term.ser.readline()
        if r:
            print(r)
            if r[:7] == "buffers".encode('utf-8'):
                break
    s = r.decode('utf-8').split(sep=" ")
    print(s)
    buf_ptr[0] = int(s[1], 16)
    buf_ptr[1] = int(s[2], 16)
    sync_ptr = int(s[4], 16)
    buf_len =  int(s[6], 16)
    #print("%x,%x,%x,%x\n", buf_ptr[0], buf_ptr[1], sync_ptr, buf_len)

def read_sync(xfer):
    global sync_ptr
    buffer = io.BytesIO()
    xfer.recv(buffer, sync_ptr, 4)
    buffer.truncate(4)
    return int.from_bytes(buffer.getvalue(), 'big')

def wait_eq_sync(xfer, val, tout=5):
    for i in range(tout*10):
        if read_sync(xfer) == val:
            return
        time.sleep(0.1)
    print("wait_eq_sync timeout\n")

def wait_neq_sync(xfer, val, tout=5):
    for i in range(tout*10):
        r = read_sync(xfer)
        if r != val:
            return r
        time.sleep(0.1)
    print("wait_neq_sync timeout\n")
    return r

def write_sync(xfer, val):
    global sync_ptr
    xfer.send(io.BytesIO(val.to_bytes(4, 'big')), sync_ptr)

def testx(flash_dev, flash_addr, flash_size):
    global chip
    global rstmsg
    global splbase
    global ser
    global term
    global buf_ptr, sync_ptr, buf_len
    err = 0

    xfer_xmodem = None
    xfer_edcl = None

    term, reset = intialize_programmer(chip)
    reset.resetToHost()
    expect(rstmsg)
    term.xfer.reconnect()
    term.xfer.connect(term.chip)
    stream = open(splpath, "rb")
    if model == MDL_1888BM018:  # для TX018 в этом нет необходимости,почему?
        send("X")
    term.xfer.send(stream, splbase)
    stream.close()
    expect("Flashwriter(1.0.0) running(help for information):")
    select(flash_dev)

# генерим случайный файл
    if os.path.exists(rd_file):
        os.remove(rd_file)
    if do_randfile:
        if os.path.exists(wr_file):
            os.remove(wr_file)
        randgen = spawn("dd if=/dev/urandom bs=1 of=%s count=%u" % (wr_file, flash_size), encoding='utf-8')
        randgen.logfile_read = sys.stdout
        randgen.expect(["%x+0 records out" % flash_size, EOF, TIMEOUT], timeout=5)
# записываем
    if do_wr:
        if write_mode == MODE_XMODEM:
            send("program %c %x %x" % ('X', flash_addr, flash_size))
            expect("completed")
            #time.sleep(0.5)
            wr_stream = open(wr_file, "rb")
            if xfer_xmodem == None:
                xfer_xmodem = xferXmodem(term)
            xfer_xmodem.connect(term.chip)
            xfer_xmodem.send(wr_stream, 0)
            wr_stream.close();
        elif write_mode == MODE_EDCL:
            get_buf_ptr()
            send("program %c %x %x" % ('E', flash_addr, flash_size))
            expect("completed")
            if xfer_edcl == None:
                xfer_edcl = xferEdcl(term)
            wr_stream = open(wr_file, "rb")
            n = 0
            xfer_edcl.connect(term.chip)
            xfer_edcl.connect(term.chip)
            while True:
                wr_buf = wr_stream.read(edcl_buf_size)
                if not wr_buf:
                    break
                xfer_edcl.send(io.BytesIO(wr_buf), buf_ptr[n & 1]) # set address of current buffer
                write_sync(xfer_edcl, buf_ptr[n & 1])
                wait_eq_sync(xfer_edcl, 0)
                n = n+1
                print(colored("send buffer %x: %x,%x,%x,%x" % (n, wr_buf[0], wr_buf[1], wr_buf[2], wr_buf[3]), 'yellow'))
            wr_stream.close();
        print(colored('%s: write finished' % flash_dev, 'green'))
# сделать обязательно...
    term.ser.reset_input_buffer()
# читаем в файл
    if do_rd:
        if read_mode == MODE_XMODEM:
            send("duplicate %c %x %x" % ('X', flash_addr, flash_size))
            expect("ready")
            rd_stream = open(rd_file, "wb")
            if xfer_xmodem == None:
                xfer_xmodem = xferXmodem(term)
            xfer_xmodem.connect(term.chip)
            xfer_xmodem.recv(rd_stream, 0, flash_size)
            rd_stream.close()
        elif read_mode == MODE_EDCL:
            get_buf_ptr()
            send("duplicate %c %x %x" % ('E', flash_addr, flash_size))
            expect("ready")
            rd_stream = open(rd_file, "wb")
            if xfer_edcl == None:
                xfer_edcl = xferEdcl(term)
            xfer_edcl.connect(term.chip)
            xfer_edcl.connect(term.chip)
            for n in range(flash_size//edcl_buf_size):
                while True:
                    buf_addr = wait_neq_sync(xfer_edcl, 0) # get address of current buffer
                    if n & 1:
                        if buf_addr == buf_ptr[1]:
                            break
                    else:
                        if buf_addr == buf_ptr[0]:
                            break
                pos = rd_stream.tell()
                xfer_edcl.recv(rd_stream, buf_addr, edcl_buf_size)
                off = pos + edcl_buf_size - rd_stream.tell()
                rd_stream.seek(off, os.SEEK_CUR)
                #write_sync(xfer_edcl, 0xffffffff if (n & 1) else 0)
                write_sync(xfer_edcl, 0)
                print(colored("recv buffer %x" % n, 'yellow'))
            rd_stream.truncate(flash_size)
            rd_stream.close()

        print(colored('%s: read finished' % flash_dev, 'green'))
# сравниваем
    if do_cmp:
        crc32_wr = zlib.crc32(open(wr_file, 'rb').read(), 0)
        crc32_rd = zlib.crc32(open(rd_file, 'rb').read(), 0)
        err = 1 if (crc32_wr != crc32_rd) else 0
        print(colored('%s: compare finished (%x-%x)' % (flash_dev, crc32_wr, crc32_rd) + ('err' if err else 'ok'), 'red' if err else 'green'))
    return err

def mmc0_to_mmc1_copy(addr, size): #    # slice mmc0 -> mmc1 and check mmc1
    global do_randfile, do_wr, do_rd, do_cmp
    err = 0
    do_randfile = 0
    do_wr = 0; do_rd = 1; do_cmp = 0
    err += testx(mmc_dev0, addr, size)
    copyfile(rd_file, wr_file)
    do_wr = 1; do_rd = 1; do_cmp = 1
    err += testx(mmc_dev1, addr, size)
    return err

sum_err = 0

if model == MDL_1888TX018:

    sum_err += testx(sf_dev0, sf_addr0, sf_size0)

   # sum_err += testx(nand_dev0, nand_addr0, nand_size0)

    sum_err += testx(mmc_dev0, mmc_addr0, mmc_size0)

    sum_err += testx(nor_dev0, nor_addr0, nor_size0)

    sum_err += testx(nor_dev1, nor_addr1, nor_size1)

elif model == MDL_1888BM018:

    #sum_err += testx(sf_dev0, sf_addr0, sf_size0)

    #sum_err += testx(sf_dev1, sf_addr1, sf_size1)

    sum_err += testx(mmc_dev0, mmc_addr0, mmc_size0)

    sum_err += testx(mmc_dev1, mmc_addr1, mmc_size1)

    #sum_err += mmc0_to_mmc1_copy(0, 0x400000)

print(colored("Completed,error %u" % sum_err, 'yellow'))

while True:
    pass
