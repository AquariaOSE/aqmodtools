#ifndef MATRIX_H
#define MATRIX_H

#include <vector>
#include <string.h>

template <typename T> class Matrix
{
private:
	std::vector<T> _v;
	size_t _shift;
	size_t _w;
	size_t _h;

public:
	Matrix() : _shift(0), _w(0), _h(0) {}
	Matrix(size_t w, size_t h) { resize(w, h); }
	~Matrix() {}

	inline size_t width() const { return _w; }
	inline size_t height() const { return _h; }

	inline T* data() { return _v.size() ? &_v[0] : NULL; }
	inline const T* data() const { return _v.size() ? &_v[0] : NULL; }

	void clear()
	{
		_w = _h = _shift = 0;
		_v.clear();
	}

	void resize(size_t w, size_t h)
	{
		if(!(w && h))
		{
			clear();
			return;
		}
		_w = w;
		_h = h;
		size_t newsize = 1;
		size_t sh = 0;

		while(newsize < w)
		{
			newsize <<= 1;
			++sh;
		}
		_shift = sh;

		_v.resize(newsize * _h);
	}

	inline T& operator()(size_t x, size_t y)
	{
		return _v[(y << _shift) | x];
	}

	inline const T& operator()(size_t x, size_t y) const
	{
		return _v[(y << _shift) | x];
	}

	inline const T& getMirrored(int x, int y) const
	{
		x = (x<0)? abs(x) : x;
		y = (y<0)? abs(y) : y;

		x = (x>=int(_w))? (_w - (x - _w + 1)) : x;
		y = (y>=int(_h))? (_h - (y - _h + 1)) : y;

		return (*this)(x,y);
	}

	inline T& getMirrored(int x, int y)
	{
		x = (x<0)? abs(x) : x;
		y = (y<0)? abs(y) : y;

		x = (x>=int(_w))? (_w - (x - _w + 1)) : x;
		y = (y>=int(_h))? (_h - (y - _h + 1)) : y;

		return (*this)(x,y);
	}

};


#endif
