/*
 * Synopsys LSIF clock driver
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2019 Nadezhda Kharlamova <Nadezhda.Kharlamova@mir.dev>
 */

#include <common.h>
#include <clk-uclass.h>
#include <div64.h>
#include <dm.h>
#include <linux/io.h>

#define RCM_PLL_WR_LOCK 0x00C
#define RCM_PLL_UPD_CK 0x01C
#define RCM_PLL_DIVMODE_LSIF 0x100
#define RCM_PLL_CKEN_LSIF 0x104

#define RCM_PLL_WRUNLOCK 0x1ACCE551

#define CRG_DDR_BASE  0x38007000

#define LSIF0_CLKEN 0
#define LSIF1_CLKEN 1


static int read_reg(u32 reg) 
{
	u32 val;
	val = ioread32(CRG_DDR_BASE + reg);
	return val;
}

static void write_reg(u32 reg, u32 val) 
{
	iowrite32(val, CRG_DDR_BASE + reg);
}


static int lsif_clk_disable(struct clk *sclk)
{
	return 0;
}

static const struct clk_ops lsif_ops = {
	.disable = lsif_clk_disable,
};


static int lsif_clk_probe(struct udevice *dev)
{
	u32 val;
	u32 Fclk;
	int Divmode;
	struct clk c;
	int ret;
	ulong Fpll;

	// check WR_LOCK
	val = read_reg(RCM_PLL_WR_LOCK);
	if (val != 0){
		printf("Writing to registers is prohibited WR_LOCK 0x%x \n", val);
		write_reg(RCM_PLL_WR_LOCK, RCM_PLL_WRUNLOCK);
	}

	ofnode_read_u32(dev->node,"clock-frequency",&Fclk);
	ret = clk_get_by_index(dev, 0, &c);
	if (ret) {
		dev_err(dev, "clock not found, err=%d\n", ret);
		return -ENODEV;
	}

//	[0 bit] - clocking LSIF0 enabled (1)/disabled(0)
//	[1 bit] - clocking LSIF1 enabled (1)/disabled(0)

	if ((ofnode_read_bool(dev->node,"lsif0enabled")) && (ofnode_read_bool(dev->node,"lsif1enabled")))
		write_reg(RCM_PLL_CKEN_LSIF, 1 << LSIF0_CLKEN | 1 << LSIF1_CLKEN);

	else if (ofnode_read_bool(dev->node,"lsif0enabled"))
		write_reg(RCM_PLL_CKEN_LSIF, 1 << LSIF0_CLKEN);

	else if (ofnode_read_bool(dev->node,"lsif1enabled"))
		write_reg(RCM_PLL_CKEN_LSIF, 1 << LSIF1_CLKEN);

	Fpll = clk_get_rate(&c);

	// Fclk = Fpll / (DIVMODE +1)

	Divmode = (Fpll / Fclk) - 1;
	write_reg(RCM_PLL_DIVMODE_LSIF, (u32)Divmode);

	//apply settings
	write_reg(RCM_PLL_UPD_CK, 1);

	return 0;
}

static const struct udevice_id lsif_clk_id[] = {
	{ .compatible = "rcm,lsif-clock" },
	{ }
};

U_BOOT_DRIVER(lsif_clk) = {
	.name = "rcm,lsif-clock",
	.id = UCLASS_CLK,
	.of_match = lsif_clk_id,
	.probe = lsif_clk_probe,
	.ops = &lsif_ops,
};
