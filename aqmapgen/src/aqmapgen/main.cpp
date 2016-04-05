
/* This code is released into the public domain. */

#include <string>
#include <stdio.h>
#include <queue>
#include <algorithm>
#include "ImagePNG.h"
#include "Filter.h"


inline unsigned int red  (unsigned int c) { return (c      ) & 0xff; }
inline unsigned int green(unsigned int c) { return (c >> 8 ) & 0xff; }
inline unsigned int blue (unsigned int c) { return (c >> 16) & 0xff; }
inline unsigned int alpha(unsigned int c) { return (c >> 24) & 0xff; }
inline unsigned int mkcol(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	return r | (g << 8) | (b << 16) | (a << 24);
}

void processImage(Image& img, int radius, float sigma)
{
	const unsigned int w = img.width();
	const unsigned int h = img.height();

	Matrix<float> ma(w, h);
	for(unsigned int y = 0; y < h; ++y)
	{
		for(unsigned int x = 0; x < w; ++x)
		{
			unsigned int c = img(x, y);
			float avg = ((red(c) + green(c) + blue(c)) * (alpha(c) / 255.0f))/ float(255*3);
			ma(x, y) = (avg < 0.5f) ? 0.0f : 1.0f;
		}
	}

	Matrix<float> blurred(w, h);
	Filter::gaussianBlur(ma, blurred, 5, 2);

	for(unsigned int y = 0; y < h; ++y)
	{
		for(unsigned int x = 0; x < w; ++x)
		{
			img(x, y) = mkcol(240, 240, 240, int(255*blurred(x, y)));
			//img(x, y) = mkcol(0, 0, 0, int(255*blurred(x, y)));
		}
	}
	
}


void processFile(const char *fn, const char *out, int radius, float sigma)
{
	Image img;
	if(!img.readPNG(fn))
	{
		printf("File not processed: %s\n", fn);
		return;
	}

	printf("Processing %s ... ", fn);
	fflush(stdout);
	processImage(img, radius, sigma);
	printf("saving ... ");
	fflush(stdout);

	if(img.writePNG(out))
		printf("OK\n");
	else
		printf("Failed to write!\n");
}


int main(int argc, char **argv)
{
	if(argc <= 2)
	{
		printf("Usage: ./aqmapgen input.png output.png [radius=1] [sigma=2]\n");
		return 2;
	}

	int radius = 0;
	float sigma = 0;

	if(argc >= 4)
		radius = atoi(argv[3]);
	if(argc >= 5)
		sigma = (float)atof(argv[4]);
	if(radius <= 0)
		radius = 1;

	processFile(argv[1], argv[2], radius, sigma);

	return 0;
}

