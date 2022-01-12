#ifndef BCDEC_HEADER_INCLUDED
#define BCDEC_HEADER_INCLUDED

/*  Used information sources:
    https://docs.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-block-compression
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

void bcdec_bc1(void* compressedBlock, void* decompressedBlock, int destinationPitch);
void bcdec_bc2(void* compressedBlock, void* decompressedBlock, int destinationPitch);
void bcdec_bc3(void* compressedBlock, void* decompressedBlock, int destinationPitch);
void bcdec_bc4(void* compressedBlock, void* decompressedBlock, int destinationPitch);
void bcdec_bc5(void* compressedBlock, void* decompressedBlock, int destinationPitch);

#ifdef BCDEC_IMPLEMENTATION

typedef struct bcdecRGBA {
    unsigned char r, g, b, a;
} bcdecRGBA_t;


void bcdec__color_block(void* compressedBlock, void* decompressedBlock, int destinationPitch, int onlyOpaqueMode) {
    unsigned short c0, c1;
    bcdecRGBA_t refColors[4];
    unsigned char* dstColors;
    unsigned char* colorIndices;
    int i, j, idx;

    refColors[0].a = refColors[1].a = refColors[2].a = refColors[3].a = 0xFF;

    c0 = ((unsigned short*)compressedBlock)[0];
    c1 = ((unsigned short*)compressedBlock)[1];

    /* Expand 565 ref colors to 888 */
    refColors[0].r = (((c0 >> 11) & 0x1F) * 527 + 23) >> 6;
    refColors[0].g = (((c0 >> 5) & 0x3F) * 259 + 33) >> 6;
    refColors[0].b = ((c0 & 0x1F) * 527 + 23) >> 6;

    refColors[1].r = (((c1 >> 11) & 0x1F) * 527 + 23) >> 6;
    refColors[1].g = (((c1 >> 5) & 0x3F) * 259 + 33) >> 6;
    refColors[1].b = ((c1 & 0x1F) * 527 + 23) >> 6;

    if (c0 > c1 || onlyOpaqueMode) {  /* Standard BC1 mode (also BC3 color block uses ONLY this mode) */
        /* color_2 = 2/3*color_0 + 1/3*color_1
           color_3 = 1/3*color_0 + 2/3*color_1 */
        refColors[2].r = (2 * refColors[0].r + refColors[1].r + 1) / 3;
        refColors[2].g = (2 * refColors[0].g + refColors[1].g + 1) / 3;
        refColors[2].b = (2 * refColors[0].b + refColors[1].b + 1) / 3;

        refColors[3].r = (refColors[0].r + 2 * refColors[1].r + 1) / 3;
        refColors[3].g = (refColors[0].g + 2 * refColors[1].g + 1) / 3;
        refColors[3].b = (refColors[0].b + 2 * refColors[1].b + 1) / 3;
    }
    else {        /* Quite rare BC1A mode */
     /* color_2 = 1/2*color_0 + 1/2*color_1;
        color_3 = 0; */
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
        alpha[2] = (6 * alpha[0] + alpha[1] + 1) / 7;   /* 6/7*alpha_0 + 1/7*alpha_1 */
        alpha[3] = (5 * alpha[0] + 2 * alpha[1] + 1) / 7;   /* 5/7*alpha_0 + 2/7*alpha_1 */
        alpha[4] = (4 * alpha[0] + 3 * alpha[1] + 1) / 7;   /* 4/7*alpha_0 + 3/7*alpha_1 */
        alpha[5] = (3 * alpha[0] + 4 * alpha[1] + 1) / 7;   /* 3/7*alpha_0 + 4/7*alpha_1 */
        alpha[6] = (2 * alpha[0] + 5 * alpha[1] + 1) / 7;   /* 2/7*alpha_0 + 5/7*alpha_1 */
        alpha[7] = (alpha[0] + 6 * alpha[1] + 1) / 7;   /* 1/7*alpha_0 + 6/7*alpha_1 */
    }
    else {
        /* 4 interpolated alpha values. */
        alpha[2] = (4 * alpha[0] + alpha[1]) / 5;       /* 4/5*alpha_0 + 1/5*alpha_1 */
        alpha[3] = (3 * alpha[0] + 2 * alpha[1]) / 5;       /* 3/5*alpha_0 + 2/5*alpha_1 */
        alpha[4] = (2 * alpha[0] + 3 * alpha[1]) / 5;       /* 2/5*alpha_0 + 3/5*alpha_1 */
        alpha[5] = (alpha[0] + 4 * alpha[1]) / 5;       /* 1/5*alpha_0 + 4/5*alpha_1 */
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

#endif /* BCDEC_IMPLEMENTATION */

#endif /* BCDEC_HEADER_INCLUDED */
