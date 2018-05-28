//#define DEBUG
//#define DEBUG_BUF
//#define DEBUG_REG

#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <part.h>
#include <mmc.h>
#include <crc.h>
#include <linux/crc7.h>
#include <asm/byteorder.h>
#include <clk.h>
#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <linux/libfdt.h>
#include <malloc.h>
#include <asm/io.h>
#include <asm/gpio.h>

#include "mpw7705_sysreg.h"

#define SDIO_TIMEOUT        2000000 

#ifndef DEBUG 
	// ASTRO TODO: find a real place where we need this delay...
	#undef  debug
	#define debug(...)  delay_loop(100)
#endif	

DECLARE_GLOBAL_DATA_PTR;

struct mpw7705_mmc_platdata {
	struct mmc_config cfg;
	struct mmc mmc;
	uint32_t regbase;
	struct gpio_desc card_detect;
};

static inline uint32_t get_regbase(const struct mmc * mmc)
{
	return ((struct mpw7705_mmc_platdata *) mmc->priv)->regbase;
}

static uint32_t wait_tick(void) 
{ 
	static uint32_t tick = 0;
	return tick ++; 
}

static void delay_loop(uint count)
{
	volatile uint cnt = count;
	while ( cnt -- )
		;
}

#define SDR_TRAN_FIFO_FINISH    0x00000001
#define SDR_TRAN_SDC_DAT_DONE   0x00000002
#define SDR_TRAN_SDC_CMD_DONE   0x00000004
#define SDR_TRAN_DAT_BOUNT      0x00000008
#define SDR_TRAN_SDC_ERR        0x00000010

#define DSSR_CHANNEL_TR_DONE    0x00080000

#define BLOCK_512_DATA_TRANS        0x00000101
#define BUFER_TRANS_START           0x04
#define DCCR_1_VAL                  0x00020F01

//-------------------------------------

#ifdef DEBUG_REG
static void mpw7705_print_op(char * func, uint32_t adr, int reg, uint val)
{
	static int s_reg = -1;
	static uint s_val;
	static bool flag;
	
	if ( reg != s_reg || val != s_val ) {
		debug("%s: adr=0x%x, reg = 0x%x, val = 0x%x\n", func, adr, reg, val);
		s_reg = reg;
		s_val = val;
		flag = false;
	} else if ( ! flag ) {
		debug("+++\n");
		flag = true;		
	}
}
#else
	#define mpw7705_print_op(func, adr, reg, val)
#endif

static inline void mpw7705_writel(const struct mmc * mmc, int reg, u32 val)
{
	uint32_t regbase = get_regbase(mmc);
	mpw7705_print_op("mpw7705_writeL", regbase, reg, val);
	writel(val, regbase + reg);
}
static inline void mpw7705_writew(const struct mmc * mmc, int reg, u16 val)
{
	uint32_t regbase = get_regbase(mmc);
	mpw7705_print_op("mpw7705_writeW", regbase, reg, val);
	writew(val, regbase + reg);
}
static inline void mpw7705_writeb(const struct mmc * mmc, int reg, u8 val)
{
	uint32_t regbase = get_regbase(mmc);
	mpw7705_print_op("mpw7705_writeB", regbase, reg, val);
	writeb(val, regbase + reg);
}

static inline u32 mpw7705_readl(const struct mmc * mmc, int reg)
{
	uint32_t regbase = get_regbase(mmc);
	u32 retval = readl(regbase + reg);
	mpw7705_print_op("mpw7705_readL", regbase, reg, retval);
	return retval;
}
static inline u16 mpw7705_sdhci_readw(const struct mmc * mmc, int reg)
{
	uint32_t regbase = get_regbase(mmc);
	u16 retval = readw(regbase + reg);
	mpw7705_print_op("mpw7705_readW", regbase, reg, retval);
	return retval;
}
static inline u8 mpw7705_readb(const struct mmc * mmc, int reg)
{
	uint32_t regbase = get_regbase(mmc);
	u8 retval = readb(regbase + reg);
	mpw7705_print_op("mpw7705_readB", regbase, reg, retval);
	return retval;
}

//-------------------------------------

static bool wait_cmd_done_handle(const struct mmc * mmc) 
{
	uint32_t start = wait_tick();
	do {
		int status = mpw7705_readl(mmc, SPISDIO_SDIO_INT_STATUS);
		if ( status & SPISDIO_SDIO_INT_STATUS_CAR_ERR ) {
			mpw7705_writel(mmc, SDIO_SDR_BUF_TRAN_RESP_REG, SDR_TRAN_SDC_ERR);
			debug(" ECMD<0x%x>", status);
			return false;
		} else if ( status & SPISDIO_SDIO_INT_STATUS_CMD_DONE ) {
			mpw7705_writel(mmc, SDIO_SDR_BUF_TRAN_RESP_REG, SDR_TRAN_SDC_CMD_DONE);
			return true;
		}
	} while ( wait_tick() - start < SDIO_TIMEOUT );

	debug("CMD ERROR: TIMEOUT\n");
	return false;
}
 
static bool wait_tran_done_handle(const struct mmc * mmc)
{
	uint32_t start = wait_tick();
	do {
		int status = mpw7705_readl(mmc, SPISDIO_SDIO_INT_STATUS);
		if ( status & SPISDIO_SDIO_INT_STATUS_CAR_ERR ) {
			mpw7705_writel(mmc, SDIO_SDR_BUF_TRAN_RESP_REG, SDR_TRAN_SDC_ERR);
			debug(" ETRN<0x%x>", status);
			return false;
		} else if ( status & SPISDIO_SDIO_INT_STATUS_TRAN_DONE ) {
			mpw7705_writel(mmc, SDIO_SDR_BUF_TRAN_RESP_REG, SDR_TRAN_SDC_DAT_DONE);
			return true;
		}
	} while ( wait_tick() - start < SDIO_TIMEOUT );

	debug("TRN ERROR: TIMEOUT\n");
	return false;
}

static bool wait_ch1_dma_done_handle(const struct mmc * mmc)
{
	uint32_t start = wait_tick();
	do {
		int status = mpw7705_readl(mmc, SPISDIO_SDIO_INT_STATUS);
		if ( status & SPISDIO_SDIO_INT_STATUS_CAR_ERR ) {
			mpw7705_writel(mmc, SDIO_SDR_BUF_TRAN_RESP_REG, SDR_TRAN_SDC_ERR);
			debug(" EDMA<0x%x>", status);
			return false;
		} else if ( status & SPISDIO_SDIO_INT_STATUS_CH1_FINISH ) {
			mpw7705_writel(mmc, SDIO_DCCR_1, DSSR_CHANNEL_TR_DONE);
			return true;
		}
	} while ( wait_tick() - start < SDIO_TIMEOUT );

	debug("DMA ERROR: TIMEOUT\n");
	return false;
}

static bool wait_buf_tran_finish_handle(const struct mmc * mmc)
{
	uint32_t start = wait_tick();
	do {
		int status = mpw7705_readl(mmc, SPISDIO_SDIO_INT_STATUS);
		if ( status & SPISDIO_SDIO_INT_STATUS_CAR_ERR ) {
			mpw7705_writel(mmc, SDIO_SDR_BUF_TRAN_RESP_REG, SDR_TRAN_SDC_ERR);
			debug(" EBLK<0x%x>", status);
			return false;
		} else if ( status & SPISDIO_SDIO_INT_STATUS_BUF_FINISH ) {
			mpw7705_writel(mmc, SDIO_SDR_BUF_TRAN_RESP_REG, SDR_TRAN_FIFO_FINISH);
			return true;
		}
	} while( wait_tick() - start < SDIO_TIMEOUT );

	debug("BLK ERROR: TIMEOUT\n");
	return false;
}
//-------------------------------------

#define SDIO_RESPONSE_NONE  0
#define SDIO_RESPONSE_R2    1
#define SDIO_RESPONSE_R1367 2
#define SDIO_RESPONSE_R1b   3

#define SDIO_CTRL_NODATA(_cmd, _resp, _crc, _idx)          \
   ( _cmd << 16 | (_idx << 12) | (_crc << 13)  | (_resp << 10) | (0x11))

static bool sd_send_cmd(const struct mmc * mmc, uint32_t cmd_ctrl, uint32_t arg)
{
	debug("SEND CMD_%d(%x)[%x]", cmd_ctrl >> 16, cmd_ctrl & 0xFFFF, arg);
	mpw7705_writel(mmc, SDIO_SDR_CMD_ARGUMENT_REG, arg);
	mpw7705_writel(mmc, SDIO_SDR_CTRL_REG, cmd_ctrl);
	bool result = wait_cmd_done_handle(mmc);
	debug(" res=%s", (result ? "ok" : "err"));
	delay_loop(20);
	return result;
}
////////////////////////////////////////////////////////////////

#define CMD17_CTRL  0x00117911

static bool SD2buf(const struct mmc * mmc, int buf_num, int idx, uint len)
{
	mpw7705_writel(mmc, SDIO_SDR_ADDRESS_REG, (buf_num << 2));
	mpw7705_writel(mmc, SDIO_SDR_CARD_BLOCK_SET_REG, (len == 512 ? BLOCK_512_DATA_TRANS : (len << 16) | 0x0001));
	mpw7705_writel(mmc, SDIO_SDR_CMD_ARGUMENT_REG, idx);
	mpw7705_writel(mmc, SDIO_SDR_CTRL_REG, CMD17_CTRL);

	if ( ! wait_cmd_done_handle(mmc) ) {
		debug("TIMEOUT:CMD DONE at function SD2buf\n");
		return false;
	}
	if ( ! wait_tran_done_handle(mmc) ) {
		debug("TIMEOUT:TRANSFER DONE at function SD2buf\n");
		return false;
	}
	return true;
}
//----------------------------
//dma_addr is a physical address
//Hi [35:32] must be set through LSIF1 reg
static bool buf2axi(const struct mmc * mmc, int buf_num, int dma_addr, uint len)
{
	mpw7705_writel(mmc, SDIO_BUF_TRAN_CTRL_REG, BUFER_TRANS_START | buf_num);
	mpw7705_writel(mmc, SDIO_DCDTR_1, len);
	mpw7705_writel(mmc, SDIO_DCDSAR_1, dma_addr);
	mpw7705_writel(mmc, SDIO_DCCR_1, DCCR_1_VAL);

	if ( ! wait_ch1_dma_done_handle(mmc) ) {
		debug("TIMEOUT:DMA DONE at function buf2axi.\n");
		return false;
	}
	if ( ! wait_buf_tran_finish_handle(mmc) ) {
		debug("TIMEOUT:BUF TRAN FINISH at function buf2axi.\n");
		return false;
	}
	return true;
}
//------------------------------------------
#define BLOCK_SIZE  512

//dest_addr is a physical address
//Hi [35:32] must be set through LSIF1 reg
static bool sd_read_block(const struct mmc * mmc, uint32_t src_adr, u8 * dst_ptr)
{
	if ( ! SD2buf(mmc, 0, src_adr, BLOCK_SIZE) )
		return false;
	
	if ( ! buf2axi(mmc, 0, (uint32_t) dst_ptr, BLOCK_SIZE) )
		return false;

	return true;
}

static bool sd_read_data(const struct mmc * mmc, uint32_t src_adr, u8 * dst_ptr, uint len)
{
	uint32_t blk_adr = src_adr - (src_adr % BLOCK_SIZE);
	u8 buf[BLOCK_SIZE];
	
	while ( len != 0 ) {
		if ( ! sd_read_block(mmc, blk_adr, buf) ) {
			debug("sd_read_block512 ERROR\n");
			((void (*) (void)) 0xfffc0178)();
		}
		uint32_t ofs = src_adr - blk_adr;
		uint32_t part = BLOCK_SIZE - ofs;
		if ( part > len )
			part = len;
		memcpy(dst_ptr, & buf[ofs], part);
		dst_ptr += part;
		blk_adr += BLOCK_SIZE;
		len -= part;
	}
	
	return true;
}

////////////////////////////////////////////////////////////////

#define PREP_BUF(buf)  prep_buf((buf), sizeof(buf))
#define PRINT_BUF(buf)  print_buf((buf), sizeof(buf))
#ifdef DEBUG_BUF
static void prep_buf(u8 * buf, uint size)
{
	uint i;
	for ( i = 0; i < size; ++ i )
		buf[i] = i;
}

static bool isprintable(const char c)
{
	return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '!' && c <= '?') || c == ' ') ? true : false;
}

static void print_buf(const u8 * buf, uint size)
{
	uint len = (size < 512 ? size : 512), i;
	debug("buf[%d]=", size);
	bool is_zero = true;
	for ( i = 0; i < len; ++ i ) 
		if ( buf[i] != '\0' ) {
			is_zero = false;
			break;
		}
	if ( ! is_zero ) {	 
		while ( len -- ) {
			char c = * buf ++;
			if ( isprintable(c) ) debug("_%c", c);		
			else                  debug("%02x", c);
		}
	} else
		debug("[ZERO]");		
	debug("\n");
}

static void test_read(const struct mmc * mmc)
{
	static u8 block[512];
	uint32_t adr = 0;//0x11800 - 10;
	uint cnt;

	for ( cnt = 0; cnt < 3; ++ cnt ) {
		PREP_BUF(block);
		debug("Read SD card (adr=%d) ... ", adr);		
		bool res = sd_read_data(mmc, adr, block, 512/*sizeof(block)*/);
		debug("%s\n", res ? "ok" : "fail");
		PRINT_BUF(block);
			
		adr += 512;
	}
	
	//((void (*) (void)) 0xfffc0178)();
}
#else
	#define prep_buf(buf, size)
	#define print_buf(buf, size)
#endif
	
static int mpw7705_dm_mmc_get_cd(struct udevice * dev)
{
	struct mpw7705_mmc_platdata * pdata = dev_get_platdata(dev);

	bool carddetected = (dm_gpio_get_value(& pdata->card_detect) == 0);
	debug(">mpw7705_dm_mmc_get_cd: %d\n", carddetected);

	return carddetected ? 1 : 0;
}

#define SDIO_CLK_DIV_400KHz   124
#define SDIO_CLK_DIV_10MHz    4
#define SDIO_CLK_DIV_50MHz    0

static int mpw7705_mmc_probe(struct udevice * dev)
{
	debug(">mpw7705_mmc_probe\n");
	
	struct mpw7705_mmc_platdata * pdata = dev_get_platdata(dev);
	struct mmc_uclass_priv * upriv = dev_get_uclass_priv(dev);
	struct mmc * mmc = & pdata->mmc;
	struct mmc_config * cfg = & pdata->cfg;

	cfg->voltages = MMC_VDD_33_34 | MMC_VDD_32_33 | MMC_VDD_31_32 | MMC_VDD_165_195;	
	cfg->host_caps = MMC_MODE_8BIT | MMC_MODE_4BIT | MMC_MODE_HS_52MHz | MMC_MODE_HS;
	cfg->f_min = DIV_ROUND_UP(24000000, 63);
	cfg->f_max = 52000000; 
	cfg->b_max = 511; /* max 512 - 1 blocks */
	cfg->name = dev->name;

	mmc->priv = pdata;
	upriv->mmc = mmc;

	if (gpio_request_by_name(dev, "carddetect-gpio", 0, &pdata->card_detect,
				 GPIOD_IS_IN)) {
		dev_err(bus, "No cs-gpios property\n");
		return -EINVAL;
	}

	mmc_set_clock(mmc, cfg->f_min, false);
	
	mpw7705_writel(mmc, SPISDIO_SDIO_CLK_DIVIDE, SDIO_CLK_DIV_10MHz);

	mpw7705_writel(mmc, SPISDIO_ENABLE, 0x1);  //sdio-on, spi-off

	/* Hard reset damned hardware */
	mpw7705_writel(mmc, SDIO_SDR_CTRL_REG, 0x1 << 7);    
	while ( mpw7705_readl(mmc, SDIO_SDR_CTRL_REG) & (0x1 << 7) )
		;

	//!!! do we really need it?
	//write_PL022_IMSC(GSPI__, 0x0);//disable interrupts in SSP

	mpw7705_writel(mmc, SPISDIO_SDIO_INT_MASKS, 0x7F);  //enable  interrupts in SDIO

	mpw7705_writel(mmc, SPISDIO_SPI_IRQMASKS, 0x00);  //disable interrupts from ssp

	mpw7705_writel(mmc, SDIO_SDR_ERROR_ENABLE_REG, 0x16F);  //enable interrupt and error flag

	return 0;
}

static int mpw7705_mmc_ofdata_to_platdata(struct udevice *dev)
{
	debug(">mpw7705_mmc_ofdata_to_platdata\n");	
	
	struct mpw7705_mmc_platdata * pdata = dev_get_platdata(dev);	
	fdt_addr_t sdio_addr = devfdt_get_addr(dev);
	debug("sdio_addr: %x\n", (int) sdio_addr);
	
	if ( sdio_addr == FDT_ADDR_T_NONE )
		return -EINVAL;
	
	pdata->regbase = sdio_addr;

	return 0;
}

int mpw7705_mmc_bind(struct udevice *dev)
{
	debug(">mpw7705_mmc_bind\n");
	
	struct mpw7705_mmc_platdata * pdata = dev_get_platdata(dev);
	return mmc_bind(dev, & pdata->mmc, & pdata->cfg);
}

static int mpw7705_dm_mmc_set_ios(struct udevice * dev)
{
#ifdef DEBUG	
	struct mmc * mmc = mmc_get_mmc_dev(dev);
#endif	
	debug(">>mpw7705_dm_mmc_set_ios: clock=%d, bus_width=%d\n", mmc->clock, mmc->bus_width);
	return 0;
}

static void mpw7705_mmc_read_response(struct mmc *mmc, struct mmc_cmd *cmd)
{
	if (cmd->resp_type & MMC_RSP_136) {
		cmd->response[0] = mpw7705_readl(mmc, SDIO_SDR_RESPONSE4_REG);
		cmd->response[1] = mpw7705_readl(mmc, SDIO_SDR_RESPONSE3_REG);
		cmd->response[2] = mpw7705_readl(mmc, SDIO_SDR_RESPONSE2_REG);
		cmd->response[3] = mpw7705_readl(mmc, SDIO_SDR_RESPONSE1_REG);
		debug(" {0x%x, 0x%x, 0x%x, 0x%x}\n", cmd->response[0], cmd->response[1], cmd->response[2], cmd->response[3]);
	} else {
		cmd->response[0] = mpw7705_readl(mmc, SDIO_SDR_RESPONSE1_REG);
		debug(" {0x%x}\n", cmd->response[0]);
	}
}

static bool proc_cmd(const struct mmc * mmc, uint code, uint arg)
{
	uint attempt = 10;
	while ( attempt -- ) {
		if ( sd_send_cmd(mmc, code, arg) )
			return true;
		debug("+");
		delay_loop(10000);
	}
	return false;
}

static int mpw7705_dm_mmc_send_cmd(struct udevice *dev, struct mmc_cmd *cmd, struct mmc_data *data)
{
	struct mmc * mmc = mmc_get_mmc_dev(dev);
	int ret = 0;

	debug(">CMD: cmd=%d, resp= 0x%x, arg=0x%x", cmd->cmdidx, cmd->resp_type, cmd->cmdarg);
	if ( data ) 
		debug(" %s bqty=%d, bsz=%d", (data->flags == MMC_DATA_READ ? "read" : "write"), data->blocks, data->blocksize);
	debug(" ...\n");

	int resp = SDIO_RESPONSE_NONE;	
	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136)
			resp = SDIO_RESPONSE_R1367;

		if (cmd->resp_type & MMC_RSP_BUSY)
			resp = SDIO_RESPONSE_R1b;
		
		if (cmd->resp_type & MMC_RSP_R2)
			resp = SDIO_RESPONSE_R2;
	} 
	resp = SDIO_RESPONSE_R1367;
	int crc = (cmd->resp_type & MMC_RSP_CRC) ? 1 : 0; 
	int idx = (cmd->resp_type & MMC_RSP_OPCODE) ? 1 : 0; 
	
	bool res = false;
	switch ( cmd->cmdidx ) {
	case MMC_CMD_GO_IDLE_STATE :
		res = proc_cmd(mmc, SDIO_CTRL_NODATA(0, 0, 0, 0), cmd->cmdarg);
		break;
	case MMC_CMD_SEND_EXT_CSD :
		res = proc_cmd(mmc, SDIO_CTRL_NODATA(8, SDIO_RESPONSE_R1367, crc, idx), cmd->cmdarg);
		break;
	case MMC_CMD_APP_CMD :
		res = proc_cmd(mmc, SDIO_CTRL_NODATA(55, SDIO_RESPONSE_R1367, crc, idx), cmd->cmdarg);
		break;
	case SD_CMD_APP_SEND_OP_COND :
		res = proc_cmd(mmc, SDIO_CTRL_NODATA(41, resp, crc, idx), cmd->cmdarg);
		break;
	case MMC_CMD_ALL_SEND_CID :
		res = proc_cmd(mmc, SDIO_CTRL_NODATA(2, SDIO_RESPONSE_R2, crc, idx), cmd->cmdarg);
		break;		
	case MMC_CMD_SET_RELATIVE_ADDR :
		res = proc_cmd(mmc, SDIO_CTRL_NODATA(3, SDIO_RESPONSE_R1367, crc, idx), cmd->cmdarg);
		break;
	case MMC_CMD_SEND_CSD :
		res = proc_cmd(mmc, SDIO_CTRL_NODATA(9, SDIO_RESPONSE_R2, crc, idx), cmd->cmdarg);
		break;
	case MMC_CMD_SELECT_CARD :
		res = proc_cmd(mmc, SDIO_CTRL_NODATA(7, SDIO_RESPONSE_R1367, crc, idx), cmd->cmdarg);
		break;
	case SD_CMD_APP_SET_BUS_WIDTH :
		res = proc_cmd(mmc, SDIO_CTRL_NODATA(6, SDIO_RESPONSE_R1367, crc, idx), cmd->cmdarg);
		break;
	case MMC_CMD_SET_BLOCKLEN :
		res = proc_cmd(mmc, SDIO_CTRL_NODATA(16, SDIO_RESPONSE_R1367, crc, idx), 512/*cmd->cmdarg*/);  //!!!
		break;		
	case MMC_CMD_STOP_TRANSMISSION :
		res = true;  //!!! no need		
		//res = proc_cmd(mmc, SDIO_CTRL_NODATA(12, SDIO_RESPONSE_R1367, crc, idx), cmd->cmdarg);
		break;		
		
	case MMC_CMD_READ_SINGLE_BLOCK :	
	case MMC_CMD_READ_MULTIPLE_BLOCK :	
		if ( data ) {
			if ( data->flags == MMC_DATA_READ ) {
				debug(">>READ: src=0x%x, dst=0x%x, bqty=%d, bsz=%d\n", cmd->cmdarg, (uint32_t) data->dest, data->blocks, data->blocksize);
				uint blk_qty = data->blocks;
				ulong src_adr = cmd->cmdarg;
				u8 * dst_ptr = (u8 *) data->dest;
				while ( blk_qty != 0 ) {				
#ifdef DEBUG_BUF
					debug(">>>%d:", blk_qty);
#endif					
					res = sd_read_data(mmc, src_adr, dst_ptr, data->blocksize);
					if ( res ) 
						print_buf(dst_ptr, data->blocksize);
					if ( ! res )
						break;
					src_adr += data->blocksize;
					dst_ptr += data->blocksize;
					-- blk_qty;
				}
				debug("\n");
			} else
				debug("Bad flag in MMC_CMD_READ\n");				
		}
		break;
		
	case SD_CMD_APP_SEND_SCR :
		{
			uint len = data->blocks * data->blocksize;
			uint attempt;
			for ( attempt = 0; attempt < 5; ++ attempt ) {
				//res = true;
				mpw7705_writel(mmc, SDIO_SDR_ADDRESS_REG, (0 << 2));
				mpw7705_writel(mmc, SDIO_SDR_CARD_BLOCK_SET_REG, (len << 16) | 0x01);
				resp = SDIO_RESPONSE_R1367;
				uint32_t cmd_code = (51 << 16) | 0x7811;				
				res = sd_send_cmd(mmc, cmd_code, cmd->cmdarg);
				if ( res )
					res = wait_tran_done_handle(mmc);
				if ( res )
					break;
				debug("+");
				delay_loop(100000);
			}
			//!!! cannot yet
			if ( res ) {
/*				
				delay_loop(1000000);
				u8 buf[512];
				PREP_BUF(buf);
				res = buf2axi(0, (uint32_t) buf, len);
				debug(">>>BUF: res=%d\n", res);
				print_buf(buf, len);
*/
				uint8_t buf[8] = { 0x02, 0x35, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 };
				print_buf(buf, len);
				if ( res )
					memcpy(data->dest, buf, len);
			}
		}
		break;

	case SD_CMD_APP_SD_STATUS :
		{
			uint len = data->blocks * data->blocksize;
			uint attempt;
			for ( attempt = 0; attempt < 5; ++ attempt ) {
				//res = true;
				mpw7705_writel(mmc, SDIO_SDR_ADDRESS_REG, (0 << 2));
				mpw7705_writel(mmc, SDIO_SDR_CARD_BLOCK_SET_REG, (len << 16) | 0x01);
				resp = SDIO_RESPONSE_R1367;
				uint32_t cmd_code = (13 << 16) | 0x7811;				
				res = sd_send_cmd(mmc, cmd_code, cmd->cmdarg);
				if ( res )
					res = wait_tran_done_handle(mmc);
				if ( res )
					break;
				debug("+");
				delay_loop(100000);
			}
			//!!! cannot yet
			if ( res ) {					
/*			
				delay_loop(1000000);
				u8 buf[512];
				PREP_BUF(buf);
				res = buf2axi(0, (uint32_t) buf, len);
				debug(">>>BUF: res=%d\n", res);
*/				
				uint8_t buf[64] = { 0x80, 0x00, 0x00, 0x00,   0x02, 0x00, 0x00, 0x00,   0x03, 0x03, 0x90, 0x00,   0x08, 0x05, 0x00, 0x00,  
														0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,    
														0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,    
														0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00 };
				print_buf(buf, len);
				if ( res )
					memcpy(data->dest, buf, data->blocks * data->blocksize);
			}
		}
		break;
		
	default:
#ifdef DEBUG	
		((void (*) (void)) 0xfffc0178)();
#endif		
		res = proc_cmd(mmc, SDIO_CTRL_NODATA(cmd->cmdidx, resp, crc, idx), cmd->cmdarg);
		break;
	}
	debug(" %s", res ? "ok" : "error");
	
	if ( ! res )
		ret = -EIO;
	
	mpw7705_mmc_read_response(mmc, cmd);
	
	return ret;
}

static const struct udevice_id mpw7705_mmc_match[] = {
	{ .compatible = "rc-module,mmc-0.2" },
	{ }
};

static const struct dm_mmc_ops mpw7705_dm_mmc_ops = {
	.send_cmd = mpw7705_dm_mmc_send_cmd,
	.set_ios = mpw7705_dm_mmc_set_ios,
	.get_cd = mpw7705_dm_mmc_get_cd
};

U_BOOT_DRIVER(mpw7705_mmc_drv) = {
	.name = "mpw7705_mmc_drv",
	.id = UCLASS_MMC,
	.of_match = mpw7705_mmc_match,
	.ops = & mpw7705_dm_mmc_ops,
	.probe = mpw7705_mmc_probe,
	.bind = mpw7705_mmc_bind,
	.ofdata_to_platdata = mpw7705_mmc_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct mpw7705_mmc_platdata),
	.priv_auto_alloc_size = 1000
};
