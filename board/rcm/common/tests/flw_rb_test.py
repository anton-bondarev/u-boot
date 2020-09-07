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
from rumboot.edclManager import edclmanager
from rumboot.chipDb import ChipDb
from rumboot.ImageFormatDb import ImageFormatDb
from rumboot.resetSeq import ResetSeqFactory
from rumboot.cmdline import arghelper
from rumboot.terminal import terminal
from rumboot.xfer import *
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

MODE_XMODEM = 0
MODE_EDCL = 1

mode = MODE_XMODEM

rstdev = "/dev/ttyACM0"
ttydev = "/dev/ttyUSB6"
baudrate = 1000000
wr_file = "tmpwr"
rd_file = "tmprd"

edcl_buf_size = 4096
ser = None

# Name m25p32, size 0x400000, page size 0x100, erase size 0x10000
sf_dev = "sf00"
sf_addr = 0x200000
sf_size = 0x20000

#Nand: chipsize=0x010000000,writesize=0x800,erasesize=0x20000
nand_dev = "nand0"
nand_addr = 0x100000
nand_size = 0x60000

# Name mmc0@0x3C064000,block length read 0x200,block length write 0x200,erase size(x512 byte) 0x1
mmc_dev = "mmc0"
mmc_addr = 0
mmc_size = 4096

do_randfile = 1
do_wr = 1
do_restart = 1
do_rd = 1
do_cmp = 1

buf0_ptr = 0
buf1_ptr = 0
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
    # PP: TODO: change __init__ for terminal and call set_progress_widgets with 2 new arguments
    # usage example
    #       term.desc_widget(value, minimum, maximum)
    #       term.desc_widget(value)
    #       term.prog_widget(text)
    # if signal_sender:
    #     term = terminal(opts.port[0],
    #                     opts.baud[0],
    #                     signal_sender.update_progress_bar_terminal,
    #                     signal_sender.set_value_terminal_label)
    # else:
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

def get_buf():
    global buf0_ptr, buf1_ptr, sync_ptr, buf_len
    send("bufptr") #"buffers 0x56474 0x55474 sync 0x55464 length 0x1000"
    while True:
        r = term.ser.readline()
        if r:
            print(r)
            if r[:7] == "buffers".encode('utf-8'):
                break
    s = r.decode('utf-8').split(sep=" ")
    print(s)
    buf0_ptr = int(s[1], 16)
    buf1_ptr = int(s[2], 16)
    sync_ptr = int(s[4], 16)
    buf_len =  int(s[6], 16)
    #print("%x,%x,%x,%x\n", buf0_ptr, buf1_ptr, sync_ptr, buf_len)

def testx(flash_dev, flash_addr, flash_size):
    global ser
    global term

    term, reset = intialize_programmer("mm7705")
    reset.resetToHost()
    expect("\x00boot: Waiting for a valid image @ 0x40000")
    term.xfer.reconnect()
    term.xfer.connect(term.chip)
    stream = open("/home/vs/work/u-boot/spl/u-boot-spl-dtb.rbi", "rb")
    term.xfer.send(stream, 0x00040000)
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
# адреса буферов
        get_buf()
# записываем
    if do_wr:
        send("program %c %x %x" % ('X', flash_addr, flash_size))
        expect("completed")
        if mode == MODE_XMODEM:
            time.sleep(0.5)
            xfer = xferXmodem(term)
            wr_stream = open(wr_file, "rb")
            xfer.connect(term.chip)
            xfer.send(wr_stream, 0)
            wr_stream.close();
        else:
            pass
        print(colored('%s: write finished' % flash_dev, 'green'))
# читаем в файл
    if do_rd:
        send("duplicate %c %x %x" % ('X', flash_addr, flash_size))
        expect("ready")
        if mode == MODE_XMODEM:
            rd_stream = open(rd_file, "wb")
            xfer.recv(rd_stream, 0, flash_size)
            rd_stream.close()
        else:
            pass
        print(colored('%s: read finished' % flash_dev, 'green'))
# сравниваем
    if do_cmp:
        crc32_wr = zlib.crc32(open(wr_file, 'rb').read(), 0)
        crc32_rd = zlib.crc32(open(rd_file, 'rb').read(), 0)
        ok = crc32_wr == crc32_rd
        print(colored('%s: compare finished (%x-%x)' % (flash_dev, crc32_wr, crc32_rd) + ('ok' if ok else 'error'), 'green'))
    return ok


testx(sf_dev, sf_addr, sf_size)

#testx(nand_dev, nand_addr, nand_size)

#testx(mmc_dev, mmc_addr, mmc_size)

while True:
    pass
