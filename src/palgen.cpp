
/* This code is released into the public domain. */

#include <string.h>
#include <stdio.h>
#include <algorithm>
#include "Image.h"

static unsigned shuf[] = { 0, 1, 2 };

static const unsigned step = 8;
static const unsigned blocksPerRow = 8;
static const unsigned blocks = 256 / step;
static const unsigned pixelsPerSide = 256 / step;
static const unsigned pixelsPerRow = pixelsPerSide * blocksPerRow;
static const unsigned rows = (blocks + (blocksPerRow - 1)) / blocksPerRow;
static const unsigned pixelsHeight = rows * (pixelsPerSide + 1);

static void doBlockRow(Image& img, unsigned xs, unsigned ys, const unsigned c3)
{
    unsigned c1 = 0, c2 = 0;
    for(unsigned y = 0; y < pixelsPerSide; ++y, c2 += step)
        for(unsigned x = 0; x < pixelsPerSide; ++x, c1 += step)
        {
            const unsigned c = 0xff000000 | (c1 << (8 * shuf[0])) | (c2 << (8 * shuf[1])) | (c3 << (8 * shuf[2]));
            img(xs+x, ys+y) = c;
        }
}

static void doSeparator(Image& img, const unsigned y, unsigned c)
{
    unsigned x = 0;
    for(unsigned b = 0; b < blocksPerRow; ++b)
    {
        const unsigned cc = 0xff000000 | (c << (8 * shuf[2]));
        for( ; x < pixelsPerRow; ++x)
            img(x, y) = cc;
        c += step;
    }
}

static void makepal()
{
    Image img(pixelsPerRow, pixelsHeight);
    unsigned c3 = 0;
    for(unsigned y = 0; y < pixelsHeight; y += pixelsPerSide)
    {
        doSeparator(img, y, c3);
        ++y;
        for(unsigned x = 0; x < pixelsPerRow; x += pixelsPerSide, c3 += step)
            doBlockRow(img, x, y, c3);
    }
    char buf[32];
#define S(x) ("rgb"[shuf[x]])
    sprintf(buf, "pal_%c%c%c.png", S(0), S(1), S(2));
#undef S
    img.writePNG(&buf[0]);
}

int main(int argc, char **argv)
{
    do
        makepal();
    while(std::next_permutation(&shuf[0], &shuf[3]));
	return 0;
}

