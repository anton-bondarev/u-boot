#
# SPDX-License-Identifier:	GPL-2.0+
#

dtb-$(CONFIG_TARGET_1888TX018) += mb115-01.dtb
targets += $(dtb-y)

DTC_FLAGS += -R 4 -p 0x1000

PHONY += dtbs
dtbs: $(addprefix $(obj)/, $(dtb-y))
	@:

clean-files := *.dtb
