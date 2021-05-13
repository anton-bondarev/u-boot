#ifndef TLB47X_H_
#define TLB47X_H_

#include <common.h>

typedef enum 
{
	TLBSID_4K   = 0x00,
	TLBSID_16K  = 0x01,
	TLBSID_64K  = 0x03,
	TLBSID_1M   = 0x07,
	TLBSID_16M  = 0x0F,
	TLBSID_256M = 0x1F,
	TLBSID_1G   = 0x3F,
	TLBSID_ERR  = 0xFF
} tlb_size_id;

typedef enum
{
  TLB_MODE_NONE = 0x0,
  TLB_MODE_R  = 0x1,
  TLB_MODE_W  = 0x2,
  TLB_MODE_RW = 0x3,
  TLB_MODE_X  = 0x4,
  TLB_MODE_RX = 0x5,
  TLB_MODE_WX  = 0x6,
  TLB_MODE_RWX = 0x7
} tlb_rwx_mode;

typedef struct 
{
  uint32_t epn   : 20;  // [0:19]
  uint32_t v     : 1;   // [20]
  uint32_t ts    : 1;   // [21]
  uint32_t dsiz  : 6;   // [22:27]  set 0x0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F (4K, 16K, 64K, 1M, 16M, 256M, 1GB) 
  uint32_t blank : 4;   // [28:31]
} tlb_entr_0;

typedef union
{
	tlb_entr_0 entr;
	uint32_t   data;
} tlb_entr_data_0;

typedef struct 
{
  uint32_t rpn   : 20;  // [0:19]
  uint32_t blank : 2;   // [20:21]
  uint32_t erpn  : 10;  // [22:31]
} tlb_entr_1;

typedef union
{
	tlb_entr_1 entr;
	uint32_t   data;
} tlb_entr_data_1;

typedef struct 
{
  uint32_t blank1 : 14;  // [0:13]
  uint32_t il1i   : 1;   // [14]
  uint32_t il1d   : 1;   // [15]
  uint32_t u      : 4;   // [16:19]
  uint32_t wimg   : 4;   // [20:23]
  uint32_t e      : 1;   // [24]  0-BE, 1-LE
  uint32_t blank2 : 1;   // [25]
  uint32_t uxwr   : 3;   // [26:28]
  uint32_t sxwr   : 3;   // [29:31]
} tlb_entr_2;

typedef union
{
	tlb_entr_2 entr;
	uint32_t   data;
} tlb_entr_data_2;


uint32_t _invalidate_tlb_entry(uint32_t tlb);
void _write_tlb_entry(uint32_t tlb0, uint32_t tlb1, uint32_t tlb2, uint32_t zero);
int _read_tlb_entry(uint32_t ea, uint32_t * tlb, uint32_t dummy);

void tlb47x_inval(uint32_t cpu_adr, tlb_size_id tlb_sid);

void tlb47x_map_nocache(uint64_t physical, uint32_t logical, tlb_size_id size, tlb_rwx_mode umode, tlb_rwx_mode smode);
void tlb47x_map_guarded(uint64_t physical, uint32_t logical, tlb_size_id size, tlb_rwx_mode umode, tlb_rwx_mode smode);
void tlb47x_map_cached(uint64_t physical, uint32_t logical, tlb_size_id size, tlb_rwx_mode umode, tlb_rwx_mode smode);
void tlb47x_map_coherent(uint64_t physical, uint32_t logical, tlb_size_id size, tlb_rwx_mode umode, tlb_rwx_mode smode);

#endif