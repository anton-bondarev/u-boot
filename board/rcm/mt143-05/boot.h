#ifndef BOOTHEADER_H
#define BOOTHEADER_H

/**
 *
 * \defgroup bootheader Boot image handling
 * \ingroup libraries
 *
 * These functions implement checking and executing boot images
 * Platforms should implement their own bootheader.h with
 *
 * RUMBOOT_PLATFORM_CHIPID
 * RUMBOOT_PLATFORM_CHIPREV
 * RUMBOOT_PLATFORM_NUMCORES
 *
 * \code{.c}
 *
 * \endcode
 *
 * \addtogroup bootheader
 * @{
 */

#define RUMBOOT_HEADER_VERSION 2
#define RUMBOOT_HEADER_MAGIC 0xb01dface

#include "bootheader.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

struct rumboot_bootsource;

struct __attribute__((packed)) rumboot_bootheader {
    uint32_t magic;
    uint8_t  version;
    uint8_t  reserved;
    uint8_t  chip_id;
    uint8_t  chip_rev;
    uint32_t data_crc32;
    uint32_t datalen;
    uint32_t entry_point[10];
    uint32_t header_crc32;
    const struct rumboot_bootsource *device;
    char     data[];
};

#endif /* end of include guard: BOOTHEADER_H */
