/*
 *  Created on: Jul 7, 2015
 *      Author: a.korablinov
 */

#ifndef BOOTHEADER_H_
#define BOOTHEADER_H_

#define BOOTHEADER_MAGIC__BOOT_IMAGE_VALID    0xbeefc0de
#define BOOTHEADER_MAGIC__HOST_IMAGE_VALID    0xdeadc0de
#define BOOTHEADER_MAGIC__RESUME_IMAGE_VALID  0xdeadbabe
#define BOOTHEADER_MAGIC__CORERESET_IMAGE_VALID  0xdefec8ed

#define BOOTHEADER_OFFSET__MAGIC        0x00
#define BOOTHEADER_OFFSET__LENGTH       0x04
#define BOOTHEADER_OFFSET__ENTRY0       0x08
#define BOOTHEADER_OFFSET__ENTRY1       0x0c
#define BOOTHEADER_OFFSET__SUM          0x10
#define BOOTHEADER_OFFSET__HDRSUM       0x14
#define BOOTHEADER_OFFSET__IMAGEDATA    0x18

#define BOOTHEADER_ENTRY__MIN   (IM0__ + BOOTHEADER_OFFSET__IMAGEDATA)
#define BOOTHEADER_ENTRY__MAX   (IM0__ + IM0_SIZE - STACK_SIZE - 4)

#ifdef __ASSEMBLER__

#else //!__ASSEMBLER__

struct bootheader {
	uint32_t	magic;          /* Image type ID. Affects image processing.*/

	uint32_t	length;         /* imagedata length in bytes. Zero is valid value. */

	uint32_t	entry0;         /* Entry point address for primary CPU that executes ROM boot code.
	                                 * Address must be within the range
	                                 * from BOOTHEADER_ENTRY__MIN
	                                 * to BOOTHEADER_ENTRY__MAX */

	uint32_t	entry1;         /* Entry point address for secondary CPU.
	                                 * Address must be within the range
	                                 * from BOOTHEADER_ENTRY__MIN
	                                 * to BOOTHEADER_ENTRY__MAX.
	                                 * 0 if secondary CPU is not used. */

	uint32_t	sum;            /* CRC32 of imagedata. If length == 0 or sum == 0 then CRC32 is not checked. */

	uint32_t	hdrsum;         /* Checksum of all previous fields. */

	uint8_t		imagedata[];   /* Image data. */
} __attribute__((packed));


int bootimage_check_header(volatile struct bootheader *bhdr, uint32_t dstlen);
int bootimage_check_header_log(volatile struct bootheader *bhdr, const char *tag, uint32_t dstlen);
bool bootheader_magic_is_valid(uint32_t const magic);
const char *bootimage_strerr(int err);
bool bootimage_check_data(volatile struct bootheader* bhdr);
bool bootimage_check_data_log(volatile struct bootheader *bhdr, const char *tag);
bool bootimage_validate(volatile struct bootheader* bhdr);

struct bootsource {
	const char *name;
	int namelen;
    int bdatalen;
    bool (*init)(void* bdata);
	bool (*load_img)(void* bdata, volatile struct bootheader* dst, int dstlen);
    void (*deinit)(void* bdata);
};

bool bootsource_try_chain(const struct bootsource **src);
bool bootsource_try_single(const struct bootsource *src);
bool bootsource_unit_test(const struct bootsource *src, bool mustfail);


volatile struct bootheader *bootheader_get();
bool bootheader_entry_is_valid(uint32_t const entry);
void __attribute__((noreturn)) bootimage_execute(volatile struct bootheader *bhdr);
#endif  //__ASSEMBLER__

#endif  /* BOOTHEADER_H_ */
