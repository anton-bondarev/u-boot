#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <bootheader.h>

#define SWAP32(value)                                 \
	((((uint32_t)((value) & 0x000000FF)) << 24) |   \
	 (((uint32_t)((value) & 0x0000FF00)) << 8) |    \
	 (((uint32_t)((value) & 0x00FF0000)) >> 8) |    \
	 (((uint32_t)((value) & 0xFF000000)) >> 24))

struct bmap {
	const char *	key;
	uint32_t	value;
};

/*
 * -DCMAKE_BUILD_TYPE=Debug
 */

struct bmap hdrs[] = {
	{ "bootimage",	 BOOTHEADER_MAGIC__BOOT_IMAGE_VALID	 },
	{ "hostimage",	 BOOTHEADER_MAGIC__HOST_IMAGE_VALID	 },
	{ "resumeimage", BOOTHEADER_MAGIC__RESUME_IMAGE_VALID	 },
	{ "resetimage",	 BOOTHEADER_MAGIC__CORERESET_IMAGE_VALID },
	{ /* Sentinel */ },
};

int get_file_length(FILE *fd)
{
	int len;

	fseek(fd, 0, SEEK_END);
	len = ftell(fd);
	rewind(fd);

	return len;
}

static uint32_t str2magic(const char *str)
{
	int i = 0;

	while (hdrs[i].key) {
		if (strcmp(hdrs[i].key, str) == 0)
			return hdrs[i].value;
		i++;
	}
	return (uint32_t)strtoll(str, NULL, 16);
}

uint32_t crc32(uint32_t crc, const void *buf, size_t size);

struct bootheader *file2image(const char *filename, uint32_t code, uint32_t e0, uint32_t e1, int pad)
{
	struct bootheader *hdr;
	int len;

	int ret;


	FILE *fd = fopen(filename, "r");
	FILE *rnd = fopen("/dev/urandom", "rb");

	if (!fd) {
		fprintf(stderr, "File open failed: %s\n", strerror(errno));
		return NULL;
	}

	len = get_file_length(fd);

	if (len == 0) {
		fprintf(stderr, "Input file is empty\n");
		goto errclose;
	}


	int imagelen = len + sizeof(struct bootheader);
	if (pad > len)
		imagelen += (pad - len);

	if (imagelen % sizeof(uint32_t))
		imagelen += imagelen % sizeof(uint32_t);

	hdr = calloc(imagelen, 1);
	if (!hdr)
		return NULL;

	ret = fread(hdr->imagedata, 1, imagelen - sizeof(struct bootheader), rnd);
	ret = fread(hdr->imagedata, 1, len, fd);

	if (ret != len) {
		fprintf(stderr, "Failed to read all input data: %d/%d \n", ret, len);
		ret = -EIO;
		goto errfreebuf;
	}

	if (pad > len)
        len = pad;

	hdr->length = SWAP32(len);
	hdr->magic = SWAP32(code);
	hdr->entry0 = SWAP32(e0);
	hdr->entry1 = SWAP32(e1);
	hdr->sum = SWAP32(crc32(0, hdr->imagedata, len));
	hdr->hdrsum = SWAP32(crc32(0, hdr, sizeof(*hdr) - sizeof(uint32_t)));

	return hdr;

errfreebuf:
	free(hdr);
errclose:
	fclose(fd);
	return NULL;
}

int image2file(const char *filename, struct bootheader *h)
{
	FILE *fd = fopen(filename, "w+");

	printf("Payload CRC32: 0x%x Header CRC32: 0x%x\n", h->sum, h->hdrsum);
	if (!fd)
		return -1;

	int towrite = SWAP32(h->length) + sizeof(struct bootheader);

	if (towrite % sizeof(uint32_t))
		towrite += sizeof(uint32_t) - (towrite % sizeof(uint32_t));

	int ret = fwrite(h, towrite, 1, fd);
	fclose(fd);
	return ret;
}


int check_file(const char *filename)
{
	int res;

	FILE *fd;

	fd = fopen(filename, "r");

	int len = get_file_length(fd);

	if (len == 0) {
		fprintf(stderr, "File %s is empty\n", filename);
		res = 0;
	}

	size_t ret = 0;

	if (fd != NULL) {
		struct bootheader *buf = malloc(len);
		ret = fread(buf, 1, len, fd);
		fclose(fd);

		printf("Magic vallue is %x\n", SWAP32(buf->magic));

		if (crc32(0, buf->imagedata, len) != SWAP32(buf->sum))
			printf("Control sum of image doesn't match!\n");

		if (crc32(0, buf, sizeof(*buf) - sizeof(uint32_t)) != SWAP32(buf->hdrsum))
			printf("Control sum of header doesn't match!\n");

		res = 1;
	} else {
		res = 0;
	}

	return res;
}

void usage(const char *nm)
{
	printf("MM7705 Image Packer v. 0.1 (c) Andrew Andrianov, RC Module\n");
	printf("Usage %s -i input_file -o out_file -t type\n",
	       nm);
	printf("\tType is either one of the following: \n"
	       "\t\tbootimage, resumeimage, hostimage, resetimage \n"
	       "or a hex number\n\n");
}



int main(int argc, char *argv[])
{
	int opt;
	struct bootheader *h;
	const char *header = "bootimage";
	const char *input = NULL;
	const char *output = NULL;
	const char *filename = NULL;
	uint32_t hdrcode;
	uint32_t e0 = 0x00040000 + 0x18;
	uint32_t e1 = 0x0;
	const char *fault = "none";
	int pad = 0;

	while ((opt = getopt(argc, argv, "i:o:t:hf:e:E:c:p:")) != -1) {
		switch (opt) {
		case 'c':
			filename = optarg;
			int ret = check_file(filename);
			exit(ret);
			break;
		case 'e':
			e0 = strtol(optarg, NULL, 16);
			break;
		case 'E':
			e1 = strtol(optarg, NULL, 16);
			break;
		case 'i':
			input = optarg;
			break;
		case 'o':
			output = optarg;
			break;
		case 't':
			header = optarg;
			break;
		case 'f':
			fault = optarg;
			break;
		case 'p':
			pad = atoi(optarg);
			break;
		default:
			usage(argv[0]);
			exit(1);
		}
	}

	if (!input) {
		fprintf(stderr, "Missing input filename\n");
		usage(argv[0]);
		exit(1);
	}

	if (!output) {
		//Why input?
		fprintf(stderr, "Missing input filename\n");
		usage(argv[0]);
		exit(1);
	}

	hdrcode = str2magic(header);
	printf("Entry points: ep0: 0x%x ep1: 0x%x\n", e0, e1);
	h = file2image(input, hdrcode, e0, e1, pad);
	if (!h) {
		fprintf(stderr, "Failed to load/prepare input image \n");
		exit(1);
	}

	/* Handle fault injection */
	if (strcmp(fault, "header") == 0) {
		printf("[WARN] Injecting fault in header\n");
		h->length++;
	} else if (strcmp(fault, "data") == 0) {
		printf("[WARN] Injecting fault in image data\n");
		h->imagedata[SWAP32(h->length) - 1]++;
	} else if (strcmp(fault, "magic") == 0) {
		printf("[WARN] Injecting fault in magic\n");
		h->magic++;
	}

	int ret = image2file(output, h);
	if (ret > 0)
		return 0;
	return 0;
}
