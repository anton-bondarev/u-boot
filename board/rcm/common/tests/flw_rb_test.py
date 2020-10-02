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
import getch

MDL_1888TX018 = 0
MDL_1888BM018 = 1
MDL_1888BC048 = 2
MDL_1879VM8YA = 3

MODE_XMODEM = 0
MODE_EDCL = 1
                            # host                  target
CONFIRM_HANDSHAKE = 0       # data -> sync
                            #                       ~sync -> conf
                            # wait for conf
                            # write 0 to conf(2x)
                            #                       wait to conf

write_mode = MODE_EDCL
read_mode = MODE_EDCL

splpath = "./../../../../spl/u-boot-spl-dtb.rbi"

def setup_model_param(model):
    global chip, rstmsg, rstdev, baudrate, splbase
    global sf_dev0, sf_addr0, sf_size0, sf_dev1, sf_addr1, sf_size1
    global nand_dev0, nand_addr0, nand_size0
    global mmc_dev0, mmc_addr0, mmc_size0, mmc_dev1, mmc_addr1, mmc_size1
    global nor_dev0, nor_addr0, nor_size0, nor_dev1, nor_addr1, nor_size1
    global i2c_dev0, i2c_addr0, i2c_size0

    if model == MDL_1888TX018:
        chip = "mm7705"
        rstmsg = "\x00boot: Waiting for a valid image @ 0x40000"
        rstdev = "/dev/ttyACM1"
        baudrate = 1000000
        splbase = 0x40000

        sf_dev0 = "sf00"        # Name m25p32, size 0x400000, page size 0x100, erase size 0x10000
        sf_addr0 = 0x300000
        sf_size0 = 0x10000

        nand_dev0 = "nand0"     # Nand: chipsize=0x010000000,writesize=0x800,erasesize=0x20000
        nand_addr0 = 0x100000
        nand_size0 = 0x20000
        """ Part  Start Sector    Num Sectors     UUID          Type      Addr
            1     2048            8388608         0d632b5d-01     83      0x100000
            2     8390656         8388608         0d632b5d-02     83      0x100100000
            3     16779264        8388608         0d632b5d-03     83      0x200100000
            4     25167872        5948416         0d632b5d-04     83      0x300100000
        """
        mmc_dev0 = "mmc0"       # Name mmc0@0x3C064000,block length read 0x200,block length write 0x200,erase size(x512 byte) 0x1
        mmc_addr0 = 0x300100000
        mmc_size0 = 0x5ad000

        nor_dev0 = "nor0"       # CFI conformant size: 010000000 erasesize: 00040000 writesize: 00000001\r\n'
        nor_addr0 = 0x0
        nor_size0 = 0x40000

        nor_dev1 = "nor1"       # CFI conformant size: 010000000 erasesize: 00040000 writesize: 00000001\r\n'
        nor_addr1 = 0x500000
        nor_size1 = 0x40000
        return True
    elif model == MDL_1888BM018:
        chip = "oi10"
        rstmsg = "boot: host: Hit 'X' for X-Modem upload"
        rstcmd = '"printf "22:05" |nc 192.168.10.239 6722'
        baudrate = 115200
        splbase = 0x80020000 # it's wrong?

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
        return True
    elif model == MDL_1888BC048:
        chip = "basis"
        rstmsg = "boot: host: Hit 'X' for xmodem upload\n"
        rstcmd = '"printf "22:05" |nc 192.168.10.239 6722'
        baudrate = 6000000
        splbase = 0x00040000

        sf_dev0 = "sf00"
        sf_addr0 = 0x0
        sf_size0 = 0x4000

        sf_dev1 = "sf10"
        sf_addr1 = 0x200000
        sf_size1 = 0x10000

        mmc_dev0 = "mmc0"
        mmc_addr0 = 0x100000
        mmc_size0 = 0x8000

        mmc_dev1 = "mmc1"
        mmc_addr1 = 0x200000
        mmc_size1 = 0x800000

        i2c_dev0 = "i2c0"
        i2c_addr0 = 0
        i2c_size0 = 0x2000

        return True
    return False

wr_file = "tmpwr"
rd_file = "tmprd"

edcl_buf_size = 4096
ser = None

do_term = 0
do_rand_file = 1
do_55aa_file = 0
do_wr = 1
do_rd = 1
do_cmp = 1

buf_ptr = [0, 0]
sync_ptr = 0
conf_ptr = 0
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

def expect(msg, crlf=True):
    global ser
    if crlf:
        msg += "\r\n"
    while True:
        r = term.ser.readline()
        if r:
            print(r)
            if r == msg.encode('utf-8'):
                return

def select(dev):
    send("list")
    expect("completed")
    send("select %s" % dev)
    expect("Device %s selected" % dev)
    send("bufsel0")
    expect("selected")

def get_buf_ptr():
    global buf_ptr, sync_ptr, conf_ptr, buf_len
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
    buf_len = int(s[6], 16)
    if CONFIRM_HANDSHAKE:
        conf_ptr =  int(s[8], 16)
    print("%x,%x,%x,%x,%x\n" % (buf_ptr[0], buf_ptr[1], sync_ptr, conf_ptr, buf_len))

def read_sync_conf(xfer, ptr):
    buffer = io.BytesIO()
    xfer.recv(buffer, ptr, 4)
    buffer.truncate(4)
    return int.from_bytes(buffer.getvalue(), 'big')

def wait_eq_sync(xfer, val, tout=5):
    global sync_ptr
    for i in range(tout*10):
        r = read_sync_conf(xfer, sync_ptr)
        if r == val:
            return True
        time.sleep(0.1)
    print("wait_eq_sync timeout(%x,%x)\n" % (val,r))
    return False

def wait_neq_sync(xfer, val, tout=5):
    global sync_ptr
    for i in range(tout*10):
        r = read_sync_conf(xfer, sync_ptr)
        if r != val:
            return r
        time.sleep(0.1)
    print("wait_neq_sync timeout(%x,%x)\n" % (val,r))
    return r

def write_sync(xfer, val, tout=0.5):
    global sync_ptr
    global conf_ptr
    xfer.send(io.BytesIO(val.to_bytes(4, 'big')), sync_ptr)
    if CONFIRM_HANDSHAKE == False:
        return True
    for i in range(int(tout*10)):
        rd_val = read_sync_conf(xfer, conf_ptr)
        if rd_val == (val ^ 0xffffffff):
            val = 0
            xfer.send(io.BytesIO(val.to_bytes(4, 'big')), conf_ptr)
            xfer.send(io.BytesIO(val.to_bytes(4, 'big')), conf_ptr)
            return True
        time.sleep(0.1)
    print("write_sync timeout(%x,%x)\n" % (val,rd_val))
    return False

def usr_input():
    while 1:
        data = input()+"\r\n"
        #print(data.encode('utf-8'))
        term.ser.write(data.encode('utf-8'))
        time.sleep(5)

def usr_term():
    myThread = threading.Thread(target=usr_input)
    myThread.start()
    while 1:
        data = term.ser.readline()
        if data != b'':
            print(data.decode('utf-8'))

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
    expect(rstmsg, False if model == MDL_1888BC048 else True)
    term.xfer.reconnect()
    term.xfer.connect(term.chip)
    stream = open(splpath, "rb")
    if model == MDL_1888BM018 or model == MDL_1888BC048:
        send("X")
    term.xfer.send(stream, splbase)
    stream.close()
    expect("Flashwriter(1.0.0) running(help for information):")

    if do_term:
        usr_term()

    select(flash_dev)
    if os.path.exists(rd_file):
        os.remove(rd_file)

    if do_rand_file:
        if os.path.exists(wr_file):
            os.remove(wr_file)
        randgen = spawn("dd if=/dev/urandom bs=1 of=%s count=%u" % (wr_file, flash_size), encoding='utf-8')
        randgen.logfile_read = sys.stdout
        randgen.expect(["%x+0 records out" % flash_size, EOF, TIMEOUT], timeout=5)

    if do_55aa_file:
        data = [0x55, 0xaa]
        if os.path.exists(wr_file):
            os.remove(wr_file)
        stream = open(wr_file, "wb")
        for i in range(flash_size):
            stream.write(data[i&1].to_bytes(1, byteorder='big'))
        stream.close()

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
                while True:
                    xfer_edcl.send(io.BytesIO(wr_buf), buf_ptr[n & 1])          # set address of current buffer
                    if write_sync(xfer_edcl, buf_ptr[n & 1]) == True:           # if sync been writen
                        break
                    xfer_edcl.reconnect()                                       # else reconnect and resend
                wait_eq_sync(xfer_edcl, 0)
                n = n+1
                print(colored("send buffer %x: %x,%x,%x,%x" % (n, wr_buf[0], wr_buf[1], wr_buf[2], wr_buf[3]), 'yellow'))
            wr_stream.close();
        print(colored('%s: write finished' % flash_dev, 'green'))

    term.ser.reset_input_buffer()

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
test_num = 100

sel_model = {
    "redd":         MDL_1888TX018,
    "nov-nil01":    MDL_1888BM018,
    "nov-nil03":    MDL_1888BC048
}

model = sel_model[socket.gethostname()]

if setup_model_param(model) == False:
    printf("Error: model mismatch\n")
    exit

for iter in range(test_num):
    if model == MDL_1888TX018:

        sum_err += testx(sf_dev0, sf_addr0, sf_size0)

        sum_err += testx(nand_dev0, nand_addr0, nand_size0)

        sum_err += testx(mmc_dev0, mmc_addr0, mmc_size0)

    # sum_err += testx(nor_dev0, nor_addr0, nor_size0)

    # sum_err += testx(nor_dev1, nor_addr1, nor_size1)

    elif model == MDL_1888BM018:

        #sum_err += testx(sf_dev0, sf_addr0, sf_size0)

        #sum_err += testx(sf_dev1, sf_addr1, sf_size1)

        sum_err += testx(mmc_dev0, mmc_addr0, mmc_size0)

        #sum_err += testx(mmc_dev1, mmc_addr1, mmc_size1)

        #sum_err += mmc0_to_mmc1_copy(0, 0x400000)

    elif model == MDL_1888BC048:

        sum_err += testx(sf_dev0, sf_addr0, sf_size0)

        sum_err += testx(i2c_dev0, i2c_addr0, i2c_size0)

        sum_err += testx(mmc_dev1, mmc_addr1, mmc_size1)

    print(colored("Iteration %u completed,error %u" % (iter+1, sum_err), 'yellow'))

while True:
    pass
