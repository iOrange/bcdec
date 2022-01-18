#ifndef BCDEC_HEADER_INCLUDED
#define BCDEC_HEADER_INCLUDED

/*  Used information sources:
    https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression
    https://docs.microsoft.com/en-us/windows/win32/direct3d11/bc6h-format
    https://docs.microsoft.com/en-us/windows/win32/direct3d11/bc7-format
    https://www.khronos.org/registry/DataFormat/specs/1.1/dataformat.1.1.html#BPTC
    https://fgiesen.wordpress.com/2021/10/04/gpu-bcn-decoding/
*/

#define BCDEC_BC1_BLOCK_SIZE    8
#define BCDEC_BC2_BLOCK_SIZE    16
#define BCDEC_BC3_BLOCK_SIZE    16
#define BCDEC_BC4_BLOCK_SIZE    8
#define BCDEC_BC5_BLOCK_SIZE    16
#define BCDEC_BC6H_BLOCK_SIZE   16
#define BCDEC_BC7_BLOCK_SIZE    16

#define BCDEC_BC1_COMPRESSED_SIZE(w, h)     ((((w)>>2)*((h)>>2))*BCDEC_BC1_BLOCK_SIZE)
#define BCDEC_BC2_COMPRESSED_SIZE(w, h)     ((((w)>>2)*((h)>>2))*BCDEC_BC2_BLOCK_SIZE)
#define BCDEC_BC3_COMPRESSED_SIZE(w, h)     ((((w)>>2)*((h)>>2))*BCDEC_BC3_BLOCK_SIZE)
#define BCDEC_BC4_COMPRESSED_SIZE(w, h)     ((((w)>>2)*((h)>>2))*BCDEC_BC4_BLOCK_SIZE)
#define BCDEC_BC5_COMPRESSED_SIZE(w, h)     ((((w)>>2)*((h)>>2))*BCDEC_BC5_BLOCK_SIZE)
#define BCDEC_BC6H_COMPRESSED_SIZE(w, h)    ((((w)>>2)*((h)>>2))*BCDEC_BC6H_BLOCK_SIZE)
#define BCDEC_BC7_COMPRESSED_SIZE(w, h)     ((((w)>>2)*((h)>>2))*BCDEC_BC7_BLOCK_SIZE)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void bcdec_bc1(void* compressedBlock, void* decompressedBlock, int destinationPitch);
void bcdec_bc2(void* compressedBlock, void* decompressedBlock, int destinationPitch);
void bcdec_bc3(void* compressedBlock, void* decompressedBlock, int destinationPitch);
void bcdec_bc4(void* compressedBlock, void* decompressedBlock, int destinationPitch);
void bcdec_bc5(void* compressedBlock, void* decompressedBlock, int destinationPitch);
void bcdec_bc6h(void* compressedBlock, void* decompressedBlock, int destinationPitch, int isSigned);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#ifdef BCDEC_IMPLEMENTATION

typedef struct bcdec__rgba {
    unsigned char r, g, b, a;
} bcdec__rgba_t;

void bcdec__color_block(void* compressedBlock, void* decompressedBlock, int destinationPitch, int onlyOpaqueMode) {
    unsigned short c0, c1;
    bcdec__rgba_t refColors[4];
    unsigned char* dstColors;
    unsigned char* colorIndices;
    int i, j, idx;

    refColors[0].a = refColors[1].a = refColors[2].a = refColors[3].a = 0xFF;

    c0 = ((unsigned short*)compressedBlock)[0];
    c1 = ((unsigned short*)compressedBlock)[1];

    /* Expand 565 ref colors to 888 */
    refColors[0].r = (((c0 >> 11) & 0x1F) * 527 + 23) >> 6;
    refColors[0].g = (((c0 >> 5)  & 0x3F) * 259 + 33) >> 6;
    refColors[0].b =  ((c0        & 0x1F) * 527 + 23) >> 6;

    refColors[1].r = (((c1 >> 11) & 0x1F) * 527 + 23) >> 6;
    refColors[1].g = (((c1 >> 5)  & 0x3F) * 259 + 33) >> 6;
    refColors[1].b =  ((c1        & 0x1F) * 527 + 23) >> 6;

    if (c0 > c1 || onlyOpaqueMode) {    /* Standard BC1 mode (also BC3 color block uses ONLY this mode) */
        /* color_2 = 2/3*color_0 + 1/3*color_1
           color_3 = 1/3*color_0 + 2/3*color_1 */
        refColors[2].r = (2 * refColors[0].r + refColors[1].r + 1) / 3;
        refColors[2].g = (2 * refColors[0].g + refColors[1].g + 1) / 3;
        refColors[2].b = (2 * refColors[0].b + refColors[1].b + 1) / 3;

        refColors[3].r = (refColors[0].r + 2 * refColors[1].r + 1) / 3;
        refColors[3].g = (refColors[0].g + 2 * refColors[1].g + 1) / 3;
        refColors[3].b = (refColors[0].b + 2 * refColors[1].b + 1) / 3;
    } else {                            /* Quite rare BC1A mode */
        /* color_2 = 1/2*color_0 + 1/2*color_1;
           color_3 = 0;                         */
        refColors[2].r = (refColors[0].r + refColors[1].r + 1) >> 1;
        refColors[2].g = (refColors[0].g + refColors[1].g + 1) >> 1;
        refColors[2].b = (refColors[0].b + refColors[1].b + 1) >> 1;

        refColors[3].r = refColors[3].g = refColors[3].b = refColors[3].a = 0;
    }

    colorIndices = ((unsigned char*)compressedBlock) + 4;

    /* Fill out the decompressed color block */
    dstColors = (unsigned char*)decompressedBlock;
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            idx = (colorIndices[i] >> (2 * j)) & 0x03;
            dstColors[4 * j + 0] = refColors[idx].r;
            dstColors[4 * j + 1] = refColors[idx].g;
            dstColors[4 * j + 2] = refColors[idx].b;
            dstColors[4 * j + 3] = refColors[idx].a;
        }

        dstColors += destinationPitch;
    }
}

void bcdec__sharp_alpha_block(void* compressedBlock, void* decompressedBlock, int destinationPitch) {
    unsigned short* alpha;
    unsigned char* decompressed;
    int i, j;

    alpha = (unsigned short*)compressedBlock;
    decompressed = (unsigned char*)decompressedBlock;

    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            decompressed[j * 4] = ((alpha[i] >> (4 * j)) & 0x0F) * 17;
        }

        decompressed += destinationPitch;
    }
}

void bcdec__smooth_alpha_block(void* compressedBlock, void* decompressedBlock, int destinationPitch, int pixelSize) {
    unsigned char* block;
    unsigned char* decompressed;
    unsigned char alpha[8];
    int i, j, indices;

    block = (unsigned char*)compressedBlock;
    decompressed = (unsigned char*)decompressedBlock;

    alpha[0] = block[0];
    alpha[1] = block[1];

    if (alpha[0] > alpha[1]) {
        /* 6 interpolated alpha values. */
        alpha[2] = (6 * alpha[0] +     alpha[1] + 1) / 7;   /* 6/7*alpha_0 + 1/7*alpha_1 */
        alpha[3] = (5 * alpha[0] + 2 * alpha[1] + 1) / 7;   /* 5/7*alpha_0 + 2/7*alpha_1 */
        alpha[4] = (4 * alpha[0] + 3 * alpha[1] + 1) / 7;   /* 4/7*alpha_0 + 3/7*alpha_1 */
        alpha[5] = (3 * alpha[0] + 4 * alpha[1] + 1) / 7;   /* 3/7*alpha_0 + 4/7*alpha_1 */
        alpha[6] = (2 * alpha[0] + 5 * alpha[1] + 1) / 7;   /* 2/7*alpha_0 + 5/7*alpha_1 */
        alpha[7] = (    alpha[0] + 6 * alpha[1] + 1) / 7;   /* 1/7*alpha_0 + 6/7*alpha_1 */
    }
    else {
        /* 4 interpolated alpha values. */
        alpha[2] = (4 * alpha[0] +     alpha[1]) / 5;       /* 4/5*alpha_0 + 1/5*alpha_1 */
        alpha[3] = (3 * alpha[0] + 2 * alpha[1]) / 5;       /* 3/5*alpha_0 + 2/5*alpha_1 */
        alpha[4] = (2 * alpha[0] + 3 * alpha[1]) / 5;       /* 2/5*alpha_0 + 3/5*alpha_1 */
        alpha[5] = (    alpha[0] + 4 * alpha[1]) / 5;       /* 1/5*alpha_0 + 4/5*alpha_1 */
        alpha[6] = 0x00;
        alpha[7] = 0xFF;
    }

    /* first 8 indices */
    indices = block[2] | (block[3] << 8) | (block[4] << 16);
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            decompressed[j * pixelSize] = alpha[indices & 0x07];
            indices >>= 3;
        }

        decompressed += destinationPitch;

        if (i == 1) {
            /* second 8 indices */
            indices = block[5] | (block[6] << 8) | (block[7] << 16);
        }
    }
}

typedef struct bcdec__bitstream {
    unsigned char*  bitstream;
    int             bitPos;
} bcdec__bitstream_t;

int bcdec__bitstream_read_bit(bcdec__bitstream_t* bstream) {
    int i, b;

    i = bstream->bitPos >> 3;
    b = (bstream->bitstream[i] >> (bstream->bitPos - (i << 3))) & 0x01;
    bstream->bitPos++;
    return b;
}

int bcdec__bitstream_read_bits(bcdec__bitstream_t* bstream, int numBits) {
    int result = 0, i = numBits;
    while (i) {
        result |= (bcdec__bitstream_read_bit(bstream) << (numBits - i));
        i--;
    }
    return result;
}

/*  reversed bits pulling, used in BC6H decoding
    why Microsoft ?? just why ???                   */
int bcdec__bitstream_read_bits_r(bcdec__bitstream_t* bstream, int numBits) {
    int result = 0;
    while (numBits--) {
        result <<= 1;
        result |= bcdec__bitstream_read_bit(bstream);
    }
    return result;
}



void bcdec_bc1(void* compressedBlock, void* decompressedBlock, int destinationPitch) {
    bcdec__color_block(compressedBlock, decompressedBlock, destinationPitch, 0);
}

void bcdec_bc2(void* compressedBlock, void* decompressedBlock, int destinationPitch) {
    bcdec__color_block(((char*)compressedBlock) + 8, decompressedBlock, destinationPitch, 1);
    bcdec__sharp_alpha_block(compressedBlock, ((char*)decompressedBlock) + 3, destinationPitch);
}

void bcdec_bc3(void* compressedBlock, void* decompressedBlock, int destinationPitch) {
    bcdec__color_block(((char*)compressedBlock) + 8, decompressedBlock, destinationPitch, 1);
    bcdec__smooth_alpha_block(compressedBlock, ((char*)decompressedBlock) + 3, destinationPitch, 4);
}

void bcdec_bc4(void* compressedBlock, void* decompressedBlock, int destinationPitch) {
    bcdec__smooth_alpha_block(compressedBlock, decompressedBlock, destinationPitch, 1);
}

void bcdec_bc5(void* compressedBlock, void* decompressedBlock, int destinationPitch) {
    bcdec__smooth_alpha_block(compressedBlock, decompressedBlock, destinationPitch, 2);
    bcdec__smooth_alpha_block(((char*)compressedBlock) + 8, ((char*)decompressedBlock) + 1, destinationPitch, 2);
}

/* http://graphics.stanford.edu/~seander/bithacks.html#VariableSignExtend */
int bcdec__extend_sign(int val, int bits) {
    return (val << (32 - bits)) >> (32 - bits);
}

int bcdec__transform_inverse(int val, int a0, int bits, int isSigned) {
    /* If the precision of A0 is "p" bits, then the transform algorithm is:
       B0 = (B0 + A0) & ((1 << p) - 1) */
    val = (val + a0) & ((1 << bits) - 1);
    if (isSigned) {
        val = bcdec__extend_sign(val, bits);
    }
    return val;
}

/* pretty much copy-paste from documentation */
int bcdec__unquantize(int val, int bits, int isSigned) {
    int unq, s = 0;

    if (!isSigned) {
        if (bits >= 15) {
            unq = val;
        } else if (!val) {
            unq = 0;
        } else if (val == ((1 << bits) - 1)) {
            unq = 0xFFFF;
        } else {
            unq = ((val << 16) + 0x8000) >> bits;
        }
    } else {
        if (bits >= 16) {
            unq = val;
        } else {
            if (val < 0) {
                s = 1;
                val = -val;
            }

            if (val == 0) {
                unq = 0;
            } else if (val >= ((1 << (bits - 1)) - 1)) {
                unq = 0x7FFF;
            } else {
                unq = ((val << 15) + 0x4000) >> (bits - 1);
            }

            if (s) {
                unq = -unq;
            }
        }
    }
    return unq;
}

int bcdec__interpolate(int a, int b, int* weights, int index) {
    return (a * (64 - weights[index]) + b * weights[index] + 32) >> 6;
}

unsigned short bcdec__finish_unquantize(int val, int isSigned) {
    int s;

    if (!isSigned) {
        return (unsigned short)((val * 31) >> 6);                   /* scale the magnitude by 31 / 64 */
    } else {
        val = (val < 0) ? -(((-val) * 31) >> 5) : (val * 31) >> 5;  /* scale the magnitude by 31 / 32 */
        s = 0;
        if (val < 0) {
            s = 0x8000;
            val = -val;
        }
        return (unsigned short)(s | val);
    }
}

/* https://fgiesen.wordpress.com/2012/03/28/half-to-float-done-quic/ */
float bcdec__half_to_float_quick(unsigned short half) {
    static unsigned int magic = (254 - 15) << 23;
    static unsigned int was_infnan = (127 + 16) << 23;
    unsigned int o;

    o = (half & 0x7FFF) << 13;                      /* exponent / mantissa bits */
    *((float*)&o) *= *((float*)&magic);             /* exponent adjust */
    if (*((float*)&o) >= *((float*)&was_infnan)) {  /* make sure Inf / NaN survive */
        o |= 255 << 23;
    }
    o |= (half & 0x8000) << 16;                     /* sign bit */
    return *((float*)&o);
}

void bcdec_bc6h(void* compressedBlock, void* decompressedBlock, int destinationPitch, int isSigned) {
    static char actual_bits_count[4][14] = {
        { 10, 7, 11, 11, 11, 9, 8, 8, 8, 6, 10, 11, 12, 16 },   /*  W */
        {  5, 6,  5,  4,  4, 5, 6, 5, 5, 6, 10,  9,  8,  4 },   /* dR */
        {  5, 6,  4,  5,  4, 5, 5, 6, 5, 6, 10,  9,  8,  4 },   /* dG */
        {  5, 6,  4,  4,  5, 5, 5, 5, 6, 6, 10,  9,  8,  4 }    /* dB */
    };

    /* There are 32 possible partition sets for a two-region tile.
       Each 4x4 block represents a single shape.
       Here also every fix-up index has MSB bit set. */
    static unsigned char partition_sets[32][4][4] = {
        { {128, 0,   1, 1}, {0, 0, 1, 1}, {  0, 0, 1, 1}, {0, 0, 1, 129} },   /*  0 */
        { {128, 0,   0, 1}, {0, 0, 0, 1}, {  0, 0, 0, 1}, {0, 0, 0, 129} },   /*  1 */
        { {128, 1,   1, 1}, {0, 1, 1, 1}, {  0, 1, 1, 1}, {0, 1, 1, 129} },   /*  2 */
        { {128, 0,   0, 1}, {0, 0, 1, 1}, {  0, 0, 1, 1}, {0, 1, 1, 129} },   /*  3 */
        { {128, 0,   0, 0}, {0, 0, 0, 1}, {  0, 0, 0, 1}, {0, 0, 1, 129} },   /*  4 */
        { {128, 0,   1, 1}, {0, 1, 1, 1}, {  0, 1, 1, 1}, {1, 1, 1, 129} },   /*  5 */
        { {128, 0,   0, 1}, {0, 0, 1, 1}, {  0, 1, 1, 1}, {1, 1, 1, 129} },   /*  6 */
        { {128, 0,   0, 0}, {0, 0, 0, 1}, {  0, 0, 1, 1}, {0, 1, 1, 129} },   /*  7 */
        { {128, 0,   0, 0}, {0, 0, 0, 0}, {  0, 0, 0, 1}, {0, 0, 1, 129} },   /*  8 */
        { {128, 0,   1, 1}, {0, 1, 1, 1}, {  1, 1, 1, 1}, {1, 1, 1, 129} },   /*  9 */
        { {128, 0,   0, 0}, {0, 0, 0, 1}, {  0, 1, 1, 1}, {1, 1, 1, 129} },   /* 10 */
        { {128, 0,   0, 0}, {0, 0, 0, 0}, {  0, 0, 0, 1}, {0, 1, 1, 129} },   /* 11 */
        { {128, 0,   0, 1}, {0, 1, 1, 1}, {  1, 1, 1, 1}, {1, 1, 1, 129} },   /* 12 */
        { {128, 0,   0, 0}, {0, 0, 0, 0}, {  1, 1, 1, 1}, {1, 1, 1, 129} },   /* 13 */
        { {128, 0,   0, 0}, {1, 1, 1, 1}, {  1, 1, 1, 1}, {1, 1, 1, 129} },   /* 14 */
        { {128, 0,   0, 0}, {0, 0, 0, 0}, {  0, 0, 0, 0}, {1, 1, 1, 129} },   /* 15 */
        { {128, 0,   0, 0}, {1, 0, 0, 0}, {  1, 1, 1, 0}, {1, 1, 1, 129} },   /* 16 */
        { {128, 1, 129, 1}, {0, 0, 0, 1}, {  0, 0, 0, 0}, {0, 0, 0,   0} },   /* 17 */
        { {128, 0,   0, 0}, {0, 0, 0, 0}, {129, 0, 0, 0}, {1, 1, 1,   0} },   /* 18 */
        { {128, 1, 129, 1}, {0, 0, 1, 1}, {  0, 0, 0, 1}, {0, 0, 0,   0} },   /* 19 */
        { {128, 0, 129, 1}, {0, 0, 0, 1}, {  0, 0, 0, 0}, {0, 0, 0,   0} },   /* 20 */
        { {128, 0,   0, 0}, {1, 0, 0, 0}, {129, 1, 0, 0}, {1, 1, 1,   0} },   /* 21 */
        { {128, 0,   0, 0}, {0, 0, 0, 0}, {129, 0, 0, 0}, {1, 1, 0,   0} },   /* 22 */
        { {128, 1,   1, 1}, {0, 0, 1, 1}, {  0, 0, 1, 1}, {0, 0, 0, 129} },   /* 23 */
        { {128, 0, 129, 1}, {0, 0, 0, 1}, {  0, 0, 0, 1}, {0, 0, 0,   0} },   /* 24 */
        { {128, 0,   0, 0}, {1, 0, 0, 0}, {129, 0, 0, 0}, {1, 1, 0,   0} },   /* 25 */
        { {128, 1, 129, 0}, {0, 1, 1, 0}, {  0, 1, 1, 0}, {0, 1, 1,   0} },   /* 26 */
        { {128, 0, 129, 1}, {0, 1, 1, 0}, {  0, 1, 1, 0}, {1, 1, 0,   0} },   /* 27 */
        { {128, 0,   0, 1}, {0, 1, 1, 1}, {129, 1, 1, 0}, {1, 0, 0,   0} },   /* 28 */
        { {128, 0,   0, 0}, {1, 1, 1, 1}, {129, 1, 1, 1}, {0, 0, 0,   0} },   /* 29 */
        { {128, 1, 129, 1}, {0, 0, 0, 1}, {  1, 0, 0, 0}, {1, 1, 1,   0} },   /* 30 */
        { {128, 0, 129, 1}, {1, 0, 0, 1}, {  1, 0, 0, 1}, {1, 1, 0,   0} }    /* 31 */
    };

    static int aWeight3[8] = { 0, 9, 18, 27, 37, 46, 55, 64 };
    static int aWeight4[16] = { 0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64 };

    bcdec__bitstream_t bstream;
    int mode, partition, numPartitions, i, j, partitionSet, indexBits, index;
    int r[4], g[4], b[4];       /* wxyz */
    int epR[2], epG[2], epB[2]; /* endpoints A and B */
    float* decompressed;

    decompressed = (float*)decompressedBlock;

    bstream.bitstream = (unsigned char*)compressedBlock;
    bstream.bitPos = 0;

    r[0] = r[1] = r[2] = r[3] = 0;
    g[0] = g[1] = g[2] = g[3] = 0;
    b[0] = b[1] = b[2] = b[3] = 0;

    mode = bcdec__bitstream_read_bits(&bstream, 2);
    if (mode > 1) {
        mode |= (bcdec__bitstream_read_bits(&bstream, 3) << 2);
    }

    switch (mode) {
        /* mode 1 */
        case 0b00: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 75 bits (10.555, 10.555, 10.555) */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gy[4]   */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* by[4]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* bz[4]   */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* rw[9:0] */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* gw[9:0] */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* bw[9:0] */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* rx[4:0] */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gz[4]   */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* gx[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gz[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* bx[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 5);        /* ry[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 5);        /* rz[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 0;
        } break;

        /* mode 2 */
        case 0b01: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 75 bits (7666, 7666, 7666) */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* gy[5]   */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gz[4]   */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* gz[5]   */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 7);        /* rw[6:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* by[4]   */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 7);        /* gw[6:0] */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* by[5]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gy[4]   */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 7);        /* bw[6:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* bz[5]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* bz[4]   */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 6);        /* rx[5:0] */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 6);        /* gx[5:0] */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gz[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 6);        /* bx[5:0] */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 6);        /* ry[5:0] */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 6);        /* rz[5:0] */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 1;
        } break;

        /* mode 3 */
        case 0b00010: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 72 bits (11.555, 11.444, 11.444) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* rw[9:0] */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* gw[9:0] */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* bw[9:0] */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* rx[4:0] */
            r[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* rw[10]  */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gx[3:0] */
            g[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* gw[10]  */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gz[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 4);        /* bx[3:0] */
            b[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* bw[10]  */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 5);        /* ry[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 5);        /* rz[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 2;
        } break;

        /* mode 4 */
        case 0b00110: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 72 bits (11.444, 11.555, 11.444) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* rw[9:0] */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* gw[9:0] */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* bw[9:0] */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 4);        /* rx[3:0] */
            r[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* rw[10]  */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gz[4]   */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* gx[4:0] */
            g[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* gw[10]  */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gz[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 4);        /* bx[3:0] */
            b[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* bw[10]  */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* ry[3:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* rz[3:0] */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gy[4]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 3;
        } break;

        /* mode 5 */
        case 0b01010: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 72 bits (11.444, 11.444, 11.555) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* rw[9:0] */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* gw[9:0] */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* bw[9:0] */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 4);        /* rx[3:0] */
            r[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* rw[10]  */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* by[4]   */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gx[3:0] */
            g[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* gw[10]  */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gz[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* bx[4:0] */
            b[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* bw[10]  */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* ry[3:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* rz[3:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* bz[4]   */ 
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 4;
        } break;

        /* mode 6 */
        case 0b01110: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 72 bits (9555, 9555, 9555) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 9);        /* rw[8:0] */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* by[4]   */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 9);        /* gw[8:0] */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gy[4]   */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 9);        /* bw[8:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* bz[4]   */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* rx[4:0] */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gz[4]   */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* gx[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gx[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* bx[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 5);        /* ry[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 5);        /* rz[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 5;
        } break;

        /* mode 7 */
        case 0b10010: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 72 bits (8666, 8555, 8555) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 8);        /* rw[7:0] */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gz[4]   */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* by[4]   */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 8);        /* gw[7:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gy[4]   */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 8);        /* bw[7:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* bz[4]   */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 6);        /* rx[5:0] */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* gx[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gz[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* bx[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 6);        /* ry[5:0] */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 6);        /* rz[5:0] */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 6;
        } break;

        /* mode 8 */
        case 0b10110: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 72 bits (8555, 8666, 8555) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 8);        /* rw[7:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* by[4]   */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 8);        /* gw[7:0] */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* gy[5]   */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gy[4]   */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 8);        /* bw[7:0] */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* gz[5]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* bz[4]   */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* rx[4:0] */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gz[4]   */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 6);        /* gx[5:0] */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* zx[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* bx[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 5);        /* ry[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 5);        /* rz[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 7;
        } break;

        /* mode 9 */
        case 0b11010: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 72 bits (8555, 8555, 8666) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 8);        /* rw[7:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* by[4]   */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 8);        /* gw[7:0] */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* by[5]   */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gy[4]   */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 8);        /* bw[7:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* bz[5]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* bz[4]   */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* bw[4:0] */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gz[4]   */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 5);        /* gx[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gz[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 6);        /* bx[5:0] */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 5);        /* ry[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 5);        /* rz[4:0] */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 8;
        } break;

        /* mode 10 */
        case 0b11110: {
            /* Partitition indices: 46 bits
               Partition: 5 bits
               Color Endpoints: 72 bits (6666, 6666, 6666) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 6);        /* rw[5:0] */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gz[4]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream);            /* bz[0]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 1;       /* bz[1]   */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* by[4]   */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 6);        /* gw[5:0] */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* gy[5]   */
            b[2] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* by[5]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 2;       /* bz[2]   */
            g[2] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* gy[4]   */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 6);        /* bw[5:0] */
            g[3] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* gz[5]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 3;       /* bz[3]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 5;       /* bz[5]   */
            b[3] |= bcdec__bitstream_read_bit(&bstream) << 4;       /* bz[4]   */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 6);        /* rx[5:0] */
            g[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gy[3:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 6);        /* gx[5:0] */
            g[3] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gz[3:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 6);        /* bx[5:0] */
            b[2] |= bcdec__bitstream_read_bits(&bstream, 4);        /* by[3:0] */
            r[2] |= bcdec__bitstream_read_bits(&bstream, 6);        /* ry[5:0] */
            r[3] |= bcdec__bitstream_read_bits(&bstream, 6);        /* rz[5:0] */
            partition = bcdec__bitstream_read_bits(&bstream, 5);    /* d[4:0]  */
            mode = 9;
        } break;

        /* mode 11 */
        case 0b00011: {
            /* Partitition indices: 63 bits
               Partition: 0 bits
               Color Endpoints: 60 bits (10.10, 10.10, 10.10) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* rw[9:0] */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* gw[9:0] */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* bw[9:0] */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 10);       /* rx[9:0] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 10);       /* gx[9:0] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 10);       /* bx[9:0] */
            mode = 10;
        } break;

        /* mode 12 */
        case 0b00111: {
            /* Partitition indices: 63 bits
               Partition: 0 bits
               Color Endpoints: 60 bits (11.9, 11.9, 11.9) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* rw[9:0] */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* gw[9:0] */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* bw[9:0] */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 9);        /* rx[8:0] */
            r[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* rw[10]  */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 9);        /* gx[8:0] */
            g[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* gw[10]  */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 9);        /* bx[8:0] */
            b[0] |= bcdec__bitstream_read_bit(&bstream) << 10;      /* bw[10]  */
            mode = 11;
        } break;

        /* mode 13 */
        case 0b01011: {
            /* Partitition indices: 63 bits
               Partition: 0 bits
               Color Endpoints: 60 bits (12.8, 12.8, 12.8) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* rw[9:0] */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* gw[9:0] */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* bw[9:0] */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 8);        /* rx[7:0] */
            r[0] |= bcdec__bitstream_read_bits_r(&bstream, 2) << 10;/* rx[10:11] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 8);        /* gx[7:0] */
            g[0] |= bcdec__bitstream_read_bits_r(&bstream, 2) << 10;/* gx[10:11] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 8);        /* bx[7:0] */
            b[0] |= bcdec__bitstream_read_bits_r(&bstream, 2) << 10;/* bx[10:11] */
            mode = 12;
        } break;

        /* mode 14 */
        case 0b01111: {
            /* Partitition indices: 63 bits
               Partition: 0 bits
               Color Endpoints: 60 bits (16.4, 16.4, 16.4) */
            r[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* rw[9:0] */
            g[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* gw[9:0] */
            b[0] |= bcdec__bitstream_read_bits(&bstream, 10);       /* bw[9:0] */
            r[1] |= bcdec__bitstream_read_bits(&bstream, 4);        /* rx[3:0] */
            r[0] |= bcdec__bitstream_read_bits_r(&bstream, 6) << 10;/* rw[10:15] */
            g[1] |= bcdec__bitstream_read_bits(&bstream, 4);        /* gx[3:0] */
            g[0] |= bcdec__bitstream_read_bits_r(&bstream, 6) << 10;/* gw[10:15] */
            b[1] |= bcdec__bitstream_read_bits(&bstream, 4);        /* bx[3:0] */
            b[0] |= bcdec__bitstream_read_bits_r(&bstream, 6) << 10;/* bw[10:15] */
            mode = 13;
        } break;

        default: {
            /* Modes 10011, 10111, 11011, and 11111 (not shown) are reserved.
               Do not use these in your encoder. If the hardware is passed blocks
               with one of these modes specified, the resulting decompressed block
               must contain all zeroes in all channels except for the alpha channel. */
            for (i = 0; i < 4; ++i) {
                for (j = 0; j < 4; ++j) {
                    decompressed[j * 3 + 0] = 0.0f;
                    decompressed[j * 3 + 1] = 0.0f;
                    decompressed[j * 3 + 2] = 0.0f;
                }
                decompressed += destinationPitch;
            }

            return;
        }
    }

    if (mode >= 10) {
        partition = 0;
        numPartitions = 0;
    } else {
        numPartitions = 1;
    }

    if (isSigned) {
        r[0] = bcdec__extend_sign(r[0], actual_bits_count[0][mode]);
        g[0] = bcdec__extend_sign(g[0], actual_bits_count[0][mode]);
        b[0] = bcdec__extend_sign(b[0], actual_bits_count[0][mode]);
    }

    /* Mode 11 (like Mode 10) does not use delta compression,
       and instead stores both color endpoints explicitly.  */
    if ((mode != 9 && mode != 10) || isSigned) {
        for (i = 1; i < (numPartitions + 1) * 2; ++i) {
            r[i] = bcdec__extend_sign(r[i], actual_bits_count[1][mode]);
            g[i] = bcdec__extend_sign(g[i], actual_bits_count[2][mode]);
            b[i] = bcdec__extend_sign(b[i], actual_bits_count[3][mode]);
        }
    }

    if (mode != 9 && mode != 10) {
        for (i = 1; i < (numPartitions + 1) * 2; ++i) {
            r[i] = bcdec__transform_inverse(r[i], r[0], actual_bits_count[0][mode], isSigned);
            g[i] = bcdec__transform_inverse(g[i], g[0], actual_bits_count[0][mode], isSigned);
            b[i] = bcdec__transform_inverse(b[i], b[0], actual_bits_count[0][mode], isSigned);
        }
    }

    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            partitionSet = (mode >= 10) ? ((i|j) ? 0 : 128) : partition_sets[partition][i][j];

            indexBits = (mode >= 10) ? 4 : 3;
            /* fix-up index is specified with one less bit */
            /* The fix-up index for subset 0 is always index 0 */
            if (partitionSet & 0x80) {
                indexBits--;
            }
            partitionSet &= 0x01;

            index = bcdec__bitstream_read_bits(&bstream, indexBits);

            epR[0] = bcdec__unquantize(r[partitionSet * 2 + 0], actual_bits_count[0][mode], isSigned);
            epG[0] = bcdec__unquantize(g[partitionSet * 2 + 0], actual_bits_count[0][mode], isSigned);
            epB[0] = bcdec__unquantize(b[partitionSet * 2 + 0], actual_bits_count[0][mode], isSigned);
            epR[1] = bcdec__unquantize(r[partitionSet * 2 + 1], actual_bits_count[0][mode], isSigned);
            epG[1] = bcdec__unquantize(g[partitionSet * 2 + 1], actual_bits_count[0][mode], isSigned);
            epB[1] = bcdec__unquantize(b[partitionSet * 2 + 1], actual_bits_count[0][mode], isSigned);

            decompressed[j * 3 + 0] = bcdec__half_to_float_quick(
                                        bcdec__finish_unquantize(
                                            bcdec__interpolate(epR[0], epR[1], (mode >= 10) ? aWeight4 : aWeight3, index), isSigned));
            decompressed[j * 3 + 1] = bcdec__half_to_float_quick(
                                        bcdec__finish_unquantize(
                                            bcdec__interpolate(epG[0], epG[1], (mode >= 10) ? aWeight4 : aWeight3, index), isSigned));
            decompressed[j * 3 + 2] = bcdec__half_to_float_quick(
                                        bcdec__finish_unquantize(
                                            bcdec__interpolate(epB[0], epB[1], (mode >= 10) ? aWeight4 : aWeight3, index), isSigned));
        }

        decompressed += destinationPitch;
    }
}

#endif /* BCDEC_IMPLEMENTATION */

#endif /* BCDEC_HEADER_INCLUDED */
