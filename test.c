#include <stdio.h>
#include <stdlib.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION 1
#include "stb_image_write.h"

#define BCDEC_IMPLEMENTATION 1
#include "bcdec.h"

#define BCDEC_FOURCC(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

#define BCDEC_FOURCC_DDS    BCDEC_FOURCC('D', 'D', 'S', ' ')
#define BCDEC_FOURCC_DXT1   BCDEC_FOURCC('D', 'X', 'T', '1')
#define BCDEC_FOURCC_DXT3   BCDEC_FOURCC('D', 'X', 'T', '3')
#define BCDEC_FOURCC_DXT5   BCDEC_FOURCC('D', 'X', 'T', '5')
#define BCDEC_FOURCC_DX10   BCDEC_FOURCC('D', 'X', '1', '0')

#define DXGI_FORMAT_BC4_UNORM   80
#define DXGI_FORMAT_BC5_UNORM   83
#define DXGI_FORMAT_BC6H_UF16   95
#define DXGI_FORMAT_BC6H_SF16   96
#define DXGI_FORMAT_BC7_UNORM   98


typedef struct DDCOLORKEY {
    unsigned int    dwUnused0;
    unsigned int    dwUnused1;
} DDCOLORKEY_t;

typedef struct DDPIXELFORMAT {
    unsigned int    dwSize;
    unsigned int    dwFlags;
    unsigned int    dwFourCC;
    unsigned int    dwRGBBitCount;
    unsigned int    dwRBitMask;
    unsigned int    dwGBitMask;
    unsigned int    dwBBitMask;
    unsigned int    dwRGBAlphaBitMask;
} DDPIXELFORMAT_t;

typedef struct DDSCAPS2 {
    unsigned int    dwCaps;
    unsigned int    dwCaps2;
    unsigned int    dwCaps3;
    unsigned int    dwCaps4;
} DDSCAPS2_t;

typedef struct DDSURFACEDESC2 {
    unsigned int    dwSize;
    unsigned int    dwFlags;
    unsigned int    dwHeight;
    unsigned int    dwWidth;
    union {
        int         lPitch;
        unsigned int dwLinearSize;
    };
    unsigned int    dwBackBufferCount;
    unsigned int    dwMipMapCount;
    unsigned int    dwAlphaBitDepth;
    unsigned int    dwUnused0;
    unsigned int    lpSurface;
    DDCOLORKEY_t    unused0;
    DDCOLORKEY_t    unused1;
    DDCOLORKEY_t    unused2;
    DDCOLORKEY_t    unused3;
    DDPIXELFORMAT_t ddpfPixelFormat;
    DDSCAPS2_t      ddsCaps;
    unsigned int    dwUnused1;
} DDSURFACEDESC2_t;

typedef struct DDS_HEADER_DXT10 {
    unsigned int    dxgiFormat;
    unsigned int    resourceDimension;
    unsigned int    miscFlag;
    unsigned int    arraySize;
    unsigned int    miscFlags2;
} DDS_HEADER_DXT10_t;

int load_dds(const char* filePath, int* w, int* h, unsigned int* fourcc, void** compressedData) {
    unsigned int magic, compressedSize;
    DDSURFACEDESC2_t ddsDesc;
    DDS_HEADER_DXT10_t dx10Desc;

    FILE* f = fopen(filePath, "rb");
    if (!f) {
        return 0;
    }

    fread(&magic, 1, 4, f);
    if (BCDEC_FOURCC_DDS != magic) {
        return 0;
    }

    fread(&ddsDesc, 1, sizeof(ddsDesc), f);

    *w = ddsDesc.dwWidth;
    *h = ddsDesc.dwHeight;
    *fourcc = ddsDesc.ddpfPixelFormat.dwFourCC;

    if (ddsDesc.ddpfPixelFormat.dwFourCC == BCDEC_FOURCC_DXT1) {
        compressedSize = BCDEC_BC1_COMPRESSED_SIZE(ddsDesc.dwWidth, ddsDesc.dwHeight);
    } else if (ddsDesc.ddpfPixelFormat.dwFourCC == BCDEC_FOURCC_DXT3 || ddsDesc.ddpfPixelFormat.dwFourCC == BCDEC_FOURCC_DXT5) {
        compressedSize = BCDEC_BC3_COMPRESSED_SIZE(ddsDesc.dwWidth, ddsDesc.dwHeight);
    } else if (ddsDesc.ddpfPixelFormat.dwFourCC == BCDEC_FOURCC_DX10) {
        fread(&dx10Desc, 1, sizeof(dx10Desc), f);

        *fourcc = dx10Desc.dxgiFormat;

        if (dx10Desc.dxgiFormat == DXGI_FORMAT_BC4_UNORM) {
            compressedSize = BCDEC_BC4_COMPRESSED_SIZE(ddsDesc.dwWidth, ddsDesc.dwHeight);
        } else if (dx10Desc.dxgiFormat == DXGI_FORMAT_BC5_UNORM) {
            compressedSize = BCDEC_BC5_COMPRESSED_SIZE(ddsDesc.dwWidth, ddsDesc.dwHeight);
        } else if (dx10Desc.dxgiFormat == DXGI_FORMAT_BC6H_UF16 || dx10Desc.dxgiFormat == DXGI_FORMAT_BC6H_SF16) {
            compressedSize = BCDEC_BC6H_COMPRESSED_SIZE(ddsDesc.dwWidth, ddsDesc.dwHeight);
        } else if (dx10Desc.dxgiFormat == DXGI_FORMAT_BC7_UNORM) {
            compressedSize = BCDEC_BC7_COMPRESSED_SIZE(ddsDesc.dwWidth, ddsDesc.dwHeight);
        } else {
            return 0;
        }
    } else {
        return 0;
    }

    *compressedData = malloc(compressedSize);
    fread(*compressedData, 1, compressedSize, f);
    fclose(f);

    return 1;
}

int main(int argc, char** argv) {
    int w, h, fourcc, i, j;
    char* compData, * uncompData, * src, * dst;
    float* uncompDataHDR, * dstHDR;;

    uncompData = 0;
    uncompDataHDR = 0;
    if (load_dds("d:\\temp\\dice_bc7.dds", &w, &h, &fourcc,(void**)&compData)) {
        switch (fourcc) {
            case BCDEC_FOURCC_DXT1:
            case BCDEC_FOURCC_DXT3:
            case BCDEC_FOURCC_DXT5:
            case DXGI_FORMAT_BC7_UNORM: {
                uncompData = (char*)malloc(w * h * 4);

                src = compData;
                dst = uncompData;

                for (i = 0; i < h; i += 4) {
                    for (j = 0; j < w; j += 4) {
                        dst = uncompData + (i * w + j) * 4;
                        if (fourcc == BCDEC_FOURCC_DXT1) {
                            bcdec_bc1(src, dst, w * 4);
                            src += BCDEC_BC1_BLOCK_SIZE;
                        } else if (fourcc == BCDEC_FOURCC_DXT3) {
                            bcdec_bc2(src, dst, w * 4);
                            src += BCDEC_BC2_BLOCK_SIZE;
                        } else if (fourcc == BCDEC_FOURCC_DXT5) {
                            bcdec_bc3(src, dst, w * 4);
                            src += BCDEC_BC3_BLOCK_SIZE;
                        } else if (fourcc == DXGI_FORMAT_BC7_UNORM) {
                            bcdec_bc7(src, dst, w * 4);
                            src += BCDEC_BC7_BLOCK_SIZE;
                        }
                    }
                }

                stbi_write_tga("d:\\temp\\dice_bc7.tga", w, h, 4, uncompData);
            } break;

            case DXGI_FORMAT_BC6H_UF16:
            case DXGI_FORMAT_BC6H_SF16: {
                uncompDataHDR = (float*)malloc(w * h * 12);

                src = compData;
                dstHDR = uncompDataHDR;

                for (i = 0; i < h; i += 4) {
                    for (j = 0; j < w; j += 4) {
                        dstHDR = uncompDataHDR + (i * w + j) * 3;
                        bcdec_bc6h(src, dstHDR, w * 3, fourcc == DXGI_FORMAT_BC6H_SF16);
                        src += BCDEC_BC6H_BLOCK_SIZE;
                    }
                }

                stbi_write_hdr("d:\\temp\\bc6hs.hdr", w, h, 3, uncompDataHDR);
            } break;
        }

        free(compData);
        free(uncompData);
        free(uncompDataHDR);
    }

    return 0;
}
