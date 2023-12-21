
/* This code is released into the public domain. */

#include <algorithm>
#include <string>
#include "Image.h"
#include "Matrix.h"


struct Pos
{
	Pos(size_t xx, size_t yy, size_t n) : x(xx), y(yy), nb(n) {}
	size_t x, y, nb;

	inline bool operator< (const Pos& p) const
	{
		return nb < p.nb;
	}
};

inline unsigned red  (unsigned c) { return (c      ) & 0xff; }
inline unsigned green(unsigned c) { return (c >> 8 ) & 0xff; }
inline unsigned blue (unsigned c) { return (c >> 16) & 0xff; }
inline unsigned alpha(unsigned c) { return (c >> 24) & 0xff; }

template<typename T> inline T vmin(T a, T b) { return a < b ? a : b; }
template<typename T> inline T vmax(T a, T b) { return a > b ? a : b; }

void pngrimAccurate(Image& img)
{
	const int w = (int)img.width();
	const int h = (int)img.height();
	Matrix<unsigned char> solid(w, h);
	std::vector<Pos> P, Q, R;

	for(int y = 0; y < h; ++y)
		for(int x = 0; x < w; ++x)
		{
			if(alpha(img(x, y)))
				solid(x, y) = 1;
			else
			{
				Pos p(x, y, 0);
				solid(x, y) = 0;
				for(int oy = -1; oy <= 1; ++oy)
					for(int ox = -1; ox <= 1; ++ox)
					{
						const int xn = x + ox;
						const int yn = y + oy;
						if(xn >= 0 && yn >= 0 && xn < w && yn < h && alpha(img(xn, yn)))
							++p.nb;
					}

					if(p.nb)
						P.push_back(p);
			}
		}

	while(P.size())
	{
		std::sort(P.begin(), P.end());
		P.swap(Q);

		while(Q.size())
		{
			unsigned r = 0;
			unsigned g = 0;
			unsigned b = 0;
			unsigned c = 0;
			Pos p = Q.back();
			Q.pop_back();
			if(solid(p.x, p.y))
				continue;

			for(int oy = -1; oy <= 1; ++oy)
			{
				const int y = int(p.y) + oy;
				if(y >= 0 && y < h)
				{
					for(int ox = -1; ox <= 1; ++ox)
					{
						if(oy || ox)
						{
							const int x = int(p.x) + ox;
							if(x >= 0 && x < w)
							{
								if(solid(x, y))
								{
									const unsigned pix = img(x, y);
									r +=   red(pix);
									g += green(pix);
									b +=  blue(pix);
									++c;
								}
								else
									R.push_back(Pos(x, y, 0));
							}
						}
					}
				}
			}

			solid(p.x, p.y) = 1;
			img(p.x, p.y) =
				  ((r / c)      )
				| ((g / c) << 8 )
				| ((b / c) << 16);
		}

		while(R.size())
		{
			Pos p = R.back();
			R.pop_back();
			if(solid(p.x, p.y))
				continue;

			for(int oy = -1; oy <= 1; ++oy)
				for(int ox = -1; ox <= 1; ++ox)
				{
					const size_t xn = p.x + ox;
					const size_t yn = p.y + oy;
					if(xn < w && yn < h && solid(xn, yn))
						++p.nb;
				}

				P.push_back(p);
		}
	}
}

void pngrimFast(Image& img)
{
	const int w = (int)img.width();
	const int h = (int)img.height();
	const unsigned inf = 0x7fffffff;
	Matrix<unsigned> dist(w, h);

	size_t numtrans = 0;
	for(int y = 0; y < h; ++y)
		for(int x = 0; x < w; ++x)
		{
			const unsigned isTrans = !alpha(img(x, y));
			dist(x, y) = isTrans;
			numtrans += isTrans;
		}
	std::vector<Pos> todo;
	todo.reserve(numtrans);

	// distance transform, X direction
	for(int y = 0; y < h; ++y)
	{
		unsigned * const row = &dist(0, y);
		unsigned d = inf;
		for(int x = 0; x < w; ++x)
			if(row[x])
				row[x] = ++d;
			else
				d = 0;
		d = inf;
		for(int x = w-1; x; --x)
			if(const unsigned val = row[x])
				row[x] = vmin(++d, val);
			else
				d = 0;
	}

	// distance transform, Y direction
	for(int x = 0; x < w; ++x)
	{
		unsigned d = dist(x, 0);
		for(int y = 0; y < h; ++y)
		{
			const unsigned val = dist(x, y);
			if(val >= d)
				dist(x, y) = vmin(++d, val);
			else
				d = val;
		}
		d = dist(x, h-1);
		for(int y = h-1; y; --y)
		{
			const unsigned val = dist(x, y);
			if(val >= d)
				dist(x, y) = vmin(++d, val);
			else
				d = val;
		}
	}

	// Use distance as heuristic for pixel processing order
	for(size_t y = 0; y < h; ++y)
		for(size_t x = 0; x < w; ++x)
			if(unsigned d = dist(x, y))
				todo.push_back(Pos(x, y, d));
	std::sort(todo.begin(), todo.end());

	for(size_t i = 0; i < todo.size(); ++i)
	{
		Pos p = todo[i];
		unsigned r = 0, g = 0, b = 0, c = 0;

		for(int oy = -1; oy <= 1; ++oy)
		{
			const size_t y = (p.y) + oy;
			if(y < h)
			{
				for(int ox = -1; ox <= 1; ++ox)
				{
					const size_t x = (p.x) + ox;
					if(x < w)
					{
						if((ox || oy) && !dist(x, y))
						{
							const unsigned pix = img(x, y);
							r +=   red(pix);
							g += green(pix);
							b +=  blue(pix);
							++c;
						}
					}
				}
			}
		}

		dist(p.x, p.y) = 0;
		img(p.x, p.y) =
			  ((r / c)      )
			| ((g / c) << 8 )
			| ((b / c) << 16);
	}
}


void processImage(Image& img, bool fast)
{
	if(fast)
		pngrimFast(img);
	else
		pngrimAccurate(img);
}

void processFile(const char *fn, bool fast, bool qoi)
{
	Image img;
	if(!img.load(fn))
	{
		printf("File not processed: %s\n", fn);
		return;
	}

	std::string ofn;
	const char *sep = strrchr(fn, '/');
	const char *dot = strrchr(fn, '.');
#ifdef _WIN32
	{
		const char *bs = strrchr(fn, '\\');
		sep = std::max(sep, bs);
	}
#endif
	if(dot > sep)
	{
		ofn.assign(fn, dot - fn);
	}
	else
		ofn = fn;

	ofn += (qoi ? ".qoi" : ".png");

	printf("Processing %s ... ", fn);
	fflush(stdout);
	processImage(img, fast);
	printf("saving [%s] ... ", ofn.c_str());
	fflush(stdout);

	bool ok = qoi
		? img.writeQOI(ofn.c_str())
		: img.writePNG(ofn.c_str());

	if(ok)
		printf("OK\n");
	else
		printf("Failed to write!\n");
}


int main(int argc, char **argv)
{
	if(argc <= 1)
	{
		printf("Usage: ./pngrim [--fast] [--png] file1.png [fileX.png ...]\n");
		printf("Load PNG files, outputs in QOI format by default. \n");
		printf("Warning: --png modifies files in place!\n");
		return 2;
	}

	int begin = 1;
	bool fast = false, qoi = false;
	if(!strcmp(argv[begin], "--fast"))
	{
		++begin;
		fast = true;
	}
	if(!strcmp(argv[begin], "--png"))
	{
		++begin;
		qoi = false;
	}
	if(!strcmp(argv[begin], "--qoi"))
	{
		++begin;
		qoi = true;
	}

	for(int i = begin; i < argc; ++i)
		processFile(argv[i], fast, qoi);

	return 0;
}
