#pragma once

#include <vector>

class Image
{
	size_t _w, _h;
	std::vector<unsigned> _v;
	unsigned char *mallocOut();

public:
	Image();
	Image(size_t w, size_t h);

	void resize(size_t w, size_t h);
	void fill(unsigned col);

	size_t width() const {return _w;}
	size_t height() const {return _h;}

	// 0xAABBGGRR
	inline unsigned& operator() (size_t x, size_t y)
	{
		return _v[y * _w + x];
	}
	inline unsigned operator() (size_t x, size_t y) const
	{
		return _v[y * _w + x];
	}

	bool writePNG(const char* fn);
	bool writeQOI(const char* fn);
	bool load(const char* fn);
};
