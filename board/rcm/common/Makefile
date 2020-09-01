obj-y += flw_dev_list.o
ifdef CONFIG_SPL_BUILD
ifdef CONFIG_FLASHWRITER
obj-$(CONFIG_SPL_SPI_FLASH_SUPPORT) += flw_sf.o
obj-$(CONFIG_SPL_MMC_SUPPORT) += flw_mmc.o
obj-$(CONFIG_SPL_NOR_SUPPORT) += flw_nor.o
obj-$(CONFIG_SPL_NAND_SUPPORT) += flw_nand.o
obj-y += flw_serial.o
obj-y += flw_main.o
endif
endif
