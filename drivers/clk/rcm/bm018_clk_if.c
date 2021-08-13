/*
 *  CLK IF driver
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2019 Nadezhda Kharlamova <Nadezhda.Kharlamova@mir.dev>
 */

#include <clk-uclass.h>
#include <dm.h>
#include <linux/io.h>
#include <asm/dcr.h>

#define CRG_SYS_BASE  0x80080000

#define RCM_CKDIVMODE 0x120
#define RCM_CKUPDATE  0x060
#define RCM_WR_LOCK   0x03C

#define RCM_WR_LOCK_LOCKED 1
#define RCM_WRUNLOCK 0x1ACCE551

#define UPD_CKDIV_MASK (1 << 0)


struct bm018_clk_if_platdata{
	unsigned int Fclk;
	unsigned int Fcpu;
};

static u32 read_reg(u32 reg)
{
	u32 val;
	val = dcr_read(CRG_SYS_BASE  + reg);
	return val;
}

static void write_reg(u32 reg, u32 val) 
{
	dcr_write(CRG_SYS_BASE + reg, val);
}

static ulong clk_fixed_rate_get_rate(struct clk *clk)
{
	struct bm018_clk_if_platdata* bm018_platdata = dev_get_platdata(clk->dev);
	return (ulong)bm018_platdata->Fclk;
}

int bm018_ofdata_to_platdata(struct udevice *dev){
	struct clk c;
	int ret;

	struct bm018_clk_if_platdata* bm018_platdata = dev_get_platdata(dev);

	ret = ofnode_read_u32(dev->node,"clock-frequency",&bm018_platdata->Fclk);
	if (ret) {
		dev_err(dev, "clock-frequency not found, err=%d\n", ret);
		return -ENODEV;
	}

	ret = clk_get_by_index(dev, 0, &c);
	if (ret) {
		dev_err(dev, "clock not found, err=%d\n", ret);
		return -ENODEV;
	}

	bm018_platdata->Fcpu = clk_get_rate(&c);
	if (IS_ERR_VALUE(bm018_platdata->Fcpu)) {
		dev_err(dev,"Invalid bm018 cpu rate: \n");
		return -EINVAL;
	}

	return 0;
}
static const struct clk_ops clk_if_ops = {
	.get_rate = clk_fixed_rate_get_rate,
};

static int bm018_clk_if_probe(struct udevice *dev)
{
	u32 val;
	u32 divmode;
	bool locked;

	struct bm018_clk_if_platdata* bm018_platdata = dev_get_platdata(dev);


	// check WR_LOCK
	val = read_reg(RCM_WR_LOCK);

	locked = (val == RCM_WR_LOCK_LOCKED);
	if (locked){
		dev_dbg(dev,"Writing to registers is prohibited WR_LOCK \n");
		write_reg(RCM_WR_LOCK, RCM_WRUNLOCK);
	}

	// Fclk = Fcpu / (DIVMODE+1)

	divmode = (u32)(bm018_platdata->Fcpu / bm018_platdata->Fclk) - 1;
	write_reg(RCM_CKDIVMODE, divmode);

	//apply settings
	write_reg(RCM_CKUPDATE, UPD_CKDIV_MASK);

	// block WR_LOCK
	if(locked)
		write_reg(RCM_WR_LOCK, RCM_WR_LOCK_LOCKED);

	return 0;
}

static const struct udevice_id clk_if_id[] = {
	{ .compatible = "rcm,bm018_clk_if" },
	{ }
};

U_BOOT_DRIVER(bm018_clk_if) = {
	.name = "rcm,bm018_clk_if",
	.id = UCLASS_CLK,
	.of_match = clk_if_id,
	.probe = bm018_clk_if_probe,
	.ofdata_to_platdata = bm018_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct bm018_clk_if_platdata),
	.ops = &clk_if_ops,
};
