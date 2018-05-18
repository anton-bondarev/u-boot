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

#define MAP_BASE  0x50000000

extern uint32_t _invalidate_tlb_entry(uint32_t tlb);
extern void _write_tlb_entry(uint32_t tlb0, uint32_t tlb1, uint32_t tlb2, uint32_t zero);

typedef enum 
{
	TLBS_4K   = 0x00,
	TLBS_16K  = 0x01,
	TLBS_64K  = 0x03,
	TLBS_1M   = 0x07,
	TLBS_16M  = 0x0F,
	TLBS_256M = 0x1F,
	TLBS_1G   = 0x3F,
	TLBS_ERR  = 0xFF
} TLB_SIZE;

typedef struct 
{
	TLB_SIZE size;
	const char * name;
} tlb_size_name;

static const tlb_size_name size_names[] = 
{
	{ TLBS_4K,   "4k"   },
	{ TLBS_16K,  "16k"  },
	{ TLBS_64K,  "64k"  },
	{ TLBS_1M,   "1m"   },
	{ TLBS_16M,  "16m"  },
	{ TLBS_256M, "256m" },
	{ TLBS_1G,   "1g"   },
	{ TLBS_ERR,  ""     }
};	

typedef struct 
{
	uint32_t phys_adr;
	uint32_t cpu_adr;
	TLB_SIZE size;
} tlb_entr;

static tlb_entr s_entr[1] = { { 0, 0, TLBS_ERR } };

////////////////////////////////////////////////////////////////

static TLB_SIZE get_tlb_size_by_name(const char * name)
{
	uint i;
	for ( i = 0; size_names[i].size != TLBS_ERR; ++ i ) 
		if ( strcmp(name, size_names[i].name) == 0 )
			break;
	return size_names[i].size;
}
static const char * get_tlb_name_by_size(TLB_SIZE size)
{
	uint i;
	for ( i = 0; size_names[i].size != TLBS_ERR; ++ i ) 
		if ( size_names[i].size == size )
			break;
	return size_names[i].name;
}

static int mem_map_list(void)
{
	printf("Maps: \n");
	if ( s_entr[0].size != TLBS_ERR ) 
		printf("  Phys: %08x, Cpu: %08x, Size: %s\n", s_entr[0].phys_adr, s_entr[0].cpu_adr, get_tlb_name_by_size(s_entr[0].size));
	else
		printf("  None\n");
	return 0;
}

int find_tlb_entr(uint32_t cpu_adr)
{
	if ( s_entr[0].size == TLBS_ERR ) 
		return -1;
	if ( s_entr[0].cpu_adr != cpu_adr ) 
		return -1;
	return 0;
}

int find_empty_tlb_entr(void)
{
	if ( s_entr[0].size != TLBS_ERR ) 
		return -1;
	return 0;
}

static int mem_map_drop(uint32_t cpu_adr)
{
	int tlb_ind = find_tlb_entr(cpu_adr);
	if ( tlb_ind == -1 ) {
		printf("Map not found\n");
		return 0;
	}
	
	_invalidate_tlb_entry(cpu_adr | ((s_entr[tlb_ind].size | 0x00) << 4));
	
	s_entr[tlb_ind].size = TLBS_ERR;
	
	return 0;
}

static int mem_map_set(uint32_t phys_adr, uint32_t cpu_adr, TLB_SIZE size)
{
	int tlb_ind;
	
	tlb_ind = find_tlb_entr(cpu_adr);
	if ( tlb_ind != -1 ) {
		printf("Map exists\n");
		return 0;	
	}
	
	tlb_ind = find_empty_tlb_entr();
	if ( tlb_ind == -1 ) {
		printf("Maps full\n");
		return 0;	
	}
	
	_write_tlb_entry(cpu_adr | ((size | 0x80) << 4), phys_adr, 0x0003043F, 0);
	
	s_entr[tlb_ind].phys_adr = phys_adr;
	s_entr[tlb_ind].cpu_adr = cpu_adr;
	s_entr[tlb_ind].size = size;
	
	return 0;
}

/*
 * Perform a memory mapping.
 */
static int do_mem_map(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if ( argc == 2 ) {
		if ( strcmp(argv[1], "list") == 0 )
			return mem_map_list();
		
		if ( strcmp(argv[1], "help") == 0 ) {
			printf("mmap set  cpu_adr { 4k | 16k | 64k | 1m | 16m | 256m | 1g } phys_adr\n");
			printf("mmap drop cpu_adr\n");
			printf("mmap list\n");
			return 0;
		}
			
		return CMD_RET_USAGE;
	} 
	
	if ( argc == 3 ) {
		if ( strcmp(argv[1], "drop") != 0 ) 
			return CMD_RET_USAGE;
		
		ulong cpu_adr;
		if ( strict_strtoul(argv[2], 16, & cpu_adr) < 0 )
			return CMD_RET_USAGE;
		
		return mem_map_drop(cpu_adr);
	}
	
	if ( argc == 5 ) {
		if ( strcmp(argv[1], "set") != 0 ) 
			return CMD_RET_USAGE;
		
		ulong cpu_adr;
		if ( strict_strtoul(argv[2], 16, & cpu_adr) < 0 ) {
			printf("Bad cpu_adr\n");
			return CMD_RET_USAGE;
		}
		
		TLB_SIZE tlb_size = get_tlb_size_by_name(argv[3]);
		if ( tlb_size == TLBS_ERR ) {
			printf("Bad size\n");
			return CMD_RET_USAGE;
		}
		
		ulong phys_adr;
		if ( strict_strtoul(argv[4], 16, & phys_adr) < 0 ) {
			printf("Bad phys_adr\n");
			return CMD_RET_USAGE;
		}
		
		return mem_map_set(phys_adr, cpu_adr, tlb_size);
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
