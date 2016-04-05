#include "Matrix.h"
#include "Vector.h"

namespace Filter {

//convolution with 1D kernel
//faster than 2D convolution, with linear running time
//valid alternative if kernel is separable
template <typename T> void convolve(const Matrix<T>& mtx, Matrix<T>& dst, const Vector<T>& kernelX, const Vector<T>& kernelY)
{
	const uint width = mtx.width();
	const uint height = mtx.height();

	T val;
	const uint kSizeX = kernelX.size();
	const uint kSizeY = kernelY.size();
	const uint offsetX = kSizeX / 2;
	const uint offsetY = kSizeY / 2;
	Matrix<T> tmp(width, height);

	//convolution in x-direction
	for(uint y=0; y<height; ++y)
	{
		for(uint x=0; x<width; ++x)
		{
			val = 0.;
			for(uint i=0; i<kSizeX; ++i)
			{
				val += kernelX[i] * mtx.getMirrored(x+offsetX-i, y);
			}
			tmp(x,y) = val;
		}
	}

	//convolution in y-direction
	for(uint y=0; y<height; ++y)
	{
		for(uint x=0; x<width; ++x)
		{
			val = 0;
			for(uint i=0; i<kSizeY; ++i)
			{
				val += kernelY[i] * tmp.getMirrored(x, y+offsetY-i);
			}
			dst(x,y) = val;
		}
	}
}

template <typename T> void gaussianBlur(const Matrix<T>& mtx, Matrix<T>& dst, int radius, float sigma)
{
	if(!sigma)
		sigma = (float)radius;

	const int kSize = (2 * radius) + 1;
	const float pi = 2.0f * asinf(1.0);
	const float divExp = 2 * sigma * sigma;
	const float div = sqrtf(2 * pi) * sigma;
	T res, scale;
	scale = 0;

	Vector<T> kernel(kSize);
	for(int i=0; i<kSize; ++i)
	{
		res = expf(- ((i-radius)*(i-radius)) / divExp);
		res /= div;
		kernel[i] = res;
		scale += res;
	}
	kernel /= (T)scale;

	convolve(mtx, dst, kernel, kernel);
}

} // end namespace filter

