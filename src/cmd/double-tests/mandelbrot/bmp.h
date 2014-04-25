#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned short WORD;
typedef unsigned long  DWORD;

typedef struct tagBmpHeader {
//	char  type[2];       /* = "BM", it is matipulated separately to make the size of this structure the multiple of 4, */
	DWORD sizeFile;      /* = offbits + sizeImage */
	DWORD reserved;      /* == 0 */
	DWORD offbits;       /* offset from start of file = 54 + size of palette */
	DWORD sizeStruct;    /* 40 */
	DWORD width, height;
	WORD  planes;        /* 1 */
	WORD  bitCount;      /* bits of each pixel, 256color it is 8, 24 color it is 24 */
	DWORD compression;   /* 0 */
	DWORD sizeImage;     /* (width+?)(till multiple of 4) * height£¬in bytes */
	DWORD xPelsPerMeter; /* resolution in mspaint */
	DWORD yPelsPerMeter;
	DWORD colorUsed;     /* if use all, it is 0 */
	DWORD colorImportant;/*  */
} BmpHeader;


typedef struct tagBitmap{
	size_t width;
	size_t height;
	size_t size;
	unsigned char *data;
}Bitmap;

#define RGB(r, g, b) ((((r)&0xFF)<<16) + (((g)&0xFF)<<8) + ((b)&0xFF))

#define CEIL4(x) ((((x)+3)/4)*4)

Bitmap mbmp = {0};
int    misopen = 0;
size_t mcolor = 0;

void setcolor(size_t c){mcolor = c;}

void closebmp(void)
{
	if (misopen) { /* clear bitmap */
		mbmp.width = mbmp.height = 0;
		if (mbmp.data != NULL) {
			free(mbmp.data);
			mbmp.data = NULL;
		}
	}
}

/* create a new bitmap file */
int newbmp(int width, int height){
	if (width <= 0 || height <= 0) {
		printf("Width and height should be positve.\n");
		return 1;
	}

	if (misopen) closebmp();
	/* fill bmp struct */
	mbmp.width = (size_t)width;
	mbmp.height = (size_t)height;
	mbmp.size = CEIL4(mbmp.width * 3) * mbmp.height;

	if ((mbmp.data = (unsigned char*)malloc(mbmp.size)) == NULL){
		printf("Error: alloc fail!");
		return 1;
	}
	memset(mbmp.data, 0xFF, mbmp.size); /* make background color white */

	misopen = 1;

	return 0;
}

int savebmp(char *fname)
{
	FILE *fp;
	BmpHeader head = {0, 0, 54, 40, 0, 0, 1, 24, 0, 0}; /* BMP file header */

	if ((fp = fopen(fname, "wb")) == NULL) {
		printf("Error: can't save to BMP file \"%s\".\n", fname);
		return 1;
	}

	fputc('B', fp); fputc('M', fp); /* write type */
	/* fill BMP file header */
	head.width = mbmp.width;
	head.height = mbmp.height;
	head.sizeImage = mbmp.size;
	head.sizeFile = mbmp.size + head.offbits;
	fwrite(&head, sizeof head, 1, fp); /* write header */
	if (fwrite(mbmp.data, 1, mbmp.size, fp) != mbmp.size) {
		fclose(fp);
		return 1; /* write bitmap infomation */
	}

	fclose(fp);
	return 0;
}

size_t getpixel(size_t x, size_t y)
{
	unsigned char *p;

	if (x < mbmp.width && y < mbmp.height){
		p = mbmp.data + CEIL4(mbmp.width*3)*y + x*3;
		return p[0]+p[1]*256L+p[2]*256L*256L;
	}
	return 0;
}

int putpixel(size_t x, size_t y, size_t color)
{
	unsigned char *p;

	if (x < mbmp.width && y < mbmp.height){
		p = mbmp.data + CEIL4(mbmp.width*3)*y + x*3;
		p[0] = (unsigned char)(color & 0xFF);
		p[1] = (unsigned char)((color>>8) & 0xFF);
		p[2] = (unsigned char)((color>>16) & 0xFF);
		return 0;
	}
	return 1;
}
