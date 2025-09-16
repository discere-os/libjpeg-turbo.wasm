/*
 * Copyright © 2025 Superstruct Ltd, New Zealand
 * Licensed under libjpeg-turbo licenses (IJG + Modified BSD)
 *
 * WASM-Native LibJPEG-Turbo Implementation
 * High-performance JPEG compression/decompression with SIMD acceleration
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>
#include "../src/turbojpeg.h"

// Global TurboJPEG handles for compression and decompression
static tjhandle compress_handle = NULL;
static tjhandle decompress_handle = NULL;

// Forward declarations for SIMD functions
void jpeg_decode_simd_init(void);
const char* jpeg_simd_get_metrics(void);

// Initialize JPEG decoder with SIMD optimizations
EMSCRIPTEN_KEEPALIVE
void jpeg_wasm_init(void) {
    // Initialize TurboJPEG handles
    compress_handle = tjInitCompress();
    decompress_handle = tjInitDecompress();

    if (!compress_handle || !decompress_handle) {
        return;
    }

    // Initialize SIMD subsystem
    jpeg_decode_simd_init();
}

// Main JPEG decoding function using real libjpeg-turbo
EMSCRIPTEN_KEEPALIVE
int jpeg_decode_from_memory(const unsigned char* jpeg_data, size_t jpeg_size,
                           unsigned char** rgb_data, int* width, int* height, int* components) {
    if (!jpeg_data || jpeg_size == 0 || !rgb_data || !width || !height || !components) {
        return -1; // Invalid parameters
    }

    if (!decompress_handle) {
        return -1;
    }

    int subsamp, colorspace;

    // Get JPEG header information
    if (tjDecompressHeader3(decompress_handle, (unsigned char*)jpeg_data, jpeg_size,
                           width, height, &subsamp, &colorspace) != 0) {
        return -1;
    }

    // Calculate buffer size for RGB output
    *components = 3; // RGB
    size_t rgb_size = (*width) * (*height) * (*components);

    // Allocate output buffer
    *rgb_data = (unsigned char*)malloc(rgb_size);
    if (!*rgb_data) {
        return -1;
    }

    // Decompress JPEG to RGB
    if (tjDecompress2(decompress_handle, (unsigned char*)jpeg_data, jpeg_size,
                     *rgb_data, *width, 0, *height, TJPF_RGB, 0) != 0) {
        free(*rgb_data);
        *rgb_data = NULL;
        return -1;
    }

    return 0; // Success
}

// Get JPEG information without full decoding using real libjpeg-turbo
EMSCRIPTEN_KEEPALIVE
int jpeg_get_info(const unsigned char* jpeg_data, size_t jpeg_size,
                 int* width, int* height, int* components) {
    if (!jpeg_data || jpeg_size < 10) {
        return -1; // Invalid JPEG data
    }

    if (!decompress_handle) {
        return -1;
    }

    int subsamp, colorspace;

    // Get JPEG header information using TurboJPEG
    if (tjDecompressHeader3(decompress_handle, (unsigned char*)jpeg_data, jpeg_size,
                           width, height, &subsamp, &colorspace) != 0) {
        return -1;
    }

    // Determine components based on colorspace
    switch (colorspace) {
        case TJCS_RGB:
        case TJCS_YCbCr:
            *components = 3;
            break;
        case TJCS_GRAY:
            *components = 1;
            break;
        case TJCS_CMYK:
        case TJCS_YCCK:
            *components = 4;
            break;
        default:
            *components = 3; // Default to RGB
            break;
    }

    return 0; // Success
}

// Encode RGB data to JPEG using real libjpeg-turbo
EMSCRIPTEN_KEEPALIVE
int jpeg_encode_to_memory(const unsigned char* rgb_data, int width, int height,
                         int quality, unsigned char** jpeg_data, size_t* jpeg_size) {
    if (!rgb_data || width <= 0 || height <= 0 || !jpeg_data || !jpeg_size) {
        return -1; // Invalid parameters
    }

    if (!compress_handle) {
        return -1;
    }

    // Clamp quality to valid range
    if (quality < 1) quality = 1;
    if (quality > 100) quality = 100;

    // Compress RGB data to JPEG
    unsigned long output_size = 0;
    if (tjCompress2(compress_handle, rgb_data, width, 0, height, TJPF_RGB,
                   jpeg_data, &output_size, TJSAMP_444, quality, 0) != 0) {
        return -1;
    }

    *jpeg_size = (size_t)output_size;

    return 0; // Success
}

// Memory management for JavaScript
EMSCRIPTEN_KEEPALIVE
void* jpeg_malloc(size_t size) {
    return malloc(size);
}

EMSCRIPTEN_KEEPALIVE
void jpeg_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

// Cleanup function
EMSCRIPTEN_KEEPALIVE
void jpeg_cleanup(void) {
    if (compress_handle) {
        tjDestroy(compress_handle);
        compress_handle = NULL;
    }
    if (decompress_handle) {
        tjDestroy(decompress_handle);
        decompress_handle = NULL;
    }
}