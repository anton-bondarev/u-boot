#ifndef SYSREG_H_
#define SYSREG_H_

#define PL061_DATA_BASE  0x000
#define PL061_DATA       (PL061_DATA_BASE + (0xff << 2))
#define PL061_DIR        0x400
#define PL061_AFSEL      0x420

#define PL022_IMSC  0x014

#define SDIO_SDR_CARD_BLOCK_SET_REG  0x00
#define SDIO_SDR_CTRL_REG            0x04
#define SDIO_SDR_CMD_ARGUMENT_REG    0x08
#define SDIO_SDR_ADDRESS_REG         0x0C
#define SDIO_SDR_ERROR_ENABLE_REG    0x14
#define SDIO_SDR_BUF_TRAN_RESP_REG   0x48
#define SDIO_BUF_TRAN_CTRL_REG       0x50
#define SDIO_SDR_RESPONSE1_REG       0x18
#define SDIO_SDR_RESPONSE2_REG       0x1C
#define SDIO_SDR_RESPONSE3_REG       0x20
#define SDIO_SDR_RESPONSE4_REG       0x24

#define SDIO_DCCR_0                  0x28
#define SDIO_DCSSAR_0                0x2C
#define SDIO_DCDTR_0                 0x34

#define SDIO_DCCR_1                  0x38
#define SDIO_DCDSAR_1                0x40
#define SDIO_DCDTR_1                 0x44

#define SPISDIO_ENABLE           0x300
#define SPISDIO_SDIO_CLK_DIVIDE  0x304
#define SPISDIO_SDIO_INT_STATUS  0x308
#define SPISDIO_SDIO_INT_MASKS   0x30C


#define SPISDIO_SDIO_INT_STATUS_CH0_FINISH  0x01
#define SPISDIO_SDIO_INT_STATUS_CH1_FINISH  0x02
#define SPISDIO_SDIO_INT_STATUS_BUF_FINISH  0x04
#define SPISDIO_SDIO_INT_STATUS_CAR_ERR     0x40
#define SPISDIO_SDIO_INT_STATUS_CMD_DONE    0x20
#define SPISDIO_SDIO_INT_STATUS_TRAN_DONE   0x10

#define	SPISDIO_SPI_IRQMASKS  0x0D8


#endif /* SYSREG_H_ */
