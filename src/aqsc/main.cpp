#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <zlib/zlib.h>

#define CHUNK 16384

static int unpackFile(FILE *source, FILE *dest)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);

    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

int packFile(FILE *source, FILE *dest, int level)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, level);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
        compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END);        /* stream will be complete */

    /* clean up and return */
    (void)deflateEnd(&strm);

    return Z_OK;
}

static void oops(const char *s)
{
    printf("ERROR: %s\n", s);
#if _WIN32
    printf("--- Press enter key to exit ---\n"); // linux users will probably not want this
#endif
    getchar();
    exit(1);
}

static bool processFile(const std::string& sourcef, const std::string& destf_plain)
{
    FILE *source = fopen(sourcef.c_str(), "rb");
    FILE *dest = NULL;
    bool success = true;

    if (!source)
        oops("Can't open input file");

    char buf[15+1];
    memset(buf, 0, 15);

    int s = fread(buf, 1, 15, source);
    fseek(source, 0, SEEK_SET);

    if(s != 15)
    {
        oops("file too short");
        return false;
    }

    bool isXML = !strcmp(buf, "<Version major=");

    if(isXML)
    {
        // not compressed, pack
        printf("File is plain XML, packing...\n");
        std::string newf = destf_plain + ".aqs";
        FILE *dest = fopen(newf.c_str(), "wb");
        packFile(source, dest, 9);
        fclose(dest);
    }
    else
    {
        // compressed, unpack
        printf("File seems to be packed, unpacking to plain XML...\n");
        std::string newf = destf_plain + ".xml";
        FILE *dest = fopen(newf.c_str(), "wb");
        int r = unpackFile(source, dest);
        fclose(dest);
        if(r != Z_OK)
        {
            unlink(newf.c_str());
            oops("Failed to decompress input file");
        }
    }

    fclose(source);

    return success;
}

static void usage()
{
    printf("AQS <-> XML converter\n"
           "Usage: aqsc <filename>\n");
}

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        usage();
        return 0;
    }
    std::string fn = argv[1];

    unsigned int s = fn.size();
    unsigned int dot = fn.find_last_of('.');
    std::string plain = fn.substr(0, dot);

    processFile(fn, plain);

    return 0;
}
