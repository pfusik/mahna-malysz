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
png_colorp palette;
int num_palette;
png_color palette6[6];
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
	png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
	memcpy(palette6, palette, 5*3);
	image = png_get_rows(png_ptr, info_ptr);
	fclose(fp);
	png_data_freer(
		png_ptr, info_ptr,
		PNG_USER_WILL_FREE_DATA, PNG_FREE_ROWS
	);
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
}

static void write_png(const char* filename) {
	png_structp png_ptr;
	png_infop info_ptr;
	FILE* fp;
	palette6[5].red = 0xff;
	palette6[5].green = 0;
	palette6[5].blue = 0;
	fp = fopen(filename, "wb");
	assert(fp != NULL, "nie mozna otworzyc %s", filename);
	png_ptr = png_create_write_struct(
		PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL
	);
	assert(png_ptr != NULL, "png_create_write_struct");
	info_ptr = png_create_info_struct(png_ptr);
	assert(info_ptr != NULL, "png_create_info_struct");
	png_init_io(png_ptr, fp);
	png_set_IHDR(
		png_ptr, info_ptr, width, height,
		4, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT
	);
	png_set_PLTE(png_ptr, info_ptr, palette6, 6);
	png_set_rows(png_ptr, info_ptr, image);
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_PACKING, NULL);
	fclose(fp);
	png_destroy_write_struct(&png_ptr, &info_ptr);
}

void ramka(int x, int y) {
	int i;
	for (i = 0; i < 7; i++) {
		image[y][x + i] = 5;
		image[y + i][x + 7] = 5;
		image[y + 1 + i][x] = 5;
		image[y + 7][x + 1 + i] = 5;
	}
}

unsigned char digits[] = {
	7,5,5,5,7, /* 0 */
	2,2,2,2,2, /* 1 */
	7,1,7,4,7, /* 2 */
	7,1,3,1,7, /* 3 */
	5,5,7,1,1, /* 4 */
	7,4,7,1,7, /* 5 */
	7,4,7,5,7, /* 6 */
	7,1,1,1,1, /* 7 */
	7,5,7,5,7, /* 8 */
	7,5,7,1,7  /* 9 */
};

void cyfra(int x, int y, int c) {
	int i;
	int j;
	for (j = 0; j < 5; j++) {
		for (i = 2; i >= 0; i--) {
			if ((digits[5 * c + j] >> i) & 1) {
				image[y + j][x + 2 - i] = 5;
			}
		}
	}
}

void cyfry(int x, int y, int v) {
	cyfra(x, y, v / 10);
	cyfra(x + 4, y, v % 10);
}

int tenkolor;
unsigned char tenznak[8];

int getznak(int x, int y) {
	int i;
	int j;
	tenkolor = -1;
	for (j = 0; j < 8; j++) {
		tenznak[j] = 0;
		for (i = 0; i < 8; i++) {
			int c = image[y + j][x + i];
			if (c == 0) continue;
			if (c > 4 || c < 0) return -1;
			if (tenkolor != c) {
				if (tenkolor > 0) return -1;
				tenkolor = c;
			}
			tenznak[j] |= 128 >> i;
		}
	}
	if (tenkolor < 0) tenkolor = 0;
	return 0;
}

unsigned char font[512];
unsigned char *screen;
int screenlen;
int znakow;
int znak;

void znajdz(void) {
	for (znak = 0; znak < znakow; znak++) {
		if (memcmp(tenznak, font + 8 * znak, 8) == 0) return;
	}
	znak = -1;
}

void logosuj(void) {
	unsigned char *sp;
	int x;
	int y;
	sp = screen;
	znakow = 0;
	for (y = 0; y < height; y += 8) {
		for (x = 0; x < width; x += 8) {
			if (getznak(x, y)) {
				ramka(x, y);
				continue;
			}
			znajdz();
			if (znak < 0) {
				if (znakow >= 64) {
					ramka(x, y);
					continue;
				}
				memcpy(font + 8 * znakow, tenznak, 8);
				znak = znakow++;
			}
			cyfry(x, y, znak);
			*sp++ = ((tenkolor - 1) << 6) | znak;
		}
	}
}

void write_raw(const char *filename, unsigned char *what, int length) {
	FILE *fp;
	fp = fopen(filename, "wb");
	assert(fp != NULL, "nie mozna otworzyc %s", filename);
	fwrite(what, 1, length, fp);
	fclose(fp);
}

int main(int argc, char *argv[]) {
	read_png("tlo.png");
	assert(width % 8 == 0, "szerokosc / 8");
	assert(height % 8 == 0, "wysokosc / 8");
	screenlen = width / 8 * (height / 8);
	screen = malloc(screenlen);
	logosuj();
	write_png("logosed.png");
	write_raw("tlo.fnt", font, 8 * znakow);
	write_raw("tlo.scr", screen, screenlen);
	return 0;
}
