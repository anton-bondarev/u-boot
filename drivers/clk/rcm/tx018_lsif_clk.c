/*
 *  LSIF clock driver
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

#define RCM_PLL_WR_LOCK 0x00C
#define RCM_PLL_UPD_CK 0x01C
#define RCM_PLL_DIVMODE_LSIF 0x100
#define RCM_PLL_CKEN_LSIF 0x104

#define RCM_PLL_WRUNLOCK 0x1ACCE551
#define RCM_PLL_WR_LOCK_LOCKED 1

#define CRG_DDR_BASE  0x38007000

#define LSIF0_CLKEN_MASK (1 << 0)
#define LSIF1_CLKEN_MASK (1 << 1)

#define UPD_CKEN_MASK (1 << 4)
#define UPD_CKDIV_MASK (1 << 0)

struct tx018_lsif_clk_platdata{
	unsigned int Fclk;
	unsigned int Fpll;
	bool lsif0enabled;
	bool lsif1enabled;
};

static u32 read_reg(u32 reg)
{
	u32 val;
	val = ioread32((const volatile void __iomem *)(CRG_DDR_BASE + reg));
	return val;
}

static void write_reg(u32 reg, u32 val) 
{
	iowrite32(val, (volatile void *)(CRG_DDR_BASE + reg));
}

static ulong clk_fixed_rate_get_rate(struct clk *clk)
{
	struct tx018_lsif_clk_platdata* tx018_platdata = dev_get_platdata(clk->dev);
	return (ulong)tx018_platdata->Fclk;
}

int tx018_ofdata_to_platdata(struct udevice *dev){
	struct clk c;
	int ret;

	struct tx018_lsif_clk_platdata* tx018_platdata = dev_get_platdata(dev);

	tx018_platdata->lsif0enabled = ofnode_read_bool(dev->node,"lsif0enabled");
	tx018_platdata->lsif1enabled = ofnode_read_bool(dev->node,"lsif1enabled");

	ret = ofnode_read_u32(dev->node,"clock-frequency",&tx018_platdata->Fclk);
	if (ret) {
		dev_err(dev, "clock-frequency not found, err=%d\n", ret);
		return -ENODEV;
	}

	ret = clk_get_by_index(dev, 0, &c);
	if (ret) {
		dev_err(dev, "clock not found, err=%d\n", ret);
		return -ENODEV;
	}

	tx018_platdata->Fpll = clk_get_rate(&c);
	if (IS_ERR_VALUE(tx018_platdata->Fpll)) {
			dev_err(dev, "Invalid tx018 lsif rate: 0\n");
			return -EINVAL;
		}

	return 0;
}
static const struct clk_ops lsif_ops = {
	.get_rate = clk_fixed_rate_get_rate,
};

static int lsif_clk_probe(struct udevice *dev)
{
	u32 val;
	u32 divmode;
	bool locked;

	// check WR_LOCK
	val = read_reg(RCM_PLL_WR_LOCK);

	struct tx018_lsif_clk_platdata* tx018_platdata = dev_get_platdata(dev);

	locked = (val == RCM_PLL_WR_LOCK_LOCKED);
	if (locked){
		dev_dbg(dev,"Writing to registers is prohibited WR_LOCK \n");
		write_reg(RCM_PLL_WR_LOCK, RCM_PLL_WRUNLOCK);
	}

//	[0 bit] - clocking LSIF0 enabled (1)/disabled(0)
//	[1 bit] - clocking LSIF1 enabled (1)/disabled(0)
	val = 0;
	if (tx018_platdata->lsif0enabled)
		val |= LSIF0_CLKEN_MASK;
	if (tx018_platdata->lsif1enabled)
		val |= LSIF1_CLKEN_MASK;

	write_reg(RCM_PLL_CKEN_LSIF, val);

	// Fclk = Fpll / (DIVMODE +1)
	divmode = (u32)(tx018_platdata->Fpll / tx018_platdata->Fclk) - 1;
	write_reg(RCM_PLL_DIVMODE_LSIF, divmode);

	//apply settings
	write_reg(RCM_PLL_UPD_CK, UPD_CKEN_MASK | UPD_CKDIV_MASK);

	// block WR_LOCK
	if(locked)
		write_reg(RCM_PLL_WR_LOCK, RCM_PLL_WR_LOCK_LOCKED);

	return 0;
}

static const struct udevice_id lsif_clk_id[] = {
	{ .compatible = "rcm,tx018_lsif_clk" },
	{ }
};

U_BOOT_DRIVER(tx018_lsif_clk) = {
	.name = "rcm,tx018_lsif_clk",
	.id = UCLASS_CLK,
	.of_match = lsif_clk_id,
	.probe = lsif_clk_probe,
	.ofdata_to_platdata = tx018_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct tx018_lsif_clk_platdata),
	.ops = &lsif_ops,
};
