#include <common.h>
//#include <console.h>
//#include <bootretry.h>
//#include <cli.h>
//#include <command.h>
//#include <console.h>
//#include <hash.h>
//#include <inttypes.h>
//#include <mapmem.h>
//#include <watchdog.h>
//#include <asm/io.h>
//#include <linux/compiler.h>


#ifdef CONFIG_CMD_MEMMAP

extern uint32_t _invalidate_tlb_entry(uint32_t tlb);
extern void _write_tlb_entry(uint32_t tlb0, uint32_t tlb1, uint32_t tlb2, uint32_t zero);
extern int _read_tlb_entry(uint32_t ea, uint32_t * tlb, uint32_t dummy);

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
} TLB_SIZE_ID;

////////////////////////////////////////////////////////////////

typedef struct 
{
	TLB_SIZE_ID sid;
	const char * name;
} tlb_sid_name;

static const tlb_sid_name tlb_sid_names[] = 
{
	{ TLBSID_4K,   "4k"   },
	{ TLBSID_16K,  "16k"  },
	{ TLBSID_64K,  "64k"  },
	{ TLBSID_1M,   "1m"   },
	{ TLBSID_16M,  "16m"  },
	{ TLBSID_256M, "256m" },
	{ TLBSID_1G,   "1g"   },
	{ TLBSID_ERR,  ""     }
};	

static TLB_SIZE_ID get_tlb_sid_by_name(const char * name)
{
	uint i;
	for ( i = 0; tlb_sid_names[i].sid != TLBSID_ERR; ++ i ) 
		if ( strcmp(name, tlb_sid_names[i].name) == 0 )
			break;
	return tlb_sid_names[i].sid;
}

static const char * get_tlb_sid_name(TLB_SIZE_ID tlb_sid)
{
	uint i;
	for ( i = 0; tlb_sid_names[i].sid != TLBSID_ERR; ++ i ) 
		if ( tlb_sid_names[i].sid == tlb_sid )
			break;
	return tlb_sid_names[i].name;
}

static const uint get_tlb_sid_size(TLB_SIZE_ID tlb_sid)
{
	switch ( tlb_sid ) {
	case TLBSID_4K :   return 4 * 1024;
	case TLBSID_16K :  return 16 * 1024;
	case TLBSID_64K :  return 64 * 1024;
	case TLBSID_1M :   return 1 * 1024 * 1024;
	case TLBSID_16M :  return 16 * 1024 * 1024;
	case TLBSID_256M : return 256 * 1024 * 1024;
	case TLBSID_1G :   return 1 * 1024 * 1024 * 1024;
	default :
		break;		
	}
	return 0;
}

////////////////////////////////////////////////////////////////

/* Current:
  epn  v ts  sz   _  rpn  _ erpn  __  ili ild u wimg  e  _ uxwr sxwr
 00000 1  0   1m  f 00000 2  010 2000  1   1  0   4  BE  0   0    7
 10000 1  0 256m  b 10000 0  010 0000  1   1  0   5  LE  0   0    3
 20000 1  0 256m  b 20000 0  010 0000  1   1  0   5  LE  0   0    3
 30000 1  0 256m  f 30000 2  010 0000  1   1  0   5  LE  0   0    3
 40000 1  0 256m  3 00000 3  000 0000  1   1  0   4  BE  0   7    7
 fff00 1  0   1m  f fff00 3  3ff 2000  1   1  0   5  BE  0   0    5
*/

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

static void print_tlb_entr_0(tlb_entr_0 * tlb, bool header)
{
	if ( ! header ) printf(" %05x %d  %d %4s  %01x", tlb->epn, tlb->v, tlb->ts, get_tlb_sid_name((TLB_SIZE_ID) tlb->dsiz), tlb->blank);
	else            printf("  epn  v ts  sz   _");
}

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

static void print_tlb_entr_1(tlb_entr_1 * tlb, bool header)
{
	if ( ! header ) printf(" %05x %01x  %03x", tlb->rpn, tlb->blank, tlb->erpn);
	else            printf("  rpn  _ erpn"); 
}

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

static void print_tlb_entr_2(tlb_entr_2 * tlb, bool header)
{
	if ( ! header ) 
		printf(" %04x  %d   %d  %01x   %01x  %s  %d   %01x    %01x", 
		       tlb->blank1, tlb->il1i, tlb->il1d, tlb->u, tlb->wimg, (tlb->e ? "LE" : "BE"), tlb->blank2, tlb->uxwr, tlb->sxwr);
	else
		printf("  __  ili ild u wimg  e  _ uxwr sxwr");
}

typedef struct 
{
	uint32_t tlb_0;
	uint32_t tlb_1;
	uint32_t tlb_2;
} tlb_entr;

static tlb_entr s_tlb_arr[1024];
static int s_tlb_cnt = -1;

#define countof(arr)  (sizeof(arr) / sizeof((arr)[0]))

static int mem_map_tlbs(void) 
{
	print_tlb_entr_0(NULL, true);
	print_tlb_entr_1(NULL, true);
	print_tlb_entr_2(NULL, true);
	printf("\n");
	
	s_tlb_cnt = -1;
	uint32_t ea;
	const uint32_t dlt = 0x1 << 12;
	for ( ea = 0x00000000; ea + dlt > ea; ea += dlt ) {
		uint32_t tlb[3] = { 0, 0, 0 };	
		uint32_t res = _read_tlb_entry(ea, tlb, 0);
		if ( res != 0 && (s_tlb_cnt == -1 || s_tlb_arr[s_tlb_cnt].tlb_0 != tlb[0]) ) {
			if ( ++ s_tlb_cnt == countof(s_tlb_arr) )
				return -1;
			s_tlb_arr[s_tlb_cnt].tlb_0 = tlb[0];
			s_tlb_arr[s_tlb_cnt].tlb_1 = tlb[1];
			s_tlb_arr[s_tlb_cnt].tlb_2 = tlb[2];
			print_tlb_entr_0((tlb_entr_0 *) & tlb[0], false);
			print_tlb_entr_1((tlb_entr_1 *) & tlb[1], false);
			print_tlb_entr_2((tlb_entr_2 *) & tlb[2], false);
			printf("\n");
		}			
	}
	return 0;
}

////////////////////////////////////////////////////////////////

typedef struct 
{
	uint32_t phys_adr;
	uint32_t cpu_adr;
	TLB_SIZE_ID sid;
} map_entr;

#define MEM_MAPS_QTY  50

static map_entr s_maps[MEM_MAPS_QTY];
static bool s_maps_ready = false;

////////////////////////////////////////////////////////////////

static void mem_map_init(void)
{
	uint map_ind;
	for ( map_ind = 0; map_ind < countof(s_maps); ++ map_ind ) 
		s_maps[map_ind].sid = TLBSID_ERR;
	s_maps_ready = true;
}
		
static int mem_map_list(void)
{
	printf("Maps: \n");
	uint map_ind;
	for ( map_ind = 0; map_ind < countof(s_maps); ++ map_ind ) {
		if ( s_maps[map_ind].sid == TLBSID_ERR ) 
			continue;
		printf("  Phys=%08x, Cpu=%08x, Size=%s\n", s_maps[map_ind].phys_adr, s_maps[map_ind].cpu_adr, get_tlb_sid_name(s_maps[map_ind].sid));
	}
	return 0;
}

static int find_map_entr(uint32_t cpu_adr)
{
	uint map_ind;
	for ( map_ind = 0; map_ind < countof(s_maps); ++ map_ind ) {
		if ( s_maps[map_ind].sid == TLBSID_ERR ) 
			continue;		
		if ( s_maps[map_ind].cpu_adr == cpu_adr ) 
			return map_ind;
	}
	return -1;
}

static int find_empty_map_entr(void)
{
	uint map_ind;
	for ( map_ind = 0; map_ind < countof(s_maps); ++ map_ind ) 
		if ( s_maps[map_ind].sid == TLBSID_ERR ) 
			return map_ind;
	return -1;
}

static void mem_map_inval(uint32_t cpu_adr, TLB_SIZE_ID tlb_sid)
{
	tlb_entr_data_0 tlb_0;
	tlb_0.data = 0x00000000;
	tlb_0.entr.epn = (cpu_adr >> 12);
	tlb_0.entr.v = 0;
	tlb_0.entr.ts = 0;
	tlb_0.entr.dsiz = tlb_sid;
		
	_invalidate_tlb_entry(tlb_0.data);
}

static int mem_map_drop(uint32_t cpu_adr, TLB_SIZE_ID tlb_sid)
{
	int map_ind = -1;
	
	if ( tlb_sid == TLBSID_ERR ) {
		map_ind = find_map_entr(cpu_adr);
		if ( map_ind == -1 ) {
			printf("Map not found\n");
			return 0;
		}
		tlb_sid = s_maps[map_ind].sid;
	}
	
	mem_map_inval(cpu_adr, tlb_sid);
	
	if ( map_ind != -1 ) 
		s_maps[map_ind].sid = TLBSID_ERR;
	
	return 0;
}

static int mem_map_drop_all(void)
{
	uint map_ind;
	for ( map_ind = 0; map_ind < countof(s_maps); ++ map_ind ) {
		if ( s_maps[map_ind].sid == TLBSID_ERR ) 
			continue;		

		mem_map_inval(s_maps[map_ind].cpu_adr, s_maps[map_ind].sid);
		
		s_maps[map_ind].sid = TLBSID_ERR;
	}
	return 0;
}

static int mem_map_set(uint32_t phys_adr, uint32_t cpu_adr, TLB_SIZE_ID tlb_sid)
{
	int map_ind;
	
	map_ind = find_map_entr(cpu_adr);
	if ( map_ind != -1 ) {
		printf("Map exists\n");
		return 0;	
	}
	
	map_ind = find_empty_map_entr();
	if ( map_ind == -1 ) {
		printf("Maps full\n");
		return 0;	
	}

	tlb_entr_data_0 tlb_0;
	tlb_0.data = 0x00000000;
  tlb_0.entr.epn = (cpu_adr >> 12);
  tlb_0.entr.v = 1;
  tlb_0.entr.ts = 0;
  tlb_0.entr.dsiz = tlb_sid;

	tlb_entr_data_1 tlb_1;
	tlb_1.data = 0x00000000;
  tlb_1.entr.rpn = (phys_adr >> 12);
  tlb_1.entr.erpn = 0x000;

	tlb_entr_data_2 tlb_2;
	tlb_2.data = 0x00000000;
  tlb_2.entr.il1i = 1;
  tlb_2.entr.il1d = 1;
  tlb_2.entr.u = 0;
  tlb_2.entr.wimg = 0x4;
  tlb_2.entr.e = 0;  // BE   
  tlb_2.entr.uxwr = 0x7;     
  tlb_2.entr.sxwr = 0x7;     
	
	_write_tlb_entry(tlb_0.data, tlb_1.data, tlb_2.data, 0); 
	
	s_maps[map_ind].phys_adr = phys_adr;
	s_maps[map_ind].cpu_adr = cpu_adr;
	s_maps[map_ind].sid = tlb_sid;
	
	return 0;
}

static int mem_map_test(bool read_flag)
{
	int retcode = 0;
	uint map_ind;
	for ( map_ind = 0; map_ind < countof(s_maps); ++ map_ind ) {
		if ( s_maps[map_ind].sid == TLBSID_ERR ) 
			continue;
		uint32_t len = get_tlb_sid_size(s_maps[map_ind].sid) / sizeof(uint32_t);
		uint32_t pat = s_maps[map_ind].phys_adr;
		uint32_t * ptr = (uint32_t *) s_maps[map_ind].cpu_adr;
		bool is_ok = true;
		while ( len -- ) {
			if ( read_flag ) {
				if ( * ptr != pat ) {
					is_ok = false;
					retcode = -1;
					break;
				}
				++ ptr;
			} else
				* ptr ++ = pat;
			pat += sizeof(uint32_t);
		}
		if ( read_flag ) {
			printf("[%08x:%08x] : ", s_maps[map_ind].phys_adr, s_maps[map_ind].phys_adr + (get_tlb_sid_size(s_maps[map_ind].sid) - 1));
			if ( is_ok ) printf("ok\n");
			else         printf("failed([%08x] = %08x)\n", pat, * ptr);			
		} else
			printf(".");						
	}
	return retcode;
}

////////////////////////////////////////////////////////////////

static char * s_sub_argv_1[][3] = 
{
	{ "C0000000", "1m", "10000000" },
	{ "C0100000", "1m", "50000000" },
	{ "C0200000", "1m", "90000000" },
	{ "C0300000", "1m", "D0000000" },
	{ NULL, NULL, NULL }
};

static char * s_sub_argv_2[][3] = 
{
	{ "00000000", "256m", "10000000" },
	{ "C0000000", "256m", "10000000" },
	{ NULL, NULL, NULL }
};

static int do_mem_map_set(int argc, char * const argv[])
{
	if ( argc == 1 ) {
		static char * (* s_sub_argv)[3];		
		if      ( strcmp(argv[0], "1") == 0 ) s_sub_argv = s_sub_argv_1;
		else if ( strcmp(argv[0], "2") == 0 ) s_sub_argv = s_sub_argv_2;
		else 
			return CMD_RET_USAGE;
		
		uint sub_arg_ind;
		for ( sub_arg_ind = 0; !! s_sub_argv[sub_arg_ind][0]; ++ sub_arg_ind ) {			
			int retcode = do_mem_map_set(countof(s_sub_argv[sub_arg_ind]), s_sub_argv[sub_arg_ind]);
			if ( retcode != 0 ) 
				return retcode;
		}
		return mem_map_list();
	}

	if ( argc == 3 ) {
		ulong cpu_adr;
		if ( strict_strtoul(argv[0], 16, & cpu_adr) < 0 ) {
			printf("Bad cpu_adr\n");
			return CMD_RET_USAGE;
		}
		
		TLB_SIZE_ID tlb_sid = get_tlb_sid_by_name(argv[1]);
		if ( tlb_sid == TLBSID_ERR ) {
			printf("Bad size\n");
			return CMD_RET_USAGE;
		}
		
		ulong phys_adr;
		if ( strict_strtoul(argv[2], 16, & phys_adr) < 0 ) {
			printf("Bad phys_adr\n");
			return CMD_RET_USAGE;
		}
		
		return mem_map_set(phys_adr, cpu_adr, tlb_sid);
	}

	return CMD_RET_USAGE;
}

static int do_mem_map_drop(int argc, char * const argv[])
{
	if ( argc < 1 ) 
		return CMD_RET_USAGE;
	
	if ( strcmp(argv[0], "all") == 0 ) 
		return mem_map_drop_all();
			
	ulong cpu_adr;
	if ( strict_strtoul(argv[0], 16, & cpu_adr) < 0 ) {
		printf("Bad cpu_adr\n");
		return CMD_RET_USAGE;
	}
	
	TLB_SIZE_ID tlb_sid = TLBSID_ERR;	
	if ( argc == 2 ) {
		tlb_sid = get_tlb_sid_by_name(argv[1]);
		if ( tlb_sid == TLBSID_ERR ) {
			printf("Bad size\n");
			return CMD_RET_USAGE;
		}
	}
		
	return mem_map_drop(cpu_adr, tlb_sid);
}

static int do_mem_map_list(int argc, char * const argv[])
{
	return (argc == 0 ? mem_map_list() : CMD_RET_USAGE);
}

static int do_mem_map_tlbs(int argc, char * const argv[])
{
	return (argc == 0 ? mem_map_tlbs() : CMD_RET_USAGE);
}

static int do_mem_map_test(int argc, char * const argv[])
{
	if ( argc != 0 ) 
		return CMD_RET_USAGE;
	
	int retcode; 
	printf("Writing");	
	retcode = mem_map_test(false);
	if ( retcode == 0 ) {
		printf(" Reading:\n");			
		retcode = mem_map_test(true);
	}
	printf(retcode ? "Test failed\n" : "All right\n");	
	return 0;
}

static int do_mem_map_help(int argc, char * const argv[])
{
	if ( argc != 1 || strcmp(argv[0], "set") == 0 ) {
		printf("mmap set cpu_adr { 4k | 16k | 64k | 1m | 16m | 256m | 1g } phys_adr\n");		
		printf("mmap set { 1 | 2 }\n");		
	}
	if ( argc != 1 || strcmp(argv[0], "drop") == 0 ) 
		printf("mmap drop { cpu_adr [size_id] | all }\n");
	if ( argc != 1 || strcmp(argv[0], "list") == 0 ) 
		printf("mmap list\n");
	if ( argc != 1 || strcmp(argv[0], "tlb") == 0 ) 
		printf("mmap tlb\n");
		
	return 0;
}

/*
 * Perform a memory mapping.
 */
static int do_mem_map(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	if ( ! s_maps_ready )
		mem_map_init();
		
	if ( argc >= 2 ) {
		if ( strcmp(argv[1], "help") == 0 ) 
			return do_mem_map_help(argc - 2, & argv[2]);		
		if ( strcmp(argv[1], "set") == 0 )
			return do_mem_map_set(argc - 2, & argv[2]);
		if ( strcmp(argv[1], "drop") == 0 )
			return do_mem_map_drop(argc - 2, & argv[2]);
		if ( strcmp(argv[1], "list") == 0 )
			return do_mem_map_list(argc - 2, & argv[2]);		
		if ( strcmp(argv[1], "tlb") == 0 )
			return do_mem_map_tlbs(argc - 2, & argv[2]);
		if ( strcmp(argv[1], "test") == 0 )
			return do_mem_map_test(argc - 2, & argv[2]);
	}
	
	return CMD_RET_USAGE;
}
#endif	/* CONFIG_CMD_MEMMAP */


#ifdef CONFIG_CMD_MEMMAP
U_BOOT_CMD(
	mmap,	5, 1, do_mem_map,
	"phys memory mapping",
	"phys_adr cpu_adr size"
);
#endif	/* CONFIG_CMD_MEMMAP */
