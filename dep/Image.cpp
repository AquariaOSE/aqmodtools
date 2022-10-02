#include "Image.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include <stdlib.h>
#include <string.h>
#include "qoi.h"



Image::Image()
	: _w(0), _h(0)
{
}

Image::Image(size_t w, size_t h)
: _w(w), _h(h), _v(w*h)
{
}

void Image::resize(size_t w, size_t h)
{
	_w = w;
	_h = h;
	_v.resize(w*h);
}

void Image::fill(unsigned col)
{
	const size_t n = _v.size();
	for(size_t i = 0; i < n; ++i)
		_v[i] = col;
}

unsigned char* Image::mallocOut()
{
	const size_t N = _w * _h;
	stbi_uc * const d = (stbi_uc*)malloc(N * 4);
	if(d)
	{
		stbi_uc *p = d;

		for(size_t i = 0; i < N; ++i)
		{
			unsigned c = _v[i];
			*p++ = c & 0xff; c >>= 8;
			*p++ = c & 0xff; c >>= 8;
			*p++ = c & 0xff; c >>= 8;
			*p++ = c & 0xff;
		}
	}
	return d;
}

bool Image::writePNG(const char* fn)
{
	stbi_uc * const b = mallocOut();
	int ok = stbi_write_png(fn, (int)_w, (int)_h, 4, b, 0);
	free(b);
	return !!ok;
}

bool Image::writeQOI(const char* fn)
{
	qoi_desc d;
	d.channels = 4;
	d.colorspace = QOI_LINEAR;
	d.width = (unsigned)_w;
	d.height = (unsigned)_h;
	stbi_uc * const b = mallocOut();
	size_t sz = qoi_write(fn, b, &d);
	free(b);
	return !!sz;
}


bool Image::load(const char* fn)
{
	int x, y, c;
	stbi_uc *b = stbi_load(fn, &x, &y, &c, 4);
	if(!b)
	{
		qoi_desc d;
		b = (stbi_uc*)qoi_read(fn, &d, 4);
		return false;
	}
	_w = x;
	_h = y;
	const size_t N = _w * _h;
	_v.resize(N);
	stbi_uc *p = b;
	for(size_t i = 0; i < N; ++i)
	{
		unsigned c = *p++;
		c |= *p++ << 8;
		c |= *p++ << 16;
		c |= *p++ << 24;
		_v[i] = c;
	}
	free(b);
	return true;
}
