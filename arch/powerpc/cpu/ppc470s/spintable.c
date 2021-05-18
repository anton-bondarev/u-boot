/*
 * Spintable processing for secondary core
 *
 * Copyright (C) 2018 AstroSoft
 *               Alexey Spirkov <alexeis@astrosoft.ru>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/tlb47x.h>

#define INITIAL_DELAY_US 1000

typedef struct {
    uint64_t entry_addr;
    uint64_t r3;
    uint32_t rsvd1;
    uint32_t pir;
} spintable_entry;


static uint32_t tcurrent(void){
  uint32_t value = 0;

  __asm volatile
  (
      "mfspr %0, %1 \n\t"
      :"=r"(value)
        :"i"(0x10C)
      :
  );

  return value;
}

static uint32_t ucurrent(void){
  return (tcurrent()/800);
}

typedef void (*kernel_start)(uint32_t p0);

static noinline int process_spintable_next(void)
{
    spintable_entry* spintable = (spintable_entry*) (RCM_PPC_SPL_SPINTABLE | 0x40000000);

    // invalidate IM0 at 0x0
    tlb47x_inval(0x00000000, TLBSID_1M);

    memset(spintable, 0, sizeof(spintable_entry));
    spintable->entry_addr = 0x1;

    // wait for entry_addr setup
    while(spintable->entry_addr == 0x1)
    {
        ppcDcbf((unsigned long) spintable);
        asm("sync");
    }
    asm("isync");

    tlb47x_map_coherent(spintable->entry_addr&(~0x0FFFFFFF), 0x00000000, TLBSID_256M, TLB_MODE_NONE, TLB_MODE_RWX);

    // never return from kernel
    ((kernel_start)(uint32_t)(spintable->entry_addr))((uint32_t)spintable->r3);

    return 0;
}

// Process required actions according ePAPR standard 5.5.2
int process_spintable (void)
{
    // if following delay will be skipped than greth0 hangs on boot 
    //   ToDo: evaluate why?
    //
    uint32_t start = ucurrent();
    while((ucurrent() - start) < INITIAL_DELAY_US);

    // map IM0 to 0x40000000 somewhere othere than 0
    tlb47x_map_nocache(0x01000000000, 0x40000000, TLBSID_1M, TLB_MODE_NONE, TLB_MODE_RWX);

    // switch context to new address
    asm("   mfmsr r0 \n\
            mtspr 0x01B, r0 \n\
            lis r0,3f@h \n\
            ori r0,r0,3f@l \n\
            oris r0, r0, 0x4000 \n\
            oris r1, r1, 0x4000 \n\
            mtspr 0x01A,r0 \n\
            sync \n\
            rfi \n\
3:     \
    ");
    process_spintable_next();    
    return 0;
}
