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

MDL_1888TX018 = 0
MDL_1888BM018 = 1
MDL_1888BC048 = 2
MDL_1879VM8YA = 3

model = MDL_1879VM8YA

rstdev = "/dev/ttyACM0"
rstchr = ['C', 'c']
ttydev = "/dev/ttyUSB0"
ttybr = 115200
wr_file = "tmpwr"
rd_file = "tmprd"

edcl_buf_size = 4096
ser = None

# w25q128fw size: 001000000 erasesize: 00001000 writesize: 00000100\r\n'
sf_dev = "sf00"
sf_addr = 0x0
sf_size = 0x2000

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
do_cksum = 1

def write_data_file(name, size):
    b = bytearray()
    s = 0
    for i in range(size-2):
        r = random.randint(0,255)
        s += r
        b.append(r)
    b.append((s>>8)&0xff)
    b.append((s>>0)&0xff)
    stream = open(name, "wb")
    stream.write(b)
    stream.close()
    #print("%x,%x,%x" % (s, b[size-2], b[size-1]))

def check_data_file(name, size):
    s = 0
    i = 0
    stream = open(name, "rb")
    b = stream.read(size)
    for i in range(size-2):
        s += b[i]
    #print("%x,%x,%x" % (s, b[size-2], b[size-1]))
    return ((s>>8)&0xff) == b[size-2] and ((s>>0)&0xff) == b[size-1]

def ser_init():
    global ttydev, ttybr
    return serial.Serial( port=ttydev, baudrate=ttybr, timeout=3, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, bytesize=serial.EIGHTBITS)

def send(cmd):
    global ser
    print(cmd)
    cmd += "\n"
    if model != MDL_1879VM8YA:
        ser.write(cmd.encode('utf-8'))
    else:
        for i in range(len(cmd)):
            ser.write(cmd[i].encode('utf-8'))
            time.sleep(0.02)

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
    d = ser.read(size) or None
    #print(colored(d if d != None else "None", 'green'))
    return d

putc_cnt = 0;
def putc(data, timeout=30):
    global ser
    global putc_cnt
    l = len(data)
    for i in range(len(data)):
        d = data[i:i+1]
        #print(colored(d, 'yellow'))
        ser.write(d)
        #time.sleep(0.01)
    putc_cnt += l
    if putc_cnt > 0x20000:
        putc_cnt = 0
        print('.', end = '\n')
    return 

def rst_cmd(chr, dev):
    os.system('echo %c > %s\n' % (chr, dev))

def reset(): # сброс (реле)
    global rstchr, rstdev
    rst_cmd(rstchr[0], rstdev)
    time.sleep(1)
    rst_cmd(rstchr[1], rstdev)
    ##expect("\x00boot: Waiting for a valid image @ 0x40000")
    time.sleep(3)

def load(): # загружаем spl
    if model == MDL_1888TX018:
        os.system('edcltool -i enp2s0 -f ./loadspl.lua')
    elif model == MDL_1879VM8YA:
        sudoPassword = '1'
        command = ['./edclload write  -m 0x0000000 -f ./../../../../spl/u-boot-spl-dtb.bin -i config_lin -v',
                './edclload run  -i config_lin -v']
        p = os.system('echo %s|sudo -S %s' % (sudoPassword, command[0]))
        p = os.system('echo %s|sudo -S %s' % (sudoPassword, command[1]))
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
        write_data_file(wr_file, flash_size)
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
# проверка данных считанного файла
    if do_cksum:
        ok = check_data_file(rd_file, flash_size)
        print(colored('%s: checksum checked-%s' % (flash_dev, 'ok' if ok else 'error'), 'green'))
# сравнение файлов
    if do_cmp:
        crc32_wr = zlib.crc32(open(wr_file, 'rb').read(), 0)
        crc32_rd = zlib.crc32(open(rd_file, 'rb').read(), 0)
        ok = crc32_wr == crc32_rd
        print(colored('%s: compare finished (%x-%x)' % (flash_dev, crc32_wr, crc32_rd) + ('ok' if ok else 'error'), 'green'))

    return ok

random.seed()

sf_addr = 0
sf_size = 0x4000
err = 0
for i in range(0x1000000//sf_size):
    if testx(sf_dev, sf_addr, sf_size) == False:
        err += 1
    sf_addr += sf_size
    print(colored("Adddress %x, errors %u\n" % (sf_addr, err), 'yellow'))

#testx(nand_dev, nand_addr, nand_size)
#testx(mmc_dev, mmc_addr, mmc_size)

#testx_complex(1, 1, 1) # шить dtb, kernel, rootfs

#nor: todo

while True:
    pass
