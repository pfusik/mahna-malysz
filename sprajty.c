#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <png.h>

#undef assert
static void assert(int condition, const char* format, ...)
{
	if (!condition) {
		va_list args;
		va_start(args, format);
		fprintf(stderr, "Blad krytyczny: ");
		vfprintf(stderr, format, args);
		fprintf(stderr, "\n");
		unlink("logosed.png");
		exit(3);
	}
}

int width;
int height;
unsigned char **image;

void read_png(const char *filename) {
	png_structp png_ptr;
	png_infop info_ptr;
	FILE* fp;
	fp = fopen(filename, "rb");
	assert(fp != NULL, "nie mozna otworzyc %s", filename);
	png_ptr = png_create_read_struct(
		PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL
	);
	assert(png_ptr != NULL, "png_create_read_struct");
	info_ptr = png_create_info_struct(png_ptr);
	assert(info_ptr != NULL, "png_create_info_struct");
	png_init_io(png_ptr, fp);
	png_read_png(
		png_ptr, info_ptr,
		PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_STRIP_ALPHA
		| PNG_TRANSFORM_PACKING/* | PNG_TRANSFORM_EXPAND*/,
		NULL
	);
	width = png_get_image_width(png_ptr, info_ptr);
	height = png_get_image_height(png_ptr, info_ptr);
	image = png_get_rows(png_ptr, info_ptr);
	fclose(fp);
	png_data_freer(
		png_ptr, info_ptr,
		PNG_USER_WILL_FREE_DATA, PNG_FREE_ROWS
	);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
}

unsigned char bgcolor;
unsigned char data[4][256];
int pos = 0;

void cut(const char *label, int x, int y, int h) {
	int i;
	printf("%s equ $%02X\n", label, pos);
	x *= 8;
	for (h += y; y < h; y++) {
		int color0 = -1;
		int color1 = -1;
		data[0][pos] = 0;
		data[1][pos] = 0;
		for (i = 0; i < 8; i++) {
			unsigned char p = image[y][x + i];
			if (p == bgcolor) continue;
			p &= ~1;
			if (color0 < 0) color0 = p;
			else if (p != color0 && color1 < 0) color1 = p;
			if (p == color0) data[0][pos] |= 128 >> i;
			else if (p == color1) data[1][pos] |= 128 >> i;
			else if (p == (color0 | color1)) {
				data[0][pos] |= 128 >> i;
				data[1][pos] |= 128 >> i;
			}
			else {
				printf("; Error: x=%d y=%d\n", x + i, y);
			}
		}
		data[2][pos] = color0 >= 0 ? color0 : pos > 0 ? data[2][pos - 1] & ~1 : 0;
		data[3][pos] = color1 >= 0 ? color1 : pos > 0 ? data[3][pos - 1] & ~1 : 0;
		pos++;
	}
	data[3][pos - 1] |= 1;
}

void dump(void) {
	static const char *labels[] = {
		"Sprite_data0", "Sprite_data1", "Sprite_color0", "Sprite_color1"
	};
	int i;
	int j;
	for (i = 0; i < 4; i++) {
		printf(labels[i]);
		for (j = 0; j < pos; j++) {
			printf(j % 8 == 0 ? "\n\tdta\t" : ",");
			printf("$%02X", data[i][j]);
		}
		printf("\n");
	}
}

int main(int argc, char *argv[]) {
	read_png("anime.png");
	bgcolor = image[0][0];
	cut("ADAMLOT0", 0, 0, 16);
	cut("ADAMLOT1", 1, 0, 16);
	cut("ADAMZJAZD0", 2, 0, 16);
	cut("ADAMZJAZD1", 3, 0, 16);
	cut("ADAMZJAZD2", 4, 0, 16);
	cut("ADAMZJAZD3", 5, 0, 16);
	cut("BOCIAN0", 0, 16, 8);
	cut("BOCIAN1", 1, 16, 8);
	cut("BANAN", 0, 24, 8);
	{
		int i;
		char red[100];
		for (i = 0; i < 5; i++) {
			sprintf(red, "RED%d", i);
			cut(red, i, 32, 8);
		}
	}
	cut("HANNAWALD", 0, 40, 16);
	cut("JAGA", 0, 56, 16);
	cut("WIWAT0", 0, 72, 16);
	cut("WIWAT1", 1, 72, 16);
	dump();
	return 0;
}
