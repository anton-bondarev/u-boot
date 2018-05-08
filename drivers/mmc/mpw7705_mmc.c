#define DEBUG
#define SDIO_DEBUG

#include <clk.h>
#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <libfdt.h>
#include <malloc.h>
#include <mmc.h>
#include <asm/io.h>

#include "mpw7705_sysreg.h"

DECLARE_GLOBAL_DATA_PTR;

struct mpw7705_mmc_platdata {
	struct mmc_config cfg;
	struct mmc mmc;
	uint32_t regbase;
};

static uint32_t ucurrent(void) 
{ 
	static uint32_t tick = 0;
	return tick ++; 
}

static bool isprintable(const char c)
{
	return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '!' && c <= '?') || c == ' ') ? true : false;
}

#define CMD17_CTRL          0x00117911

#define SDR_TRAN_FIFO_FINISH    0x00000001
#define SDR_TRAN_SDC_DAT_DONE   0x00000002
#define SDR_TRAN_SDC_CMD_DONE   0x00000004
#define SDR_TRAN_DAT_BOUNT      0x00000008
#define SDR_TRAN_SDC_ERR        0x00000010

#define DSSR_CHANNEL_TR_DONE    0x00080000
//--------------------------SDIO_INT------------------------

#define BLOCK_512_DATA_TRANS        0x00000101
#define BUFER_TRANS_START           0x04
#define DCCR_1_VAL                  0x00020F01

#define SDIO_TIMEOUT        2000000 //in microseconds

//-------------------------------------
static bool wait_cmd_done_handle(void) 
{
	uint32_t start = ucurrent();
	do {
		int status = read_SPISDIO_SDIO_INT_STATUS(SPISDIO__);
		if ( status & SPISDIO_SDIO_INT_STATUS_CAR_ERR ) {
			write_SDIO_SDR_BUF_TRAN_RESP_REG(SDIO__, SDR_TRAN_SDC_ERR);
			debug(" ECMD<0x%x>", status);
			return false;
		} else if ( status & SPISDIO_SDIO_INT_STATUS_CMD_DONE ) {
			write_SDIO_SDR_BUF_TRAN_RESP_REG(SDIO__, SDR_TRAN_SDC_CMD_DONE);
			return true;
		}
	} while ( ucurrent() - start < SDIO_TIMEOUT );

	debug("CMD ERROR: TIMEOUT\n");
	return false;
}

static bool wait_tran_done_handle(void)
{
	uint32_t start = ucurrent();
	do {
		int status = read_SPISDIO_SDIO_INT_STATUS(SPISDIO__);
		if ( status & SPISDIO_SDIO_INT_STATUS_CAR_ERR ) {
			write_SDIO_SDR_BUF_TRAN_RESP_REG(SDIO__, SDR_TRAN_SDC_ERR);
			debug(" ETRN<0x%x>", status);
			return false;
		} else if ( status & SPISDIO_SDIO_INT_STATUS_TRAN_DONE ) {
			write_SDIO_SDR_BUF_TRAN_RESP_REG(SDIO__, SDR_TRAN_SDC_DAT_DONE);
			return true;
		}
	} while ( ucurrent() - start < SDIO_TIMEOUT );

	debug("TRN ERROR: TIMEOUT\n");
	return false;
}

static bool wait_ch1_dma_done_handle(void)
{
	uint32_t start = ucurrent();
	do {
		int status = read_SPISDIO_SDIO_INT_STATUS(SPISDIO__);
		if ( status & SPISDIO_SDIO_INT_STATUS_CAR_ERR ) {
			write_SDIO_SDR_BUF_TRAN_RESP_REG(SDIO__, SDR_TRAN_SDC_ERR);
			debug(" EDMA<0x%x>", status);
			return false;
		} else if ( status & SPISDIO_SDIO_INT_STATUS_CH1_FINISH ) {
			write_SDIO_DCCR_1(SDIO__, DSSR_CHANNEL_TR_DONE);
			return true;
		}
	} while ( ucurrent() - start < SDIO_TIMEOUT );

	debug("DMA ERROR: TIMEOUT\n");
	return false;
}

static bool wait_buf_tran_finish_handle(void)
{
	uint32_t start = ucurrent();
	do {
		int status = read_SPISDIO_SDIO_INT_STATUS(SPISDIO__);
		if ( status & SPISDIO_SDIO_INT_STATUS_CAR_ERR ) {
			write_SDIO_SDR_BUF_TRAN_RESP_REG(SDIO__, SDR_TRAN_SDC_ERR);
			debug(" EBLK<0x%x>", status);
			return false;
		} else if ( status & SPISDIO_SDIO_INT_STATUS_BUF_FINISH ) {
			write_SDIO_SDR_BUF_TRAN_RESP_REG(SDIO__, SDR_TRAN_FIFO_FINISH);
			return true;
		}
	} while( ucurrent() - start < SDIO_TIMEOUT );

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

static bool sd_send_cmd(uint32_t cmd_ctrl, uint32_t arg)
{
#ifdef SDIO_DEBUG
	debug("SEND CMD_%d(%x)[%x]", cmd_ctrl >> 16, cmd_ctrl & 0xFFFF, arg);
#endif
	write_SDIO_SDR_CMD_ARGUMENT_REG(SDIO__, arg);
	write_SDIO_SDR_CTRL_REG(SDIO__, cmd_ctrl);
	bool result = wait_cmd_done_handle();
#ifdef SDIO_DEBUG
	int r1, r2, r3, r4;
	r1 = read_SDIO_SDR_RESPONSE1_REG(SDIO__);
	r2 = read_SDIO_SDR_RESPONSE2_REG(SDIO__);
	r3 = read_SDIO_SDR_RESPONSE3_REG(SDIO__);
	r4 = read_SDIO_SDR_RESPONSE4_REG(SDIO__);
	debug(" res=%s {0x%x, 0x%x, 0x%x, 0x%x}\n", (result ? "ok" : "err"), r1, r2, r3, r4);
#endif
	return result;
}
////////////////////////////////////////////////////////////////

#define SD_CARD_DETECT_GPIO_BASE  GPIO1__
#define SD_CARD_DETECT_GPIO_PIN  (1 << 1)

bool check_sd_card_present(void)
{
    //set software control
    uint8_t afsel;
    afsel = read_PL061_AFSEL(SD_CARD_DETECT_GPIO_BASE);
    afsel &= ~SD_CARD_DETECT_GPIO_PIN;
    write_PL061_AFSEL(SD_CARD_DETECT_GPIO_BASE, afsel);

    //set SD detect bit direction to input
    uint8_t dir;
    dir = read_PL061_DIR(SD_CARD_DETECT_GPIO_BASE);
    dir &= ~SD_CARD_DETECT_GPIO_PIN;
    write_PL061_DIR(SD_CARD_DETECT_GPIO_BASE, dir);

    if (read_PL061_DATA(SD_CARD_DETECT_GPIO_BASE) & SD_CARD_DETECT_GPIO_PIN)
        return false;

    return true;
}

static void enable_gpio_for_SDIO(void)
{
    uint8_t afsel;
    //http://svn.module.ru/r42_mm7705/mm7705/trunk/ifsys/units/lsif1/doc/LSIF1-pinout.xlsx
    afsel = read_PL061_AFSEL(LSIF1_MGPIO4__);
    afsel |= 0b11111000;
    write_PL061_AFSEL(LSIF1_MGPIO4__, afsel);
}

#define SDIO_CLK_DIV_400KHz   124
#define SDIO_CLK_DIV_10MHz    4
#define SDIO_CLK_DIV_50MHz    0

bool sd_init(void)
{
    if (!check_sd_card_present())
    {
        debug("SD: No card inserted\n");
        return false;
    }

    enable_gpio_for_SDIO();

    write_SPISDIO_SDIO_CLK_DIVIDE(SPISDIO__, SDIO_CLK_DIV_10MHz);

    write_SPISDIO_ENABLE(SPISDIO__, 0x1);//sdio-on, spi-off

    /* Hard reset damned hardware */
    write_SDIO_SDR_CTRL_REG(SDIO__, (1<<7));
    while (read_SDIO_SDR_CTRL_REG(SDIO__) & (1<<7));;;

    write_PL022_IMSC(GSPI__, 0x0);//disable interrupts in SSP

    write_SPISDIO_SDIO_INT_MASKS(SPISDIO__, 0x7F);//enable  interrupts in SDIO

    write_SPISDIO_SPI_IRQMASKS(SPISDIO__, 0x00);//disable interrupts from ssp

    write_SDIO_SDR_ERROR_ENABLE_REG(SDIO__, 0x16F);//enable interrupt and error flag

    return true;
}

//----------------------------
static bool SD2buf(int buf_num, int idx, uint len)
{
	write_SDIO_SDR_ADDRESS_REG(SDIO__, (buf_num << 2));
	write_SDIO_SDR_CARD_BLOCK_SET_REG(SDIO__, (len == 512 ? BLOCK_512_DATA_TRANS : (len << 16) | 0x0001));
	write_SDIO_SDR_CMD_ARGUMENT_REG(SDIO__, idx);
	write_SDIO_SDR_CTRL_REG(SDIO__, CMD17_CTRL);

	if ( ! wait_cmd_done_handle() ) {
		debug("TIMEOUT:CMD DONE at function SD2buf\n");
		return false;
	}
	if ( ! wait_tran_done_handle() ) {
		debug("TIMEOUT:TRANSFER DONE at function SD2buf\n");
		return false;
	}
	return true;
}
//----------------------------
//dma_addr is a physical address
//Hi [35:32] must be set through LSIF1 reg
static bool buf2axi(int buf_num, int dma_addr, uint len)
{
	write_SDIO_BUF_TRAN_CTRL_REG(SDIO__, BUFER_TRANS_START | buf_num);
	write_SDIO_DCDTR_1(SDIO__, len);
	write_SDIO_DCDSAR_1(SDIO__, dma_addr);
	write_SDIO_DCCR_1(SDIO__, DCCR_1_VAL);

	if ( ! wait_ch1_dma_done_handle() ) {
		debug("TIMEOUT:DMA DONE at function buf2axi.\n");
		return false;
	}
	if ( ! wait_buf_tran_finish_handle() ) {
		debug("TIMEOUT:BUF TRAN FINISH at function buf2axi.\n");
		return false;
	}
	return true;
}
//------------------------------------------
//dest_addr is a physical address
//Hi [35:32] must be set through LSIF1 reg
static bool sd_read_block(uint32_t block, uint32_t dest_addr, uint len)
{
	if ( ! SD2buf(0, block, len) )
		return false;
	
	if ( ! buf2axi(0, dest_addr, len) )
		return false;

	return true;
}
//-

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

static void mpw7705_writel(uint32_t adr, u32 val, int reg)
{
	mpw7705_print_op("mpw7705_writeL", adr, reg, val);
	writel(val, adr + reg);
}
static void mpw7705_writew(uint32_t adr, u16 val, int reg)
{
	mpw7705_print_op("mpw7705_writeW", adr, reg, val);
	writew(val, adr + reg);
}
static void mpw7705_writeb(uint32_t adr, u8 val, int reg)
{
	mpw7705_print_op("mpw7705_writeB", adr, reg, val);
	writeb(val, adr + reg);
}

static u32 mpw7705_readl(uint32_t adr, int reg)
{
	u32 retval = readl(adr + reg);
	mpw7705_print_op("mpw7705_readL", adr, reg, retval);
	return retval;
}
static u16 mpw7705_sdhci_readw(uint32_t adr, int reg)
{
	u16 retval = readw(adr + reg);
	mpw7705_print_op("mpw7705_readW", adr, reg, retval);
	return retval;
}
static u8 mpw7705_readb(uint32_t adr, int reg)
{
	u8 retval = readb(adr + reg);
	mpw7705_print_op("mpw7705_readB", adr, reg, retval);
	return retval;
}

////////////////////////////////////////////////////////////////

#define PREP_BUF(buf)  prep_buf((buf), sizeof(buf))
static void prep_buf(u8 * buf, uint size)
{
	uint i;
	for ( i = 0; i < size; ++ i )
		buf[i] = i;
}

#define PRINT_BUF(buf)  print_buf((buf), sizeof(buf))
static void print_buf(const u8 * buf, uint size)
{
	uint len = (size < 512 ? size : 512);
	debug("buf[%d]=", size);
	while ( len -- ) {
		char c = * buf ++;
		if ( isprintable(c) ) debug("_%c", c);		
		else                  debug("%02x", c);
	}
	debug("\n");
}

static void test_read(void)
{
	static u8 block[512];
	uint32_t adr = 0x11800;
	uint cnt;

	for ( cnt = 0; cnt < 1; ++ cnt ) {
		PREP_BUF(block);
		debug("Read SD card (adr=%d) ... ", adr);		
		bool res = sd_read_block(adr, (uint32_t) block, sizeof(block));
		debug("%s\n", res ? "ok" : "fail");
		PRINT_BUF(block);
			
		adr += 50000000;
	}
	
	//((void (*) (void)) 0xfffc0178)();
}

static int mpw7705_mmc_probe(struct udevice *dev)
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

	mmc_set_clock(mmc, cfg->f_min);
	
	bool res = sd_init();
	if ( ! res )
		return -1;

	return 0;
}

static int mpw7705_mmc_ofdata_to_platdata(struct udevice *dev)
{
	debug(">mpw7705_mmc_ofdata_to_platdata\n");
	
	debug("dev: %x\n", (int) dev);
	
	struct mpw7705_mmc_platdata * pdata = dev_get_platdata(dev);
	debug("plat: %x\n", (int) pdata);
	
	fdt_addr_t addr = devfdt_get_addr(dev);
	
	debug("addr(1): %x\n", (int) addr);
	if ( addr == FDT_ADDR_T_NONE ) 
		addr = 0x3C06A000;
	debug("addr(2): %x\n", (int) addr);

	pdata->regbase = addr;

	return 0;
}

int mpw7705_mmc_bind(struct udevice *dev)
{
	debug(">mpw7705_mmc_bind\n");
	
	struct mpw7705_mmc_platdata * pdata = dev_get_platdata(dev);
	return mmc_bind(dev, & pdata->mmc, & pdata->cfg);
}

static int mpw7705_dm_mmc_set_ios(struct udevice *dev)
{
	debug(">mpw7705_dm_mmc_set_ios\n");
	
	return 0;
}

static void mpw7705_mmc_read_response(struct mmc *mmc, struct mmc_cmd *cmd)
{
	if (cmd->resp_type & MMC_RSP_136) {
		cmd->response[0] = read_SDIO_SDR_RESPONSE4_REG(SDIO__);
		cmd->response[1] = read_SDIO_SDR_RESPONSE3_REG(SDIO__);
		cmd->response[2] = read_SDIO_SDR_RESPONSE2_REG(SDIO__);
		cmd->response[3] = read_SDIO_SDR_RESPONSE1_REG(SDIO__);
	} else {
		cmd->response[0] = read_SDIO_SDR_RESPONSE1_REG(SDIO__);
	}
}

static bool proc_cmd(uint code, uint arg)
{
	uint attempt = 5;
	while ( attempt -- ) {
		if ( sd_send_cmd(code, arg) )
			return true;
		debug("+");
		uint delay;
		for ( delay = 0; delay < 100000; ++ delay )
			;
	}
	return false;
}

static int mpw7705_dm_mmc_send_cmd(struct udevice *dev, struct mmc_cmd *cmd, struct mmc_data *data)
{
	struct mmc * mmc = mmc_get_mmc_dev(dev);
	struct meson_mmc_platdata * pdata = mmc->priv;
	uint32_t status;
	ulong start;
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
		res = proc_cmd(SDIO_CTRL_NODATA(0, 0, 0, 0), cmd->cmdarg);
		break;
	case MMC_CMD_SEND_EXT_CSD :
		res = proc_cmd(SDIO_CTRL_NODATA(8, SDIO_RESPONSE_R1367, crc, idx), cmd->cmdarg);
		break;
	case MMC_CMD_APP_CMD :
		res = proc_cmd(SDIO_CTRL_NODATA(55, SDIO_RESPONSE_R1367, crc, idx), cmd->cmdarg);
		break;
	case SD_CMD_APP_SEND_OP_COND :
		res = proc_cmd(SDIO_CTRL_NODATA(41, resp, crc, idx), cmd->cmdarg);
		break;
	case MMC_CMD_ALL_SEND_CID :
		res = proc_cmd(SDIO_CTRL_NODATA(2, SDIO_RESPONSE_R2, crc, idx), cmd->cmdarg);
		break;		
	case MMC_CMD_SET_RELATIVE_ADDR :
		res = proc_cmd(SDIO_CTRL_NODATA(3, SDIO_RESPONSE_R1367, crc, idx), cmd->cmdarg);
		break;
	case MMC_CMD_SEND_CSD :
		res = proc_cmd(SDIO_CTRL_NODATA(9, SDIO_RESPONSE_R1367, crc, idx), cmd->cmdarg);
		break;
	case MMC_CMD_SELECT_CARD :
		res = proc_cmd(SDIO_CTRL_NODATA(7, SDIO_RESPONSE_R1367, crc, idx), cmd->cmdarg);
		break;
	case SD_CMD_APP_SET_BUS_WIDTH :
		res = proc_cmd(SDIO_CTRL_NODATA(6, SDIO_RESPONSE_R1367, crc, idx), cmd->cmdarg);
		break;
	case MMC_CMD_SET_BLOCKLEN :
		res = proc_cmd(SDIO_CTRL_NODATA(16, SDIO_RESPONSE_R1367, crc, idx), 512/*cmd->cmdarg*/);  //!!!
		break;		
		
	case SD_CMD_APP_SEND_SCR :
		{
			uint len = data->blocks * data->blocksize;
			u8 buf[512];
			PREP_BUF(buf);
			uint attempt;
			for ( attempt = 0; attempt < 5; ++ attempt ) {
				//res = true;
				write_SDIO_SDR_ADDRESS_REG(SDIO__, (0 << 2));
				write_SDIO_SDR_CARD_BLOCK_SET_REG(SDIO__, (len << 16) | 0x01);
				resp = SDIO_RESPONSE_R1367;
				uint32_t cmd_code = (51 << 16) | 0x7811;				
				res = sd_send_cmd(cmd_code, cmd->cmdarg);
				if ( res )
					res = wait_tran_done_handle();
				if ( res )
					break;
				debug("+");
				uint delay;
				for ( delay = 0; delay < 100000; ++ delay )
					;
			}
			if ( res ) {								
				res = buf2axi(0, (uint32_t) buf, len);
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
				write_SDIO_SDR_ADDRESS_REG(SDIO__, (0 << 2));
				write_SDIO_SDR_CARD_BLOCK_SET_REG(SDIO__, (len << 16) | 0x01);
				resp = SDIO_RESPONSE_R1367;
				uint32_t cmd_code = (13 << 16) | 0x7811;				
				res = sd_send_cmd(cmd_code, cmd->cmdarg);
				if ( res )
					res = wait_tran_done_handle();
				if ( res )
					break;
				debug("+");
				uint delay;
				for ( delay = 0; delay < 100000; ++ delay )
					;
			}
			if ( res ) {								
				u8 buf[512];
				prep_buf(buf, len);
				res = buf2axi(0, (uint32_t) buf, len);
				debug(">>>BUF: res=%d\n", res);
				print_buf(buf, len);
				if ( res )
					memcpy(data->dest, buf, data->blocks * data->blocksize);
			}
		}
		break;
		
	case MMC_CMD_STOP_TRANSMISSION :
		res = true;
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
					debug(">>>%d ", blk_qty);
					res = sd_read_block(src_adr, (uint32_t) dst_ptr, 512/*data->blocksize*/);  //!!!
					if ( ! res )
						break;
					src_adr += 512/*data->blocksize*/;  //!!!
					dst_ptr += 512/*data->blocksize*/;  //!!!
					-- blk_qty;
				}
				debug("\n");
				break;
			}
		}
		((void (*) (void)) 0xfffc0178)();
		// nobreak
		
	default:
		res = proc_cmd(SDIO_CTRL_NODATA(cmd->cmdidx, resp, crc, idx), cmd->cmdarg);
		break;
	}
	debug("%s\n", res ? "ok" : "error");
	
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
