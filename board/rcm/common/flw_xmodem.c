/*
 * Adapted from avr-liberty project
 * (c) Pascal Stang - Copyright (C) 2006
 * This code is distributed under the GNU Public License
 * which can be found at http://www.gnu.org/licenses/gpl.txt
 */

#include <string.h>
#include "flw_serial.h"
#include "flw_xmodem.h"


#define XMODEM_BUFFER_SIZE		1024

uint16_t crc_xmodem_update(uint16_t crc, uint8_t data)
{
	int i;

	crc = crc ^ ((uint16_t)data << 8);
	for (i=0; i<8; i++)
	{
		if(crc & 0x8000)
			crc = (crc << 1) ^ 0x1021;
		else
			crc <<= 1;
	}

	return crc;
}

int xmodem_chksum(int crcflag, const unsigned char *buffer, int size, unsigned short* calc_crc)
{
	if(crcflag)
	{
		unsigned short crc=0;
		unsigned short pktcrc = (buffer[size]<<8)+buffer[size+1];
		while(size--)
			crc = crc_xmodem_update(crc, *buffer++);
        if (calc_crc)
            *calc_crc = crc;
		if(crc == pktcrc)
			return 1;
	}
	else
	{
		int i;
		unsigned char cksum = 0;
		for(i=0; i<size; ++i)
		{
			cksum += buffer[i];
		}
		if(cksum == buffer[size])
			return 1;
	}

	return 0;
}

void xmodem_flush_input(void)
{
	while(flw_getc(XMODEM_TIMEOUT_DELAY / 2) >= 0);
}

static size_t memcpy_cb(size_t curpos, void *data, size_t length, void *arg)
{
	char *to = arg;
	memcpy(&to[curpos], data, length);
	// для калбека прошивальщика тут будем писать во флеш, если буфер заполнен и обнулим текущую позицию в буфере
	return curpos + length; // вернем новую позицию в 4К буфере, в прошивальщике может быть 0,если буфер полностью заполнен, записан во флэш и требуется повторное его заполнение от начала
}


size_t xmodem_get_async(size_t maxszs, size_t (*recv_cb)(size_t curpos, void *ptr, size_t length, void *arg), void *arg)
{
	unsigned char xmbuf[XMODEM_BUFFER_SIZE+6];
	unsigned char seqnum=1;
	unsigned short pktsize=128;
	unsigned char response='C';
	char retry=XMODEM_RETRY_LIMIT;
	unsigned char crcflag=0;
	unsigned long totalbytes=0;
	int i,c;
	size_t pos = 0;

	while(retry > 0)
	{
		if (response)
			flw_putc(response);
		if( (c = flw_getc(XMODEM_TIMEOUT_DELAY)) >= 0)
		{
			switch(c)
			{
			case SOH:
				pktsize = 128;
				break;
			#if(XMODEM_BUFFER_SIZE>=1024)
			case STX:
				pktsize = 1024;
				break;
			#endif
			case EOT:
				xmodem_flush_input();
				flw_putc(ACK);
				return totalbytes;
			case CAN:
				if((c = flw_getc(XMODEM_TIMEOUT_DELAY)) == CAN)
				{
					xmodem_flush_input();
					flw_putc(ACK);
					return XMODEM_ERROR_REMOTECANCEL;
				}
			default:
				break;
			}
		}
		else
		{
			retry--;
			if (response != 'C')
				response = 0;
			continue;
		}

		if(response == 'C') crcflag = 1;
		xmbuf[0] = c;
		for(i=0; i<(pktsize+crcflag+4-1); i++)
		{
			if((c = flw_getc(XMODEM_TIMEOUT_DELAY)) >= 0)
			{
				xmbuf[1+i] = c;
			}
			else
			{
				retry--;
				xmodem_flush_input();
				response = NAK;
				break;
			}
		}
		if(i<(pktsize+crcflag+4-1))
			continue;

		if(	(xmbuf[1] == (unsigned char)(~xmbuf[2])) &&		// sequence number was transmitted w/o error
			xmodem_chksum(crcflag, &xmbuf[3], pktsize, NULL) )	// packet is not corrupt
		{
			if(xmbuf[1] == seqnum)
			{
				// write/deliver data
				pos = recv_cb(pos, &xmbuf[3], pktsize, arg);
				totalbytes += pktsize;
				seqnum++;
				retry = XMODEM_RETRY_LIMIT;
				response = ACK;
				continue;
			}
			else if(xmbuf[1] == (unsigned char)(seqnum-1))
			{
				response = ACK;
				continue;
			}
			else
			{
				xmodem_flush_input();
				flw_putc(CAN);
				flw_putc(CAN);
				flw_putc(CAN);
				return XMODEM_ERROR_OUTOFSYNC;
			}
		}
		else
		{
			retry--;
			xmodem_flush_input();
			response = NAK;
			continue;
		}
	}

	xmodem_flush_input();
	flw_putc(CAN);
	flw_putc(CAN);
	flw_putc(CAN);
	return XMODEM_ERROR_RETRYEXCEED;
}

size_t xmodem_get( char *to, size_t maxszs)
{
	return xmodem_get_async(maxszs, memcpy_cb, to);
}

static size_t transmit_cb(size_t curpos, void *ptr, size_t length, void *arg)
{
	char *src = arg;
// тут в калбеке прошивальщика прочитаем из флеша в буфер, если ранее считанный передали или еще не читали
	memcpy(ptr, &src[curpos], length);
	return curpos + length; // в прошивальщике может быть 0, если считанный из флэш буфер передан и требуется его считать снова
}

int xmodem_send(void *ptr, size_t length) {
	int ret = xmodem_tx_async(transmit_cb, length, ptr);
	return ret;
}

int xmodem_tx_async(size_t (*transmit_cb)(size_t curpos, void *ptr, size_t length, void *arg), int srcsz, void *arg)
{
	unsigned char xbuff[1030]; /* 1024 for XModem 1k + 3 head chars + 2 crc + nul */
	int bufsz, crc = -1;
	unsigned char packetno = 1;
	int i, c, len = 0;
	int retry;
	uint32_t curpos = 0;

	for(;;) {
		for( retry = 0; retry < 16; ++retry) {
			if ((c = flw_getc((XMODEM_TIMEOUT_DELAY)<<1)) >= 0) {
				switch (c) {
				case 'C':
					crc = 1;
					goto start_trans;
				case NAK:
					crc = 0;
					goto start_trans;
				case CAN:
					if ((c = flw_getc(XMODEM_TIMEOUT_DELAY)) == CAN) {
						flw_putc(ACK);
						xmodem_flush_input();
						return -1; /* canceled by remote */
					}
					break;
				default:
					break;
				}
			}
		}
		flw_putc(CAN);
		flw_putc(CAN);
		flw_putc(CAN);
		xmodem_flush_input();
		return -2; /* no sync */

		for(;;) {
		start_trans:
			xbuff[0] = SOH; bufsz = 128;
			xbuff[1] = packetno;
			xbuff[2] = ~packetno;
			c = srcsz - len;
			if (c > bufsz) c = bufsz;
			if (c > 0) { // >=
				memset (&xbuff[3], 0, bufsz);
				if (c == 0) {
					xbuff[3] = CTRLZ;
				}
				else {
					curpos = transmit_cb(curpos, &xbuff[3], c, arg);
					if (c < bufsz) xbuff[3+c] = CTRLZ; // fill free area?
				}
				if (crc) {
					unsigned short ccrc = 0xFFFF;
                    xmodem_chksum(1, &xbuff[3], bufsz, &ccrc);
					xbuff[bufsz+3] = (ccrc>>8) & 0xFF;
					xbuff[bufsz+4] = ccrc & 0xFF;
				}
				else {
					unsigned char ccks = 0;
					for (i = 3; i < bufsz+3; ++i) {
						ccks += xbuff[i];
					}
					xbuff[bufsz+3] = ccks;
				}
				for (retry = 0; retry < XMODEM_RETRY_LIMIT; ++retry) {
					for (i = 0; i < bufsz+4+(crc?1:0); ++i) {
						flw_putc(xbuff[i]);
					}
					if ((c = flw_getc(XMODEM_TIMEOUT_DELAY)) >= 0 ) {
						switch (c) {
						case ACK:
							++packetno;
							len += bufsz;
							goto start_trans;
						case CAN:
							if ((c = flw_getc(XMODEM_TIMEOUT_DELAY)) == CAN) {
								flw_putc(ACK);
								xmodem_flush_input();
								return -1; /* canceled by remote */
							}
							break;
						case NAK:
						default:
							break;
						}
					}
				}
				flw_putc(CAN);
				flw_putc(CAN);
				flw_putc(CAN);
				xmodem_flush_input();
				return XMODEM_ERROR_XMIT; /* xmit error */
			}
			else {
				for (retry = 0; retry < 10; ++retry) {
					flw_putc(EOT);
					if ((c = flw_getc((XMODEM_TIMEOUT_DELAY)<<1)) == ACK) break;
				}
				xmodem_flush_input();
				return (c == ACK)?len:XMODEM_ERROR_ACT;
			}
		}
	}
}