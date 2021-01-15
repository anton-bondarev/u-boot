//#define DEBUG
//#define DEBUG_BUF
//#define DEBUG_REG

#include <common.h>
#include <errno.h> 
#include <malloc.h>
#include <part.h>
#include <mmc.h>
#include <u-boot/crc.h>
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

#include "rcm_sysreg.h"

// !!! Checking possible DMA problem with memory 
// #define DMA_PAD  (4 * 1024)

#define BUS_CLOCK  100000000
#define CLKDIV_MAX 255

#define SDIO_TIMEOUT        2000000 

// ASTRO TODO: find a real place where we need this delay...
#ifdef DEBUG 
	#define Debug(...)  debug(__VA_ARGS__)
#else
	#define Debug(...)  delay_loop(3000)
#endif	

DECLARE_GLOBAL_DATA_PTR;

struct rcm_mmc_platdata {
	struct mmc_config cfg;
	struct mmc mmc;
	uint32_t reg_base;
	struct gpio_desc card_detect;
	uint bus_width;
#ifdef CONFIG_TARGET_1888BC048
	u8* blk_buf;
#endif
};

static inline struct rcm_mmc_platdata * mmc_get_platdata(const struct mmc * mmc)
{
	return (struct rcm_mmc_platdata *) mmc->priv;
}

static uint32_t wait_tick(void) 
{ 
	static volatile uint32_t tick = 0;
	return tick ++; 
}

static void delay_loop(uint count)
{
	volatile uint cnt = count;
#ifdef CONFIG_TARGET_1888BC048
	cnt <<= 4;
#endif
	while ( cnt -- )
		;
}

#define SDR_TRAN_FIFO_FINISH    0x00000001
#define SDR_TRAN_SDC_DAT_DONE   0x00000002
#define SDR_TRAN_SDC_CMD_DONE   0x00000004
#define SDR_TRAN_DAT_BOUNT      0x00000008
#define SDR_TRAN_SDC_ERR        0x00000010

#define DSSR_CHANNEL_TR_DONE    0x00080000

#define BLOCK_512_DATA_TRANS    0x00000101
#define BUFER_TRANS_WRITE       0x02
#define BUFER_TRANS_START       0x04
#define DCCR_VAL_READ           0x00020F01
#define DCCR_VAL_WRITE          0x000020F1

//-------------------------------------

#ifdef DEBUG_REG
static void rcm_print_op(char * func, uint32_t adr, int reg, uint val)
{
	static int s_reg = -1;
	static uint s_val;
	static bool flag;
	
	if ( reg != s_reg || val != s_val ) {
		Debug("%s: adr=0x%x, reg = 0x%x, val = 0x%x\n", func, adr, reg, val);
		s_reg = reg;
		s_val = val;
		flag = false;
	} else if ( ! flag ) {
		Debug("+++\n");
		flag = true;		
	}
}
#else
	#define rcm_print_op(func, adr, reg, val)
#endif

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
	Debug("buf[%d]=", size);
	bool is_zero = true;
	for ( i = 0; i < len; ++ i ) 
		if ( buf[i] != '\0' ) {
			is_zero = false;
			break;
		}
	if ( ! is_zero ) {	 
		while ( len -- ) {
			char c = * buf ++;
			if ( isprintable(c) ) Debug("_%c", c);		
			else                  Debug("%02x", c);
		}
	} else
		Debug("[ZERO]");		
	Debug("\n");
}
#else
	#define prep_buf(buf, size)
	#define print_buf(buf, size)
#endif

////////////////////////////////////////////////////////////////

static inline void rcm_writel(const struct mmc * mmc, int reg, u32 val)
{
	uint32_t regbase = mmc_get_platdata(mmc)->reg_base;
	rcm_print_op("rcm_writeL", regbase, reg, val);
	writel(val, regbase + reg);
}
static inline void rcm_writew(const struct mmc * mmc, int reg, u16 val)
{
	uint32_t regbase = mmc_get_platdata(mmc)->reg_base;
	rcm_print_op("rcm_writeW", regbase, reg, val);
	writew(val, regbase + reg);
}
static inline void rcm_writeb(const struct mmc * mmc, int reg, u8 val)
{
	uint32_t regbase = mmc_get_platdata(mmc)->reg_base;
	rcm_print_op("rcm_writeB", regbase, reg, val);
	writeb(val, regbase + reg);
}

static inline u32 rcm_readl(const struct mmc * mmc, int reg)
{
	uint32_t regbase = mmc_get_platdata(mmc)->reg_base;
	u32 retval = readl(regbase + reg);
	rcm_print_op("rcm_readL", regbase, reg, retval);
	return retval;
}
static inline u16 rcm_sdhci_readw(const struct mmc * mmc, int reg)
{
	uint32_t regbase = mmc_get_platdata(mmc)->reg_base;
	u16 retval = readw(regbase + reg);
	rcm_print_op("rcm_readW", regbase, reg, retval);
	return retval;
}
static inline u8 rcm_readb(const struct mmc * mmc, int reg)
{
	uint32_t regbase = mmc_get_platdata(mmc)->reg_base;
	u8 retval = readb(regbase + reg);
	rcm_print_op("rcm_readB", regbase, reg, retval);
	return retval;
}

//-------------------------------------

static bool wait_cmd_done_handle(const struct mmc * mmc) 
{
	uint32_t start = wait_tick();
	do {
		int status = rcm_readl(mmc, SPISDIO_SDIO_INT_STATUS);
		if ( status & SPISDIO_SDIO_INT_STATUS_CAR_ERR ) {
			rcm_writel(mmc, SDIO_SDR_BUF_TRAN_RESP_REG, SDR_TRAN_SDC_ERR);
			Debug(" ECMD<0x%x>\n", status);
			return false;
		} else if ( status & SPISDIO_SDIO_INT_STATUS_CMD_DONE ) {
			rcm_writel(mmc, SDIO_SDR_BUF_TRAN_RESP_REG, SDR_TRAN_SDC_CMD_DONE);
			return true;
		}
	} while ( wait_tick() - start < SDIO_TIMEOUT );

	Debug("CMD ERROR: TIMEOUT\n");
	return false;
}
 
static bool wait_tran_done_handle(const struct mmc * mmc)
{
	uint32_t start = wait_tick();
	do {
		int status = rcm_readl(mmc, SPISDIO_SDIO_INT_STATUS);
		if ( status & SPISDIO_SDIO_INT_STATUS_CAR_ERR ) {
			rcm_writel(mmc, SDIO_SDR_BUF_TRAN_RESP_REG, SDR_TRAN_SDC_ERR);
			Debug(" ETRN<0x%x>\n", status);
			return false;
		} else if ( status & SPISDIO_SDIO_INT_STATUS_TRAN_DONE ) {
			rcm_writel(mmc, SDIO_SDR_BUF_TRAN_RESP_REG, SDR_TRAN_SDC_DAT_DONE);
			return true;
		}
	} while ( wait_tick() - start < SDIO_TIMEOUT );

	Debug("TRN ERROR: TIMEOUT\n");
	return false;
}

static bool wait_ch0_dma_done_handle(const struct mmc * mmc)
{
	uint32_t start = wait_tick();
	do {
		int status = rcm_readl(mmc, SPISDIO_SDIO_INT_STATUS);
		if ( status & SPISDIO_SDIO_INT_STATUS_CAR_ERR ) {
			rcm_writel(mmc, SDIO_SDR_BUF_TRAN_RESP_REG, SDR_TRAN_SDC_ERR);
			Debug(" EDMA_0<0x%x>\n", status);
			return false;
		} else if ( status & SPISDIO_SDIO_INT_STATUS_CH0_FINISH ) {
			rcm_writel(mmc, SDIO_DCCR_0, DSSR_CHANNEL_TR_DONE);
			return true;
		}
	} while ( wait_tick() - start < SDIO_TIMEOUT );

	Debug("DMA_0 ERROR: TIMEOUT\n");
	return false;
}

static bool wait_ch1_dma_done_handle(const struct mmc * mmc)
{
	uint32_t start = wait_tick();
	do {
		int status = rcm_readl(mmc, SPISDIO_SDIO_INT_STATUS);
		if ( status & SPISDIO_SDIO_INT_STATUS_CAR_ERR ) {
			rcm_writel(mmc, SDIO_SDR_BUF_TRAN_RESP_REG, SDR_TRAN_SDC_ERR);
			Debug(" EDMA_1<0x%x>\n", status);
			return false;
		} else if ( status & SPISDIO_SDIO_INT_STATUS_CH1_FINISH ) {
			rcm_writel(mmc, SDIO_DCCR_1, DSSR_CHANNEL_TR_DONE);
			return true;
		}
	} while ( wait_tick() - start < SDIO_TIMEOUT );

	Debug("DMA_1 ERROR: TIMEOUT\n");
	return false;
}

static bool wait_buf_tran_finish_handle(const struct mmc * mmc)
{
	uint32_t start = wait_tick();
	do {
		int status = rcm_readl(mmc, SPISDIO_SDIO_INT_STATUS);
		if ( status & SPISDIO_SDIO_INT_STATUS_CAR_ERR ) {
			rcm_writel(mmc, SDIO_SDR_BUF_TRAN_RESP_REG, SDR_TRAN_SDC_ERR);
			Debug(" EBLK<0x%x>\n", status);
			return false;
		} else if ( status & SPISDIO_SDIO_INT_STATUS_BUF_FINISH ) {
			rcm_writel(mmc, SDIO_SDR_BUF_TRAN_RESP_REG, SDR_TRAN_FIFO_FINISH);
			return true;
		}
	} while( wait_tick() - start < SDIO_TIMEOUT );

	Debug("BLK ERROR: TIMEOUT\n");
	return false;
}
//-------------------------------------

static bool sd_send_cmd(const struct mmc * mmc, uint32_t cmd_ctrl, uint32_t arg)
{
	Debug("SEND CMD_%d(%x)[%x]", cmd_ctrl >> 16, cmd_ctrl & 0xFFFF, arg);
	rcm_writel(mmc, SDIO_SDR_CMD_ARGUMENT_REG, arg);
	rcm_writel(mmc, SDIO_SDR_CTRL_REG, cmd_ctrl);
	bool result = wait_cmd_done_handle(mmc);
	Debug(" res=%s", (result ? "ok" : "err"));
	delay_loop(200);
	return result;
}
////////////////////////////////////////////////////////////////

#define CMD17_CTRL  0x00117910
#define CMD24_CTRL  0x00187810

static bool SD_buf(const struct mmc * mmc, uint cmd_ctrl, int buf_num, int idx, uint len)
{
	rcm_writel(mmc, SDIO_SDR_ADDRESS_REG, (buf_num << 2));
	rcm_writel(mmc, SDIO_SDR_CARD_BLOCK_SET_REG, (len == 512 ? BLOCK_512_DATA_TRANS : (len << 16) | 0x0001));
	rcm_writel(mmc, SDIO_SDR_CMD_ARGUMENT_REG, idx);
	rcm_writel(mmc, SDIO_SDR_CTRL_REG, cmd_ctrl | mmc_get_platdata(mmc)->bus_width);

	if ( ! wait_cmd_done_handle(mmc) ) 
		return false;
	
	if ( ! wait_tran_done_handle(mmc) ) 
		return false;
	
	return true;
}

static inline bool SD2buf(const struct mmc * mmc, int buf_num, int idx, uint len)
{
	return SD_buf(mmc, CMD17_CTRL, buf_num, idx, len);
}

static inline bool buf2SD(const struct mmc * mmc, int buf_num, int idx, uint len)
{
	return SD_buf(mmc, CMD24_CTRL, buf_num, idx, len);
}

//----------------------------

typedef enum 
{
	DATA_READ,
	DATA_WRITE
} data_dir;

static bool AXI_buf(const struct mmc * mmc, data_dir dir, uint buf_num, u8 * mem_ptr, uint len) 
{
	rcm_writel(mmc, SDIO_BUF_TRAN_CTRL_REG, BUFER_TRANS_START | (dir == DATA_WRITE ? BUFER_TRANS_WRITE : 0x0) | buf_num);
	rcm_writel(mmc, (dir == DATA_WRITE ? SDIO_DCDTR_0 : SDIO_DCDTR_1), len);
	rcm_writel(mmc, (dir == DATA_WRITE ? SDIO_DCSSAR_0 : SDIO_DCDSAR_1), (uint32_t) mem_ptr);
	rcm_writel(mmc, (dir == DATA_WRITE ? SDIO_DCCR_0 : SDIO_DCCR_1), (dir == DATA_WRITE ? DCCR_VAL_WRITE : DCCR_VAL_READ));

	if ( ! wait_ch1_dma_done_handle(mmc) ) 
		return false;
	
	if ( ! wait_buf_tran_finish_handle(mmc) ) 
		return false;
	
	return true;
}

static inline bool buf2axi(const struct mmc * mmc, uint buf_num, u8 * mem_ptr, uint len)
{
#ifdef DMA_PAD	
	u8 buf[3 * DMA_PAD], * ptr = & buf[DMA_PAD];
	ptr = (u8 *) ((uint32_t) ptr & ~0xF);
	
	bool res = AXI_buf(mmc, DATA_READ, buf_num, ptr, len);
	if ( ! res ) 
		return false;
	
	memcpy(mem_ptr, ptr, len);
	
	return true;
#else
	return AXI_buf(mmc, DATA_READ, buf_num, mem_ptr, len);
#endif	
}

static inline bool axi2buf(const struct mmc * mmc, uint buf_num, u8 * mem_ptr, uint len)
{
	return AXI_buf(mmc, DATA_WRITE, buf_num, mem_ptr, len);
}

//------------------------------------------

static bool sd_read_block(const struct mmc * mmc, uint32_t src_adr, u8 * dst_ptr, uint len)
{
	if ( ! SD2buf(mmc, 0, src_adr, len) )
		return false;
	
	if ( ! buf2axi(mmc, 0, dst_ptr, len) )
		return false;

	return true;
}

static bool sd_write_block(const struct mmc * mmc, uint32_t dst_adr, u8 * src_ptr, uint len)
{
	rcm_writel(mmc, SDIO_SDR_CARD_BLOCK_SET_REG, (len == 512 ? BLOCK_512_DATA_TRANS : (len << 16) | 0x0001));
	rcm_writel(mmc, SDIO_BUF_TRAN_CTRL_REG, BUFER_TRANS_START | BUFER_TRANS_WRITE | 0);
	rcm_writel(mmc, SDIO_DCDTR_0, len);
	rcm_writel(mmc, SDIO_DCSSAR_0, (uint32_t) src_ptr);
	rcm_writel(mmc, SDIO_DCCR_0, DCCR_VAL_WRITE);
	
	if ( ! wait_ch0_dma_done_handle(mmc) ) 
		return false;
	
	if ( ! wait_buf_tran_finish_handle(mmc) ) 
		return false;

	rcm_writel(mmc, SDIO_SDR_ADDRESS_REG, (0 << 2));
	rcm_writel(mmc, SDIO_SDR_CARD_BLOCK_SET_REG, (len == 512 ? BLOCK_512_DATA_TRANS : (len << 16) | 0x0001));
	rcm_writel(mmc, SDIO_SDR_CMD_ARGUMENT_REG, dst_adr);
	rcm_writel(mmc, SDIO_SDR_CTRL_REG, CMD24_CTRL | mmc_get_platdata(mmc)->bus_width);
	
	if ( ! wait_cmd_done_handle(mmc) ) 
		return false;
	
	if ( ! wait_tran_done_handle(mmc) ) 
		return false;

	return true;
}

typedef bool (* sd_data_oper)(const struct mmc * mmc, uint32_t sdc_adr, u8 * mem_ptr, uint len);

static bool sd_read_block(const struct mmc*, uint32_t, u8*, uint);
static bool sd_write_block(const struct mmc*, uint32_t, u8*, uint);

static bool sd_trans_data(const struct mmc * mmc, sd_data_oper oper, uint32_t sdc_adr, u8 * mem_ptr, uint blk_qty, uint blk_len)
{
#ifndef CONFIG_TARGET_1888BC048
	u8* blk_buf = (u8*)memalign( 64, blk_len );
#else
	u8* blk_buf = ((struct rcm_mmc_platdata*)dev_get_platdata(mmc->dev))->blk_buf; // reserved memory
#endif
	int ret = false;

	if( ! blk_buf )
		goto exit;
	//BUG_ON(! mmc->high_capacity && (sdc_adr % blk_len) != 0);
	while ( blk_qty -- ) {
			if( oper == sd_write_block )
				memcpy( blk_buf, mem_ptr, blk_len );
			if ( ! (* oper)(mmc, sdc_adr, blk_buf, blk_len) ) {
				Debug("sd_trans_data ERROR\n");
				goto exit;
			}
			if( oper == sd_read_block )
				memcpy( mem_ptr, blk_buf, blk_len );
#ifdef DEBUG_BUF
			Debug(">>%d:", blk_qty);
			print_buf(mem_ptr, blk_len);
#endif					
			mem_ptr += blk_len;
			sdc_adr += (mmc->high_capacity ? 1 : blk_len);
	}
	ret = true;
#ifdef DEBUG_BUF
	Debug("\n");
#endif
exit:
#ifndef CONFIG_TARGET_1888BC048
	if (blk_buf) free(blk_buf);
#endif
	return ret;
}

static inline bool sd_read_data(const struct mmc * mmc, uint32_t src_adr, u8 * dst_ptr, uint blk_qty, uint blk_len)
{
	return sd_trans_data(mmc, sd_read_block, src_adr, dst_ptr, blk_qty, blk_len);
}

static inline bool sd_write_data(const struct mmc * mmc, uint32_t dst_adr, u8 * src_ptr, uint blk_qty, uint blk_len)
{
	return sd_trans_data(mmc, sd_write_block, dst_adr, src_ptr, blk_qty, blk_len);
}
	
////////////////////////////////////////////////////////////////

static int rcm_dm_mmc_get_cd(struct udevice * dev)
{
	struct rcm_mmc_platdata * pdata = dev_get_platdata(dev);

	bool carddetected = (dm_gpio_get_value(& pdata->card_detect) == 0);
	Debug(">rcm_dm_mmc_get_cd: %d\n", carddetected);

	return carddetected ? 1 : 0;
}

static int rcm_dm_mmc_get_wp(struct udevice * dev)
{
	Debug(">rcm_dm_mmc_get_cd:\n");

	return 0;
}

#ifdef CONFIG_TARGET_1888BC048
static int rcm_mmc_map_memory(struct udevice *dev)
{
	struct fdt_resource r;
	struct rcm_mmc_platdata* pdata = dev_get_platdata(dev);
	int node = dev_of_offset(dev);
	int phandle;

	if ((phandle=fdtdec_lookup_phandle(gd->fdt_blob, node, "mmc-buffer")) < 0)
		return -EINVAL;

	if (fdt_get_resource(gd->fdt_blob, phandle, "reg", 0, &r)<0)
		return -EINVAL;

	pdata->blk_buf = (u8*)r.start;
	return 0;
}
#endif

static int rcm_mmc_probe(struct udevice * dev)
{
	Debug(">rcm_mmc_probe\n");
	
	struct rcm_mmc_platdata * pdata = dev_get_platdata(dev);
	struct mmc_uclass_priv * upriv = dev_get_uclass_priv(dev);
	struct mmc * mmc = & pdata->mmc;
	struct mmc_config * cfg = & pdata->cfg;

	cfg->voltages = MMC_VDD_33_34 | MMC_VDD_32_33 | MMC_VDD_31_32 | MMC_VDD_165_195;	
	cfg->host_caps = MMC_MODE_8BIT | MMC_MODE_4BIT | MMC_MODE_HS_52MHz | MMC_MODE_HS;
	cfg->f_min = 400000;
	cfg->f_max = 8000000; 
	cfg->b_max = 511; /* max 512 - 1 blocks */
	cfg->name = dev->name;

	mmc->priv = pdata;
	upriv->mmc = mmc;

	if (gpio_request_by_name(dev, "carddetect-gpio", 0, &pdata->card_detect,
				 GPIOD_IS_IN)) {
		dev_err(bus, "No cs-gpios property\n");
		return -EINVAL;
	}

#ifdef CONFIG_TARGET_1888BC048
	if (rcm_mmc_map_memory(dev)) {
		return -EINVAL;
	}
#endif

	mmc_set_clock(mmc, cfg->f_min, false);

	rcm_writel(mmc, SPISDIO_ENABLE, 0x1);  //sdio-on, spi-off

	/* Hard reset damned hardware */
	rcm_writel(mmc, SDIO_SDR_CTRL_REG, 0x1 << 7);    
	while ( rcm_readl(mmc, SDIO_SDR_CTRL_REG) & (0x1 << 7) )
		;

	//!!! do we really need it?
	//write_PL022_IMSC(GSPI__, 0x0);//disable interrupts in SSP

	rcm_writel(mmc, SPISDIO_SDIO_INT_MASKS, 0x7F);  //enable  interrupts in SDIO

	rcm_writel(mmc, SPISDIO_SPI_IRQMASKS, 0x00);  //disable interrupts from ssp

	rcm_writel(mmc, SDIO_SDR_ERROR_ENABLE_REG, 0x16F);  //enable interrupt and error flag

	// rcm_writel(mmc, SPISDIO_ENABLE, 0x0);  //sdio-on, spi-off

	return 0;
}

static int rcm_mmc_ofdata_to_platdata(struct udevice *dev)
{
	Debug(">rcm_mmc_ofdata_to_platdata\n");	
	
	struct rcm_mmc_platdata * pdata = dev_get_platdata(dev);	
	fdt_addr_t sdio_addr = devfdt_get_addr(dev);
	Debug("sdio_addr: %x\n", (int) sdio_addr);
	
	if ( sdio_addr == FDT_ADDR_T_NONE )
		return -EINVAL;
	
	pdata->reg_base = sdio_addr;

	return 0;
}

int rcm_mmc_bind(struct udevice *dev)
{
	Debug(">rcm_mmc_bind\n");
	
	struct rcm_mmc_platdata * pdata = dev_get_platdata(dev);
	return mmc_bind(dev, & pdata->mmc, & pdata->cfg);
}

static void set_clock(struct udevice * dev, uint clock)
{
	struct mmc * mmc = mmc_get_mmc_dev(dev);
	uint32_t m = CLKDIV_MAX;
	if(clock)
	{
		m = BUS_CLOCK/(clock*2) - 1;		
		if ( m > CLKDIV_MAX )
			m = CLKDIV_MAX;
	}
	rcm_writel(mmc, SPISDIO_SDIO_CLK_DIVIDE, m);  
}

static int rcm_dm_mmc_set_ios(struct udevice * dev)
{
	struct rcm_mmc_platdata * pdata = dev_get_platdata(dev);	
	struct mmc * mmc = mmc_get_mmc_dev(dev);
	Debug(">>rcm_dm_mmc_set_ios: clock=%d, bus_width=%d\n", mmc->clock, mmc->bus_width);
	
	pdata->bus_width = (mmc->bus_width == 4 ? 1 : 0);
	set_clock(dev, mmc->clock);

	return 0;
}

static void rcm_mmc_read_response(struct mmc *mmc, struct mmc_cmd *cmd)
{
	if (cmd->resp_type & MMC_RSP_136) {
		cmd->response[0] = rcm_readl(mmc, SDIO_SDR_RESPONSE4_REG);
		cmd->response[1] = rcm_readl(mmc, SDIO_SDR_RESPONSE3_REG);
		cmd->response[2] = rcm_readl(mmc, SDIO_SDR_RESPONSE2_REG);
		cmd->response[3] = rcm_readl(mmc, SDIO_SDR_RESPONSE1_REG);
		Debug(" {0x%x, 0x%x, 0x%x, 0x%x}\n", cmd->response[0], cmd->response[1], cmd->response[2], cmd->response[3]);
	} else {
		cmd->response[0] = rcm_readl(mmc, SDIO_SDR_RESPONSE1_REG);
		Debug(" {0x%x}\n", cmd->response[0]);
	}
}

static bool proc_cmd(const struct mmc * mmc, uint code, uint arg)
{
	uint attempt = 10;
	while ( attempt -- ) {
		if ( sd_send_cmd(mmc, code, arg) )
			return true;
		Debug("+");
		delay_loop(10000);
	}
	return false;
}

#define SDIO_RESPONSE_NONE  0
#define SDIO_RESPONSE_R2    1
#define SDIO_RESPONSE_R1367 2
#define SDIO_RESPONSE_R1b   3

#define SDIO_CTRL_NODATA(_cmd, _resp, _crc, _idx, _bw)  \
   (((_cmd) << 16) | ((_idx) << 12) | ((_crc) << 13)  | ((_resp) << 10) | (0x10) | ((_bw) << 0))

#define CMD_HAS_DATA   0x4000
#define CMD_DATA_READ  0x0100

static int rcm_dm_mmc_send_cmd(struct udevice *dev, struct mmc_cmd *cmd, struct mmc_data *data)
{
	struct mmc * mmc = mmc_get_mmc_dev(dev);
	int ret = 0;
	
	// rcm_writel(mmc, SPISDIO_ENABLE, 0x1);  //sdio-on, spi-off

	int resp = SDIO_RESPONSE_NONE;	
	if ( cmd->resp_type & MMC_RSP_PRESENT ) {
		if ( cmd->resp_type & MMC_RSP_136 )
			resp = SDIO_RESPONSE_R2;
		else if ( cmd->resp_type & MMC_RSP_BUSY ) 
			resp = SDIO_RESPONSE_R1b;
		else 
			resp = SDIO_RESPONSE_R1367;
	}

	Debug(">CMD: cmd=%d, resp= 0x%x(0x%x), arg=0x%x", cmd->cmdidx, cmd->resp_type, resp, cmd->cmdarg);
	if ( data ) 
		Debug(" %s bqty=%d, bsz=%d", (data->flags == MMC_DATA_READ ? "read" : "write"), data->blocks, data->blocksize);
	Debug(" ...\n");

	int crc = (cmd->resp_type & MMC_RSP_CRC) ? 1 : 0; 
	int idx = (cmd->resp_type & MMC_RSP_OPCODE) ? 1 : 0; 
	uint bw = mmc_get_platdata(mmc)->bus_width;
	bool is_int_data_cmd = (!! data && (cmd->cmdidx == 6 || cmd->cmdidx == 13 || cmd->cmdidx == 51));
	uint data_len = 0;
	uint32_t cmd_code = SDIO_CTRL_NODATA(cmd->cmdidx, resp, crc, idx, bw);
	if ( is_int_data_cmd ) {
		cmd_code |= (CMD_HAS_DATA | CMD_DATA_READ);
		data_len = data->blocks * data->blocksize;
		rcm_writel(mmc, SDIO_SDR_ADDRESS_REG, (0 << 2));
		rcm_writel(mmc, SDIO_SDR_CARD_BLOCK_SET_REG, (data_len << 16) | 0x0001);
	}
	
	bool res = false;
	switch ( cmd->cmdidx ) {
	case MMC_CMD_GO_IDLE_STATE :
	case MMC_CMD_SEND_EXT_CSD :
	case MMC_CMD_APP_CMD :
	case SD_CMD_APP_SEND_OP_COND :
	case MMC_CMD_ALL_SEND_CID :
	case MMC_CMD_SET_RELATIVE_ADDR :
	case MMC_CMD_SEND_CSD :
	case MMC_CMD_SELECT_CARD :
	case MMC_CMD_SET_BLOCKLEN :
	case 6 :  // MMC_CMD_SWITCH, SD_CMD_SWITCH_FUNC, SD_CMD_APP_SET_BUS_WIDTH
	case 13 :  // MMC_CMD_SEND_STATUS, SD_CMD_APP_SD_STATUS	
	case 51 :  // SD_CMD_APP_SEND_SCR
		res = proc_cmd(mmc, cmd_code, cmd->cmdarg);
		break;
		
	case MMC_CMD_READ_SINGLE_BLOCK :	
	case MMC_CMD_READ_MULTIPLE_BLOCK :	
	case MMC_CMD_WRITE_SINGLE_BLOCK :	
	case MMC_CMD_WRITE_MULTIPLE_BLOCK :	
		BUG_ON(! data);
		Debug(">>>%s: sdc=0x%x, mem=0x%x, bqty=%d, bsz=%d\n", (data->flags == MMC_DATA_READ ? "READ" : "WRITE"), cmd->cmdarg, (uint32_t) data->dest, data->blocks, data->blocksize);
		res = (data->flags == MMC_DATA_READ ? sd_read_data : sd_write_data)(mmc, cmd->cmdarg, (u8 *) data->dest, data->blocks, data->blocksize);
		break;
		
	case MMC_CMD_STOP_TRANSMISSION :
		res = true;  // !!! no need		
		break;		
		
	default:
#ifdef DEBUG	
		((void (*) (void)) 0xfffc0178)();
#endif		
		res = proc_cmd(mmc, SDIO_CTRL_NODATA(cmd->cmdidx, resp, crc, idx, bw), cmd->cmdarg);
		break;
	}
	Debug(" %s", res ? "ok" : "error");
	
	if ( res && is_int_data_cmd ) {
		res = wait_tran_done_handle(mmc);
		if ( res ) {				
			u32 buf[256];
			BUG_ON(data_len > sizeof(buf));
			PREP_BUF(buf);
			res = buf2axi(mmc, 0, (u8*)&buf[0], data_len);
			Debug(">>>BUF: res=%d\n", res);
			print_buf(buf, data_len);
			memcpy(data->dest, buf, data_len);
		}
	}
	
	if ( ! res )
		ret = -EIO;

	rcm_mmc_read_response(mmc, cmd);
	
	// rcm_writel(mmc, SPISDIO_ENABLE, 0x0);  //sdio-on, spi-off

	return ret;
}

static const struct udevice_id rcm_mmc_match[] = {
	{ .compatible = "rcm,mmc-0.2" },
	{ }
};

static const struct dm_mmc_ops rcm_dm_mmc_ops = {
	.send_cmd = rcm_dm_mmc_send_cmd,
	.set_ios = rcm_dm_mmc_set_ios,
	.get_cd = rcm_dm_mmc_get_cd,
	.get_wp = rcm_dm_mmc_get_wp
};

U_BOOT_DRIVER(rcm_mmc_drv) = {
	.name = "rcm_mmc_drv",
	.id = UCLASS_MMC,
	.of_match = rcm_mmc_match,
	.ops = & rcm_dm_mmc_ops,
	.probe = rcm_mmc_probe,
	.bind = rcm_mmc_bind,
	.ofdata_to_platdata = rcm_mmc_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct rcm_mmc_platdata),
	.priv_auto_alloc_size = 1000
};
