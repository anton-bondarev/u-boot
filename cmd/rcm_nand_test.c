#include <common.h>
#include <console.h>
#include <linux/mtd/mtd.h>
#include <asm/tlb47x.h>
#include <nand.h>

DECLARE_GLOBAL_DATA_PTR;

//#define DBG_ONE_PAGE_PER_BLOCK
//#define DBG_INS_RAND_ERROR
//#define DBG_WR_DATA_AS_ADDR

#define MAX_PAGE_LEN 4096
static int g_verbose = 0;
static int g_err_max = 20;

static struct mtd_info* do_nand_test_info( bool show_info,
								unsigned int* block_size, unsigned int* blocks_count,
								unsigned int* page_size, unsigned int* pages_count )
{
	unsigned int bc = 0, pc = 0;
	struct mtd_info* mtd = get_nand_dev_by_index(0);
	if (!mtd) {
		printf( "Mtd information not available\n" );
		return NULL;
	}
	if( show_info ) {
		printf( "NAND information:\n" );
		printf( "block size %u(0x%x)\n", mtd->erasesize, mtd->erasesize );
		printf( "page size  %u(0x%x)\n", mtd->writesize, mtd->writesize );
		printf( "oob size   %u(0x%x)\n", mtd->oobsize, mtd->oobsize );
		printf( "full size  %llu(0x%llx)\n", mtd->size, mtd->size );
	}

	if(  mtd->erasesize ) {
		bc = mtd->size / mtd->erasesize;
		if( show_info )
			printf( "blocks in flash %u\n", bc );
	}
	else {
		printf( "Error: erasesize 0\n" );
		return NULL;
	}

	if(  mtd->writesize ) {
		pc = mtd->erasesize / mtd->writesize;
		if( show_info )
			printf( "pages in block %u\n", pc );
	}
	else {
		printf( "Error: writesize 0\n" );
		return NULL;
	}

	if( block_size )
		*block_size = mtd->erasesize;

	if( blocks_count )
		*blocks_count = bc;

	if( page_size )
		*page_size =  mtd->writesize;

	if( pages_count )
		*pages_count = pc;

	return mtd;
}

static int nand_block_erase( struct mtd_info* mtd, unsigned int offset, unsigned int length )
{
	int ret;
	struct erase_info instr;

	instr.mtd = mtd;
	instr.addr = offset;
	instr.len = length;
	instr.callback = 0;

	ret =  mtd_erase(mtd, &instr);
	if( ret )
		printf( "erase failed,error%d\n", ret );
	return ret;
}

static int nand_rand_data_write( struct mtd_info* mtd, unsigned int offset, unsigned int length, u_char* buf, uint32_t* cs )
{
	int ret;
	unsigned int i;
	size_t retlen;

	srand(get_ticks());

	if( cs ) *cs = 0;

#ifdef DBG_WR_DATA_AS_ADDR
	for( i = 0; i < length; i+=4 ) {
		uint32_t* b32 = (uint32_t*)(buf+i);
		*b32 = offset+i;
	}
#else
	for( i = 0; i < length; i++ ) {
		buf[i] = rand();
		if( cs ) *cs += buf[i]; // just for information now
	}
#endif

	ret = mtd_write(mtd, offset, length, &retlen, buf );

	if( retlen != length )
		printf( "write: lentgh mismatch %u,%u\n", length, retlen );

	if( ret )
		printf( "write: failed error %d\n", ret );

#ifdef DBG_INS_RAND_ERROR
	if( !( rand() % 111 ) )
		buf[10] = 0x55;
#endif

	return ret;
}

static int nand_data_read( struct mtd_info* mtd, unsigned int offset, unsigned int length, u_char* buf, uint32_t* cs )
{
	int ret;
	unsigned int i;
	size_t retlen;

	if( cs ) *cs = 0;

	ret = mtd_read(mtd, offset, length, &retlen, buf );
	if( retlen != length )
		printf( "read: lentgh mismatch %u,%u\n", length, retlen );
	if( cs )
		for( i = 0; i < length; i++ )
			*cs += buf[i]; // just for information now
	return ret;
}

static int do_nand_test_run( unsigned int block_first, unsigned int block_end )
{
	struct mtd_info* mtd;
	unsigned int bl, bl_start, bl_end, bl_num, bl_len = 0;
	unsigned int pg, pg_num, pg_len;
	unsigned int bl_off, pg_off;
	static u_char buf_wr[MAX_PAGE_LEN], buf_rd[MAX_PAGE_LEN];
	unsigned int bad_pages_total = 0;
	uint32_t csw, csr;
	
	mtd = do_nand_test_info( false, &bl_len, &bl_num, &pg_len, &pg_num );
	if( mtd == NULL )
		return -1;

	if( pg_len > MAX_PAGE_LEN ) {
		printf( "Too big page length\n" );
		return -1;
	}

	if( block_first != UINT_MAX && block_end != UINT_MAX ) {
		if( block_first > block_end || block_end >= bl_num ) {
			printf( "Bad parameters (first=%u,end=%u)\n", block_first, block_end );
			return -1;
		}
		bl_start = block_first;
		bl_end = block_end;
	}
	else {
		bl_start = 0;
		bl_end = bl_num-1;
	}

	printf( "NAND test starts (blocks %u-%u):\n", bl_start, bl_end );

	bl_off = bl_start * bl_len;
	for( bl = bl_start; bl <= bl_end; bl++ ) {
			printf( "Block %u(offset 0x%x) check...", bl, bl_off );

		if( g_verbose )
			printf( " erase..." );

		if( nand_block_erase( mtd, bl_off, bl_len ) )
			return -1;
		
#ifdef DBG_ONE_PAGE_PER_BLOCK // checking only page #1 (second!) in block
		pg_off = bl_off + pg_len;
		for( pg = 1; pg < 2; pg++ ) {
#else
		pg_off = bl_off;
		int bad = 0;
		for( pg = 0; pg < pg_num; pg++ ) {
#endif
			if( g_verbose )
				printf( "Page %u,offset 0x%x,length %u: write...", pg, pg_off, pg_len );

			if( nand_rand_data_write( mtd, pg_off, pg_len, buf_wr, &csw ) )
				return -1;

			if( g_verbose )
				printf( " read..." );
			
			if( nand_data_read( mtd, pg_off, pg_len, buf_rd, &csr ) )
				return -1;

			if( g_verbose )
				printf( " compare..." );

			if( memcmp( buf_wr, buf_rd, pg_len ) ) {
				bad++;
				bad_pages_total++;
				if( g_verbose )
					printf( "ERROR\n" );
			}
			else {
				if( g_verbose )
					printf( "OK (%08x-%08x)\n", csw, csr );
			}

			pg_off += pg_len;

			if(ctrlc())
				return 0;
		}

		if( !g_verbose )
			printf( !bad ? "OK\r" : "ERROR(bad pages %u)\n", bad );

		bl_off += bl_len;
	}

	if( bad_pages_total > g_err_max ) {
		printf( "\nNAND test failed, too many bad pages %u\n", bad_pages_total );
		return -1;
	}
	printf( "\nNAND test passed,total bad pages %u\n", bad_pages_total );
	return 0;
}

static int do_max_err( const char* maxerr ) {
	unsigned long tmp;
	if( strict_strtoul( maxerr, 10, &tmp ) < 0  || tmp > 100 )
		printf( "Wrong argument\n" );
	else
		g_err_max = tmp;
	return 0;
}

static int do_verbose( const char* on_off ) {
	if( !strcmp( on_off, "on" ) ) {
		g_verbose = 1;
		return 0;
	}
	else if( !strcmp( on_off, "off" ) ) {
		g_verbose = 0;
		return 0;
	}
	return CMD_RET_USAGE;
}

static int do_nand_test(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	if ( argc == 2 && !strcmp( argv[1], "info" ) ) {
		do_nand_test_info(true,NULL,NULL,NULL, NULL);
		return 0;
	}
	else if( argc == 2 && !strcmp( argv[1], "run" ) ) {
		do_nand_test_run( UINT_MAX, UINT_MAX );
		return 0;
	}
	else if( argc == 3 && !strcmp( argv[1], "maxerr" ) )
		return do_max_err( argv[2] );
	else if( argc == 3 && !strcmp( argv[1], "verbose" ) )
		return do_verbose( argv[2] );
	else if( argc == 4 && !strcmp( argv[1], "run" ) ) {
		unsigned long block_first, block_end;
		if( strict_strtoul( argv[2], 10, &block_first ) < 0 || strict_strtoul( argv[3], 10, &block_end ) < 0 ) {
			printf( "Wrong sector number\n" );
			return 0;
		}
		do_nand_test_run( (unsigned int)block_first, (unsigned int)block_end );
		return 0;
	}
	else if( argc == 2 && !strcmp( argv[1], "version" ) ) {
		printf( "NAND test 1.0.0\n" );
		return 0;
	}
	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	nandtest, 4, 0, do_nand_test,
	"NAND test for MB115 board",
	"info              - mapping information\n" \
	"nandtest verbose [on off]  - show advanced information\n" \
	"nandtest maxerr #errors    - set the maximum number of bad pages\n" \
	"nandtest run               - sram words are 32 bits wide and are filled with address values\n" \
	"nandtest run #start #end  - the same check for blocks from #start(0,1...) to #end only\n" \
	"nandtest version           - output version information"
);
