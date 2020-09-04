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

rstdev = "/dev/ttyACM0"
ttydev = "/dev/ttyUSB6"
baudrate = 1000000
wr_file = "tmpwr"
rd_file = "tmprd"

edcl_buf_size = 4096
ser = None

# Name m25p32, size 0x400000, page size 0x100, erase size 0x10000
sf_dev = "sf00"
sf_addr = 0x20000
sf_size = 0x10000

#Nand: chipsize=0x010000000,writesize=0x800,erasesize=0x20000
nand_dev = "nand0"
nand_addr = 0x000000
nand_size = 0x20000

# Name mmc0@0x3C064000,block length read 0x200,block length write 0x200,erase size(x512 byte) 0x1
mmc_dev = "mmc0"
mmc_addr = 0x0
mmc_size = 0x60000

do_randfile = 1
do_wr = 1
do_restart = 1
do_rd = 1
do_cmp = 1

def ser_init():
    global ttydev
    return serial.Serial( port=ttydev, baudrate=1000000, timeout=30, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, bytesize=serial.EIGHTBITS)

def send(cmd):
    global ser
    print(cmd)
    cmd += "\n"
    ser.write(cmd.encode('utf-8'))

def expect(msg):
    global ser
    msg += "\r\n"
    while True:
        r = ser.readline()
        if r:
            print(r)
            if r == msg.encode('utf-8'):
                return

getc_cnt = 0;
def getc(size, timeout=30):
    global ser
    global getc_cnt
    getc_cnt += size
    if getc_cnt > 0x20000:
        getc_cnt = 0
        print('.', end = '\n')
    return ser.read(size) or None

putc_cnt = 0;
def putc(data, timeout=30):
    global ser
    global putc_cnt
    l = len(data)
    for i in range(len(data)):
        ser.write(data[i:i+1])
    putc_cnt += l
    if putc_cnt > 0x20000:
        putc_cnt = 0
        print('.', end = '\n')
    return l

def reset(): # сброс (реле)
    os.system('echo b > %s' % rstdev)
    time.sleep(1)
    os.system('echo B > %s' % rstdev)
    expect("\x00boot: Waiting for a valid image @ 0x40000")
    time.sleep(1)

def load(): # загружаем spl
    os.system('edcltool -i enp2s0 -f ./loadspl.lua')
    expect("Flashwriter(1.0.0) running(help for information):")

def select(dev): # список устройств и выбор нужного,выбор буфера можно не делать
    send("list")
    expect("completed")
    send("select %s" % dev)
    expect("Device %s selected" % dev)
    send("bufsel0")
    expect("selected")

def testx_complex(dtb, kern, fs):
# зашивает в NAND dtb, ядро и файловую систему (в загружаемом затем uboot включить dts mb115-03)
# ниже последовательность команд в uboot для загрузки, используя зашитое в NAND:
# setenv mtdids nand0=mtd0
# setenv mtdparts mtdparts=mtd0:0x720000(rootfs),0x20000(dtb),0x540000(kern)
# setenv bootargs console=ttyAMA0 rootfstype=ubifs ubi.mtd=0 root=ubi0:root_volume rootwait
# run setmem
# nand read 50000000 740000 540000
# nand read 50f00000 720000 20000
# bootm 50000000 - 50f00000
    global wr_file
    global do_randfile,do_wr, do_restart, do_rd, do_cmp
    do_randfile = 0
    do_wr = 1
    do_restart = 0
    do_rd = 1
    do_cmp = 1
# пишем файловую систему
    addr = 0
    size = 0x720000
    wr_file = "rootfs"
    if fs:
        testx(nand_dev, addr, size)
# пишем dtb
    addr += size
    size = 0x20000
    wr_file = "tx011.dtb"
    if dtb:
        testx(nand_dev, addr, size)
# пишем образ ядра
    addr += size
    size = 0x540000
    wr_file = "tx011.uImage"
    if kern:
        testx(nand_dev, addr, size)

def testx(flash_dev, flash_addr, flash_size):
    global ser
    ser = ser_init()
    reset()
    load()
    select(flash_dev)
# генерим случайный файл
    if do_randfile:
        randgen = spawn("dd if=/dev/urandom bs=1 of=%s count=%u" % (wr_file, flash_size), encoding='utf-8')
        randgen.logfile_read = sys.stdout
        randgen.expect(["%x+0 records out" % flash_size, EOF, TIMEOUT], timeout=5)
# записываем
    if do_wr:
        send("program %c %x %x" % ('X', flash_addr, flash_size))
        expect("completed")

        time.sleep(0.1)
        modem = XMODEM(getc, putc)
        stream = open(wr_file, "rb")
        modem.send(stream)
        stream.close()
        print(colored('%s: write finished' % flash_dev, 'green'))
# выключим-включим
    if do_restart:
        ser.close()
        ser = ser_init()
        reset()
        load()
        select(flash_dev)
# читаем в файл
    if do_rd:
        send("duplicate %c %x %x" % ('X', flash_addr, flash_size))
        expect("ready")

        ser = ser_init()
        modem = XMODEM(getc, putc)
        stream = open(rd_file, "w+b")
        modem.recv(stream, timeout=30)
        stream.close()
        print(colored('%s: read finished' % flash_dev, 'green'))

    ser.close()
# сравниваем
    if do_cmp:
        crc32_wr = zlib.crc32(open(wr_file, 'rb').read(), 0)
        crc32_rd = zlib.crc32(open(rd_file, 'rb').read(), 0)
        ok = crc32_wr == crc32_rd
        print(colored('%s: compare finished (%x-%x)' % (flash_dev, crc32_wr, crc32_rd) + ('ok' if ok else 'error'), 'green'))

    return ok

#testx(sf_dev, sf_addr, sf_size)
#testx(nand_dev, nand_addr, nand_size)
#testx(mmc_dev, mmc_addr, mmc_size)

testx_complex(1, 1, 1) # шить dtb, kernel, rootfs

#nor: todo

while True:
    pass
