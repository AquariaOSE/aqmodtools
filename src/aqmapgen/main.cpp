
/* This code is released into the public domain. */

#include <string>
#include <string.h>
#include <stdio.h>
#include <queue>
#include <algorithm>
#include "Image.h"
#include "Filter.h"
#include "stb_image_resize.h"


inline unsigned red  (unsigned c) { return (c      ) & 0xff; }
inline unsigned green(unsigned c) { return (c >> 8 ) & 0xff; }
inline unsigned blue (unsigned c) { return (c >> 16) & 0xff; }
inline unsigned alpha(unsigned c) { return (c >> 24) & 0xff; }
inline unsigned mkcol(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	return r | (g << 8) | (b << 16) | (a << 24);
}

#define WHITE 240

// floor to next power of 2 (not changed if already power of 2)
inline unsigned flp2(unsigned x)
{
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return x - (x >> 1);
}

// ceil to next power of 2 (not changed if already power of 2)
inline unsigned clp2(unsigned x)
{
	--x;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return x + 1;
}


void processImage(Image& img, int radius, float sigma, float scale)
{
	const size_t w = img.width();
	const size_t h = img.height();

	Matrix<float> ma(w, h);
	for(size_t y = 0; y < h; ++y)
	{
		for(size_t x = 0; x < w; ++x)
		{
			unsigned c = img(x, y);
			float avg = ((red(c) + green(c) + blue(c)) * (alpha(c) / 255.0f))/ float(255*3);
			ma(x, y) = (avg < 0.5f) ? 0.0f : 1.0f;
		}
	}

	Matrix<float> tmp;
	if(scale != 1)
	{
		float scaledW = (float(w) * scale) + 0.0001f; // ward off rounding errors
		float scaledH = (float(h) * scale) + 0.0001f;
		tmp.resize((size_t)scaledW, (size_t)scaledH);
		stbir_resize_float_generic(ma.data(), w, h, w*sizeof(float), tmp.data(), tmp.width(), tmp.height(), tmp.width()*sizeof(float), 1,
			STBIR_ALPHA_CHANNEL_NONE, 0, STBIR_EDGE_CLAMP, STBIR_FILTER_CUBICBSPLINE, STBIR_COLORSPACE_LINEAR, NULL);
	}
	else
		tmp = ma;

	if(radius)
	{
		Matrix<float> blurred(tmp.width(), tmp.height());
		Filter::gaussianBlur(tmp, blurred, radius, sigma);
		tmp = blurred;
	}

	const size_t ww = clp2(tmp.width());
	const size_t hh = clp2(tmp.height());
	const size_t halfMarginX = (ww - tmp.width()) / 2;
	const size_t halfMarginY = (hh - tmp.height()) / 2;

	img.resize(ww, hh);
	img.fill(mkcol(WHITE, WHITE, WHITE, 0));
	for(size_t y = 0; y < tmp.height(); ++y)
		for(size_t x = 0; x < tmp.width(); ++x)
			img(x + halfMarginX, y + halfMarginY) = mkcol(WHITE, WHITE, WHITE, int(255*tmp(x, y)));
}


void processFile(const char *fn, const char *out, int radius, float sigma, float scale)
{
	Image img;
	if(!img.load(fn))
	{
		printf("File not processed: %s\n", fn);
		return;
	}

	const char *dot = strrchr(out, '.');
	const bool qoi = !strcmp(dot, ".qoi");

	printf("Processing %s (%ux%u)... ", fn, (unsigned)img.width(), (unsigned)img.height());
	fflush(stdout);
	processImage(img, radius, sigma, scale);
	printf("saving %s (%ux%u)... ", out, (unsigned)img.width(), (unsigned)img.height());
	fflush(stdout);

	const bool ok = qoi
		? img.writeQOI(out)
		: img.writePNG(out);

	if(ok)
		printf("OK\n");
	else
		printf("Failed to write!\n");
}


int main(int argc, char **argv)
{
	if(argc <= 2)
	{
		printf("Usage: ./aqmapgen input.png output.png [scale=0.5] [radius=0] [sigma=radius]\n");
		return 2;
	}

	int radius = 0;
	float sigma = 0;
	float scale = 0.5f;

	if(argc >= 4)
		scale = (float)atof(argv[3]);
	if(argc >= 5)
		radius = atoi(argv[4]);
	if(argc >= 6)
		sigma = (float)atof(argv[5]);


	processFile(argv[1], argv[2], radius, sigma, scale);

	return 0;
}

