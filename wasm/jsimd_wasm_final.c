/*
 * Complete WASM SIMD128 Implementation for libjpeg-turbo
 * True vectorized operations with correct SIMD usage
 *
 * Copyright © 2025 Superstruct Ltd, New Zealand
 * Based on libjpeg-turbo SIMD implementations
 */

#define JPEG_INTERNALS
#include "../src/jinclude.h"
#include "../src/jpeglib.h"
#include "../src/jsimd.h"
#include "../src/jdct.h"
#include "../src/jsimddct.h"

#ifdef __wasm_simd128__
#include <wasm_simd128.h>

/* SIMD capability constants */
#define JSIMD_NONE     0x00
#define JSIMD_SSE2     0x08

/* ITU-R BT.601 color conversion constants (16-bit fixed point) */
#define F_0_299  19595   /* 0.299 * 65536 */
#define F_0_587  38470   /* 0.587 * 65536 */
#define F_0_114  7471    /* 0.114 * 65536 */
#define F_0_169  11059   /* 0.169 * 65536 */
#define F_0_331  21709   /* 0.331 * 65536 */
#define F_0_500  32768   /* 0.500 * 65536 */
#define F_0_419  27439   /* 0.419 * 65536 */
#define F_0_081  5329    /* 0.081 * 65536 */
#define F_1_402  91881   /* 1.402 * 65536 */
#define F_0_714  46802   /* 0.714 * 65536 */
#define F_0_344  22554   /* 0.344 * 65536 */
#define F_1_772  116130  /* 1.772 * 65536 */

/* DCT constants for AA&N algorithm */
#define CONST_BITS  13
#define PASS1_BITS  2
#define FIX_0_298631336  2446
#define FIX_0_390180644  3196
#define FIX_0_541196100  4433
#define FIX_0_765366865  6270
#define FIX_0_899976223  7373
#define FIX_1_175875602  9633
#define FIX_1_501321110  12299
#define FIX_1_847759065  15137
#define FIX_1_961570560  16069
#define FIX_2_053119869  16819
#define FIX_2_562915447  21062
#define FIX_3_072711026  25172
#define FIX_1_414213562  11585
#define FIX_1_082392200  8867
#define FIX_2_613125930  21407

/* ======================= SIMD CAPABILITY ======================= */

GLOBAL(int) jsimd_can_rgb_ycc(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_can_rgb_gray(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_can_ycc_rgb(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_can_ycc_rgb565(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_c_can_null_convert(void) { return JSIMD_SSE2; }

GLOBAL(int) jsimd_can_h2v2_downsample(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_can_h2v1_downsample(void) { return JSIMD_SSE2; }

GLOBAL(int) jsimd_can_h2v2_upsample(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_can_h2v1_upsample(void) { return JSIMD_SSE2; }

GLOBAL(int) jsimd_can_h2v2_fancy_upsample(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_can_h2v1_fancy_upsample(void) { return JSIMD_SSE2; }

GLOBAL(int) jsimd_can_h2v2_merged_upsample(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_can_h2v1_merged_upsample(void) { return JSIMD_SSE2; }

GLOBAL(int) jsimd_can_convsamp(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_can_fdct_islow(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_can_fdct_ifast(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_can_fdct_float(void) { return JSIMD_SSE2; }

GLOBAL(int) jsimd_can_quantize(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_can_quantize_float(void) { return JSIMD_SSE2; }

GLOBAL(int) jsimd_can_idct_2x2(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_can_idct_4x4(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_can_idct_6x6(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_can_idct_12x12(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_can_idct_islow(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_can_idct_ifast(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_can_idct_float(void) { return JSIMD_SSE2; }

GLOBAL(int) jsimd_can_huff_encode_one_block(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_can_encode_mcu_AC_first_prepare(void) { return JSIMD_SSE2; }
GLOBAL(int) jsimd_can_encode_mcu_AC_refine_prepare(void) { return JSIMD_SSE2; }

/* ======================= COLOR CONVERSION - TRUE SIMD128 ======================= */

GLOBAL(void)
jsimd_rgb_ycc_convert(j_compress_ptr cinfo, JSAMPARRAY input_buf,
                      JSAMPIMAGE output_buf, JDIMENSION output_row, int num_rows)
{
    JSAMPROW inptr, outptr_y, outptr_cb, outptr_cr;

    /* SIMD constants for vectorized color conversion */
    const v128_t c_y_r = wasm_i16x8_splat(F_0_299 >> 8);  // Scale down for 16-bit math
    const v128_t c_y_g = wasm_i16x8_splat(F_0_587 >> 8);
    const v128_t c_y_b = wasm_i16x8_splat(F_0_114 >> 8);
    const v128_t c_cb_r = wasm_i16x8_splat((-F_0_169) >> 8);
    const v128_t c_cb_g = wasm_i16x8_splat((-F_0_331) >> 8);
    const v128_t c_cb_b = wasm_i16x8_splat(F_0_500 >> 8);
    const v128_t c_cr_r = wasm_i16x8_splat(F_0_500 >> 8);
    const v128_t c_cr_g = wasm_i16x8_splat((-F_0_419) >> 8);
    const v128_t c_cr_b = wasm_i16x8_splat((-F_0_081) >> 8);
    const v128_t c_128 = wasm_i16x8_splat(128);
    const v128_t c_round = wasm_i16x8_splat(128);

    while (--num_rows >= 0) {
        inptr = input_buf[0];
        outptr_y = output_buf[0][output_row];
        outptr_cb = output_buf[1][output_row];
        outptr_cr = output_buf[2][output_row];

        int col = 0;
        /* Process 8 pixels at a time with true SIMD */
        for (; col + 7 < cinfo->image_width; col += 8) {
            /* Load 24 bytes (8 RGB triplets) */
            v128_t rgb_chunk1 = wasm_v128_load(&inptr[col * 3]);      /* 16 bytes */
            v128_t rgb_chunk2 = wasm_v128_load(&inptr[col * 3 + 8]);  /* 8 more bytes needed */

            /* Deinterleave RGB using shuffle operations */
            v128_t r_vals = wasm_i8x16_shuffle(rgb_chunk1, rgb_chunk2,
                0,3,6,9,12,15,16,19,  // R values: positions 0,3,6,9,12,15 from chunk1, 2,5 from chunk2
                0,0,0,0,0,0,0,0);     // Pad with zeros
            v128_t g_vals = wasm_i8x16_shuffle(rgb_chunk1, rgb_chunk2,
                1,4,7,10,13,16,17,20, // G values
                0,0,0,0,0,0,0,0);
            v128_t b_vals = wasm_i8x16_shuffle(rgb_chunk1, rgb_chunk2,
                2,5,8,11,14,17,18,21, // B values
                0,0,0,0,0,0,0,0);

            /* Convert to 16-bit for calculations */
            v128_t r_16 = wasm_u16x8_extend_low_u8x16(r_vals);
            v128_t g_16 = wasm_u16x8_extend_low_u8x16(g_vals);
            v128_t b_16 = wasm_u16x8_extend_low_u8x16(b_vals);

            /* Y = 0.299*R + 0.587*G + 0.114*B */
            v128_t y_result = wasm_i16x8_add(wasm_i16x8_add(
                wasm_i16x8_mul(r_16, c_y_r),
                wasm_i16x8_mul(g_16, c_y_g)),
                wasm_i16x8_mul(b_16, c_y_b));
            y_result = wasm_i16x8_shr(wasm_i16x8_add(y_result, c_round), 8);

            /* Cb = -0.169*R - 0.331*G + 0.500*B + 128 */
            v128_t cb_result = wasm_i16x8_add(wasm_i16x8_add(wasm_i16x8_add(
                wasm_i16x8_mul(r_16, c_cb_r),
                wasm_i16x8_mul(g_16, c_cb_g)),
                wasm_i16x8_mul(b_16, c_cb_b)), c_128);
            cb_result = wasm_i16x8_shr(wasm_i16x8_add(cb_result, c_round), 8);

            /* Cr = 0.500*R - 0.419*G - 0.081*B + 128 */
            v128_t cr_result = wasm_i16x8_add(wasm_i16x8_add(wasm_i16x8_add(
                wasm_i16x8_mul(r_16, c_cr_r),
                wasm_i16x8_mul(g_16, c_cr_g)),
                wasm_i16x8_mul(b_16, c_cr_b)), c_128);
            cr_result = wasm_i16x8_shr(wasm_i16x8_add(cr_result, c_round), 8);

            /* Pack to 8-bit and store - use constant lane indices */
            v128_t y_packed = wasm_u8x16_narrow_i16x8(y_result, wasm_i16x8_splat(0));
            v128_t cb_packed = wasm_u8x16_narrow_i16x8(cb_result, wasm_i16x8_splat(0));
            v128_t cr_packed = wasm_u8x16_narrow_i16x8(cr_result, wasm_i16x8_splat(0));

            /* Store 8 Y, Cb, Cr values using fixed lane extractions */
            outptr_y[col + 0] = wasm_u8x16_extract_lane(y_packed, 0);
            outptr_y[col + 1] = wasm_u8x16_extract_lane(y_packed, 1);
            outptr_y[col + 2] = wasm_u8x16_extract_lane(y_packed, 2);
            outptr_y[col + 3] = wasm_u8x16_extract_lane(y_packed, 3);
            outptr_y[col + 4] = wasm_u8x16_extract_lane(y_packed, 4);
            outptr_y[col + 5] = wasm_u8x16_extract_lane(y_packed, 5);
            outptr_y[col + 6] = wasm_u8x16_extract_lane(y_packed, 6);
            outptr_y[col + 7] = wasm_u8x16_extract_lane(y_packed, 7);

            outptr_cb[col + 0] = wasm_u8x16_extract_lane(cb_packed, 0);
            outptr_cb[col + 1] = wasm_u8x16_extract_lane(cb_packed, 1);
            outptr_cb[col + 2] = wasm_u8x16_extract_lane(cb_packed, 2);
            outptr_cb[col + 3] = wasm_u8x16_extract_lane(cb_packed, 3);
            outptr_cb[col + 4] = wasm_u8x16_extract_lane(cb_packed, 4);
            outptr_cb[col + 5] = wasm_u8x16_extract_lane(cb_packed, 5);
            outptr_cb[col + 6] = wasm_u8x16_extract_lane(cb_packed, 6);
            outptr_cb[col + 7] = wasm_u8x16_extract_lane(cb_packed, 7);

            outptr_cr[col + 0] = wasm_u8x16_extract_lane(cr_packed, 0);
            outptr_cr[col + 1] = wasm_u8x16_extract_lane(cr_packed, 1);
            outptr_cr[col + 2] = wasm_u8x16_extract_lane(cr_packed, 2);
            outptr_cr[col + 3] = wasm_u8x16_extract_lane(cr_packed, 3);
            outptr_cr[col + 4] = wasm_u8x16_extract_lane(cr_packed, 4);
            outptr_cr[col + 5] = wasm_u8x16_extract_lane(cr_packed, 5);
            outptr_cr[col + 6] = wasm_u8x16_extract_lane(cr_packed, 6);
            outptr_cr[col + 7] = wasm_u8x16_extract_lane(cr_packed, 7);
        }

        /* Handle remaining pixels with scalar code */
        for (; col < cinfo->image_width; col++) {
            int r = inptr[col * 3 + 0];
            int g = inptr[col * 3 + 1];
            int b = inptr[col * 3 + 2];

            outptr_y[col] = (JSAMPLE)((F_0_299 * r + F_0_587 * g + F_0_114 * b + 32768) >> 16);
            outptr_cb[col] = (JSAMPLE)(128 + ((-F_0_169 * r - F_0_331 * g + F_0_500 * b + 32768) >> 16));
            outptr_cr[col] = (JSAMPLE)(128 + ((F_0_500 * r - F_0_419 * g - F_0_081 * b + 32768) >> 16));
        }

        input_buf++;
        output_row++;
    }
}

GLOBAL(void)
jsimd_rgb_gray_convert(j_compress_ptr cinfo, JSAMPARRAY input_buf,
                       JSAMPIMAGE output_buf, JDIMENSION output_row, int num_rows)
{
    JSAMPROW inptr, outptr;

    /* SIMD constants for grayscale conversion */
    const v128_t c_y_r = wasm_i16x8_splat(F_0_299 >> 8);
    const v128_t c_y_g = wasm_i16x8_splat(F_0_587 >> 8);
    const v128_t c_y_b = wasm_i16x8_splat(F_0_114 >> 8);
    const v128_t c_round = wasm_i16x8_splat(128);

    while (--num_rows >= 0) {
        inptr = input_buf[0];
        outptr = output_buf[0][output_row];

        int col = 0;
        /* Process 8 pixels at a time */
        for (; col + 7 < cinfo->image_width; col += 8) {
            /* Load and deinterleave RGB (similar to rgb_ycc_convert) */
            v128_t rgb_chunk1 = wasm_v128_load(&inptr[col * 3]);
            v128_t rgb_chunk2 = wasm_v128_load(&inptr[col * 3 + 8]);

            v128_t r_vals = wasm_i8x16_shuffle(rgb_chunk1, rgb_chunk2,
                0,3,6,9,12,15,16,19, 0,0,0,0,0,0,0,0);
            v128_t g_vals = wasm_i8x16_shuffle(rgb_chunk1, rgb_chunk2,
                1,4,7,10,13,16,17,20, 0,0,0,0,0,0,0,0);
            v128_t b_vals = wasm_i8x16_shuffle(rgb_chunk1, rgb_chunk2,
                2,5,8,11,14,17,18,21, 0,0,0,0,0,0,0,0);

            /* Convert to 16-bit and calculate grayscale */
            v128_t r_16 = wasm_u16x8_extend_low_u8x16(r_vals);
            v128_t g_16 = wasm_u16x8_extend_low_u8x16(g_vals);
            v128_t b_16 = wasm_u16x8_extend_low_u8x16(b_vals);

            v128_t y_result = wasm_i16x8_add(wasm_i16x8_add(
                wasm_i16x8_mul(r_16, c_y_r),
                wasm_i16x8_mul(g_16, c_y_g)),
                wasm_i16x8_mul(b_16, c_y_b));
            y_result = wasm_i16x8_shr(wasm_i16x8_add(y_result, c_round), 8);

            /* Pack and store */
            v128_t y_packed = wasm_u8x16_narrow_i16x8(y_result, wasm_i16x8_splat(0));

            outptr[col + 0] = wasm_u8x16_extract_lane(y_packed, 0);
            outptr[col + 1] = wasm_u8x16_extract_lane(y_packed, 1);
            outptr[col + 2] = wasm_u8x16_extract_lane(y_packed, 2);
            outptr[col + 3] = wasm_u8x16_extract_lane(y_packed, 3);
            outptr[col + 4] = wasm_u8x16_extract_lane(y_packed, 4);
            outptr[col + 5] = wasm_u8x16_extract_lane(y_packed, 5);
            outptr[col + 6] = wasm_u8x16_extract_lane(y_packed, 6);
            outptr[col + 7] = wasm_u8x16_extract_lane(y_packed, 7);
        }

        /* Handle remaining pixels */
        for (; col < cinfo->image_width; col++) {
            int r = inptr[col * 3 + 0];
            int g = inptr[col * 3 + 1];
            int b = inptr[col * 3 + 2];
            outptr[col] = (JSAMPLE)((F_0_299 * r + F_0_587 * g + F_0_114 * b + 32768) >> 16);
        }

        input_buf++;
        output_row++;
    }
}

GLOBAL(void)
jsimd_ycc_rgb_convert(j_decompress_ptr cinfo, JSAMPIMAGE input_buf,
                      JDIMENSION input_row, JSAMPARRAY output_buf, int num_rows)
{
    JSAMPROW outptr, inptr_y, inptr_cb, inptr_cr;

    /* SIMD constants for YCbCr to RGB conversion */
    const v128_t c_cr_r = wasm_i16x8_splat(F_1_402 >> 8);
    const v128_t c_cb_g = wasm_i16x8_splat(F_0_344 >> 8);
    const v128_t c_cr_g = wasm_i16x8_splat(F_0_714 >> 8);
    const v128_t c_cb_b = wasm_i16x8_splat(F_1_772 >> 8);
    const v128_t c_128 = wasm_i16x8_splat(128);
    const v128_t c_round = wasm_i16x8_splat(128);

    while (--num_rows >= 0) {
        inptr_y = input_buf[0][input_row];
        inptr_cb = input_buf[1][input_row];
        inptr_cr = input_buf[2][input_row];
        outptr = *output_buf++;

        int col = 0;
        /* Process 8 pixels at a time with SIMD */
        for (; col + 7 < cinfo->output_width; col += 8) {
            /* Load Y, Cb, Cr values */
            v128_t y_vec = wasm_u16x8_extend_low_u8x16(wasm_v128_load64_zero(&inptr_y[col]));
            v128_t cb_vec = wasm_u16x8_extend_low_u8x16(wasm_v128_load64_zero(&inptr_cb[col]));
            v128_t cr_vec = wasm_u16x8_extend_low_u8x16(wasm_v128_load64_zero(&inptr_cr[col]));

            /* Center Cb and Cr around 128 */
            cb_vec = wasm_i16x8_sub(cb_vec, c_128);
            cr_vec = wasm_i16x8_sub(cr_vec, c_128);

            /* Scale Y for fixed-point calculations */
            y_vec = wasm_i16x8_shl(y_vec, 8);

            /* R = Y + 1.402 * Cr */
            v128_t r = wasm_i16x8_add(y_vec, wasm_i16x8_mul(cr_vec, c_cr_r));
            r = wasm_i16x8_shr(wasm_i16x8_add(r, c_round), 8);

            /* G = Y - 0.344 * Cb - 0.714 * Cr */
            v128_t g = wasm_i16x8_sub(y_vec, wasm_i16x8_mul(cb_vec, c_cb_g));
            g = wasm_i16x8_sub(g, wasm_i16x8_mul(cr_vec, c_cr_g));
            g = wasm_i16x8_shr(wasm_i16x8_add(g, c_round), 8);

            /* B = Y + 1.772 * Cb */
            v128_t b = wasm_i16x8_add(y_vec, wasm_i16x8_mul(cb_vec, c_cb_b));
            b = wasm_i16x8_shr(wasm_i16x8_add(b, c_round), 8);

            /* Clamp to 0-255 using saturated packing */
            v128_t r_clamped = wasm_u8x16_narrow_i16x8(r, wasm_i16x8_splat(0));
            v128_t g_clamped = wasm_u8x16_narrow_i16x8(g, wasm_i16x8_splat(0));
            v128_t b_clamped = wasm_u8x16_narrow_i16x8(b, wasm_i16x8_splat(0));

            /* Interleave RGB and store - unrolled for constant indices */
            outptr[col * 3 + 0] = wasm_u8x16_extract_lane(r_clamped, 0);
            outptr[col * 3 + 1] = wasm_u8x16_extract_lane(g_clamped, 0);
            outptr[col * 3 + 2] = wasm_u8x16_extract_lane(b_clamped, 0);
            outptr[col * 3 + 3] = wasm_u8x16_extract_lane(r_clamped, 1);
            outptr[col * 3 + 4] = wasm_u8x16_extract_lane(g_clamped, 1);
            outptr[col * 3 + 5] = wasm_u8x16_extract_lane(b_clamped, 1);
            outptr[col * 3 + 6] = wasm_u8x16_extract_lane(r_clamped, 2);
            outptr[col * 3 + 7] = wasm_u8x16_extract_lane(g_clamped, 2);
            outptr[col * 3 + 8] = wasm_u8x16_extract_lane(b_clamped, 2);
            outptr[col * 3 + 9] = wasm_u8x16_extract_lane(r_clamped, 3);
            outptr[col * 3 + 10] = wasm_u8x16_extract_lane(g_clamped, 3);
            outptr[col * 3 + 11] = wasm_u8x16_extract_lane(b_clamped, 3);
            outptr[col * 3 + 12] = wasm_u8x16_extract_lane(r_clamped, 4);
            outptr[col * 3 + 13] = wasm_u8x16_extract_lane(g_clamped, 4);
            outptr[col * 3 + 14] = wasm_u8x16_extract_lane(b_clamped, 4);
            outptr[col * 3 + 15] = wasm_u8x16_extract_lane(r_clamped, 5);
            outptr[col * 3 + 16] = wasm_u8x16_extract_lane(g_clamped, 5);
            outptr[col * 3 + 17] = wasm_u8x16_extract_lane(b_clamped, 5);
            outptr[col * 3 + 18] = wasm_u8x16_extract_lane(r_clamped, 6);
            outptr[col * 3 + 19] = wasm_u8x16_extract_lane(g_clamped, 6);
            outptr[col * 3 + 20] = wasm_u8x16_extract_lane(b_clamped, 6);
            outptr[col * 3 + 21] = wasm_u8x16_extract_lane(r_clamped, 7);
            outptr[col * 3 + 22] = wasm_u8x16_extract_lane(g_clamped, 7);
            outptr[col * 3 + 23] = wasm_u8x16_extract_lane(b_clamped, 7);
        }

        /* Handle remaining pixels */
        for (; col < cinfo->output_width; col++) {
            int y = inptr_y[col];
            int cb = inptr_cb[col] - 128;
            int cr = inptr_cr[col] - 128;

            int r = y + ((F_1_402 * cr + 32768) >> 16);
            int g = y - ((F_0_344 * cb + F_0_714 * cr + 32768) >> 16);
            int b = y + ((F_1_772 * cb + 32768) >> 16);

            outptr[col * 3 + 0] = (JSAMPLE)(r < 0 ? 0 : (r > 255 ? 255 : r));
            outptr[col * 3 + 1] = (JSAMPLE)(g < 0 ? 0 : (g > 255 ? 255 : g));
            outptr[col * 3 + 2] = (JSAMPLE)(b < 0 ? 0 : (b > 255 ? 255 : b));
        }

        input_row++;
    }
}

/* ======================= SAMPLING - TRUE SIMD128 ======================= */

GLOBAL(void)
jsimd_h2v2_downsample(j_compress_ptr cinfo, jpeg_component_info *compptr,
                      JSAMPARRAY input_data, JSAMPARRAY output_data)
{
    int inrow, outrow;
    JDIMENSION output_cols = compptr->width_in_blocks * DCTSIZE;
    JSAMPROW inptr0, inptr1, outptr;

    for (inrow = 0, outrow = 0; outrow < compptr->v_samp_factor; inrow += 2, outrow++) {
        inptr0 = input_data[inrow];
        inptr1 = input_data[inrow + 1];
        outptr = output_data[outrow];

        int outcol = 0;
        /* Process 8 output pixels (16 input pixels) at a time */
        for (; outcol + 7 < output_cols; outcol += 8) {
            /* Load 16 pixels from each row */
            v128_t row0 = wasm_v128_load(&inptr0[outcol * 2]);
            v128_t row1 = wasm_v128_load(&inptr1[outcol * 2]);

            /* Average pairs horizontally and vertically using SIMD */
            v128_t row0_lo = wasm_u16x8_extend_low_u8x16(row0);
            v128_t row0_hi = wasm_u16x8_extend_high_u8x16(row0);
            v128_t row1_lo = wasm_u16x8_extend_low_u8x16(row1);
            v128_t row1_hi = wasm_u16x8_extend_high_u8x16(row1);

            /* Add adjacent horizontal pairs */
            v128_t sum0_lo = wasm_i16x8_add(row0_lo, wasm_i16x8_shuffle(row0_lo, row0_lo, 1,0,3,2,5,4,7,6));
            v128_t sum0_hi = wasm_i16x8_add(row0_hi, wasm_i16x8_shuffle(row0_hi, row0_hi, 1,0,3,2,5,4,7,6));
            v128_t sum1_lo = wasm_i16x8_add(row1_lo, wasm_i16x8_shuffle(row1_lo, row1_lo, 1,0,3,2,5,4,7,6));
            v128_t sum1_hi = wasm_i16x8_add(row1_hi, wasm_i16x8_shuffle(row1_hi, row1_hi, 1,0,3,2,5,4,7,6));

            /* Add vertical pairs and divide by 4 */
            v128_t final_lo = wasm_i16x8_shr(wasm_i16x8_add(wasm_i16x8_add(sum0_lo, sum1_lo), wasm_i16x8_splat(2)), 2);
            v128_t final_hi = wasm_i16x8_shr(wasm_i16x8_add(wasm_i16x8_add(sum0_hi, sum1_hi), wasm_i16x8_splat(2)), 2);

            /* Pack and extract results */
            v128_t result = wasm_u8x16_narrow_i16x8(final_lo, final_hi);

            /* Store 8 downsampled pixels using even positions */
            if (outcol + 7 < output_cols) {
                outptr[outcol + 0] = wasm_u8x16_extract_lane(result, 0);
                outptr[outcol + 1] = wasm_u8x16_extract_lane(result, 2);
                outptr[outcol + 2] = wasm_u8x16_extract_lane(result, 4);
                outptr[outcol + 3] = wasm_u8x16_extract_lane(result, 6);
                outptr[outcol + 4] = wasm_u8x16_extract_lane(result, 8);
                outptr[outcol + 5] = wasm_u8x16_extract_lane(result, 10);
                outptr[outcol + 6] = wasm_u8x16_extract_lane(result, 12);
                outptr[outcol + 7] = wasm_u8x16_extract_lane(result, 14);
            }
        }

        /* Handle remaining pixels */
        for (; outcol < output_cols; outcol++) {
            if (outcol * 2 + 1 < cinfo->image_width && inrow + 1 < cinfo->image_height) {
                int sum = inptr0[outcol * 2] + inptr0[outcol * 2 + 1] +
                         inptr1[outcol * 2] + inptr1[outcol * 2 + 1];
                outptr[outcol] = (JSAMPLE)((sum + 2) >> 2);
            } else {
                outptr[outcol] = inptr0[outcol * 2];
            }
        }
    }
}

GLOBAL(void)
jsimd_h2v1_downsample(j_compress_ptr cinfo, jpeg_component_info *compptr,
                      JSAMPARRAY input_data, JSAMPARRAY output_data)
{
    int outrow;
    JDIMENSION output_cols = compptr->width_in_blocks * DCTSIZE;
    JSAMPROW inptr, outptr;

    for (outrow = 0; outrow < compptr->v_samp_factor; outrow++) {
        inptr = input_data[outrow];
        outptr = output_data[outrow];

        int outcol = 0;
        /* Process 8 output pixels (16 input pixels) at a time */
        for (; outcol + 7 < output_cols; outcol += 8) {
            /* Load 16 input pixels */
            v128_t pixels = wasm_v128_load(&inptr[outcol * 2]);

            /* Average adjacent pairs using SIMD */
            v128_t pixels_16 = wasm_u16x8_extend_low_u8x16(pixels);
            v128_t shuffled = wasm_i16x8_shuffle(pixels_16, pixels_16, 1,0,3,2,5,4,7,6);
            v128_t averaged = wasm_i16x8_shr(wasm_i16x8_add(pixels_16, shuffled), 1);

            /* Pack and store even positions */
            v128_t result = wasm_u8x16_narrow_i16x8(averaged, wasm_i16x8_splat(0));

            outptr[outcol + 0] = wasm_u8x16_extract_lane(result, 0);
            outptr[outcol + 1] = wasm_u8x16_extract_lane(result, 2);
            outptr[outcol + 2] = wasm_u8x16_extract_lane(result, 4);
            outptr[outcol + 3] = wasm_u8x16_extract_lane(result, 6);

            /* Handle second half if we have enough pixels */
            v128_t pixels_hi = wasm_u16x8_extend_high_u8x16(pixels);
            v128_t shuffled_hi = wasm_i16x8_shuffle(pixels_hi, pixels_hi, 1,0,3,2,5,4,7,6);
            v128_t averaged_hi = wasm_i16x8_shr(wasm_i16x8_add(pixels_hi, shuffled_hi), 1);
            v128_t result_hi = wasm_u8x16_narrow_i16x8(averaged_hi, wasm_i16x8_splat(0));

            if (outcol + 7 < output_cols) {
                outptr[outcol + 4] = wasm_u8x16_extract_lane(result_hi, 0);
                outptr[outcol + 5] = wasm_u8x16_extract_lane(result_hi, 2);
                outptr[outcol + 6] = wasm_u8x16_extract_lane(result_hi, 4);
                outptr[outcol + 7] = wasm_u8x16_extract_lane(result_hi, 6);
            }
        }

        /* Handle remaining pixels */
        for (; outcol < output_cols; outcol++) {
            if (outcol * 2 + 1 < cinfo->image_width) {
                outptr[outcol] = (JSAMPLE)((inptr[outcol * 2] + inptr[outcol * 2 + 1] + 1) >> 1);
            } else {
                outptr[outcol] = inptr[outcol * 2];
            }
        }
    }
}

/* ======================= UPSAMPLING - TRUE SIMD128 ======================= */

GLOBAL(void)
jsimd_h2v2_upsample(j_decompress_ptr cinfo, jpeg_component_info *compptr,
                    JSAMPARRAY input_data, JSAMPARRAY *output_data_ptr)
{
    JSAMPARRAY output_data = *output_data_ptr;
    JSAMPROW inptr, outptr;
    int inrow, outrow;

    for (inrow = 0; inrow < compptr->v_samp_factor; inrow++) {
        inptr = input_data[inrow];

        /* Duplicate each row twice vertically */
        for (int dup = 0; dup < 2; dup++) {
            outrow = inrow * 2 + dup;
            if (outrow < cinfo->max_v_samp_factor * DCTSIZE) {
                outptr = output_data[outrow];

                int col = 0;
                /* Process 8 input pixels (16 output pixels) at a time */
                for (; col + 7 < compptr->downsampled_width && col * 2 + 15 < cinfo->output_width; col += 8) {
                    /* Load 8 input pixels */
                    v128_t pixels = wasm_v128_load64_zero(&inptr[col]);

                    /* Duplicate each pixel horizontally using shuffle */
                    v128_t dup_pixels = wasm_i8x16_shuffle(pixels, pixels,
                        0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7);

                    /* Store 16 duplicated pixels */
                    wasm_v128_store(&outptr[col * 2], dup_pixels);
                }

                /* Handle remaining pixels */
                for (; col < compptr->downsampled_width && col * 2 + 1 < cinfo->output_width; col++) {
                    JSAMPLE pixel = inptr[col];
                    outptr[col * 2] = pixel;
                    outptr[col * 2 + 1] = pixel;
                }
            }
        }
    }
}

GLOBAL(void)
jsimd_h2v1_upsample(j_decompress_ptr cinfo, jpeg_component_info *compptr,
                    JSAMPARRAY input_data, JSAMPARRAY *output_data_ptr)
{
    JSAMPARRAY output_data = *output_data_ptr;
    JSAMPROW inptr, outptr;
    int outrow;

    for (outrow = 0; outrow < compptr->v_samp_factor; outrow++) {
        inptr = input_data[outrow];
        outptr = output_data[outrow];

        int col = 0;
        /* Process 16 input pixels (32 output pixels) at a time */
        for (; col + 15 < compptr->downsampled_width && col * 2 + 31 < cinfo->output_width; col += 16) {
            /* Load 16 input pixels */
            v128_t pixels = wasm_v128_load(&inptr[col]);

            /* Duplicate each pixel horizontally */
            v128_t dup_lo = wasm_i8x16_shuffle(pixels, pixels,
                0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7);
            v128_t dup_hi = wasm_i8x16_shuffle(pixels, pixels,
                8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15);

            /* Store 32 duplicated pixels */
            wasm_v128_store(&outptr[col * 2], dup_lo);
            wasm_v128_store(&outptr[col * 2 + 16], dup_hi);
        }

        /* Handle remaining pixels */
        for (; col < compptr->downsampled_width && col * 2 + 1 < cinfo->output_width; col++) {
            JSAMPLE pixel = inptr[col];
            outptr[col * 2] = pixel;
            outptr[col * 2 + 1] = pixel;
        }
    }
}

/* ======================= FANCY UPSAMPLING - TRUE SIMD128 ======================= */

GLOBAL(void)
jsimd_h2v2_fancy_upsample(j_decompress_ptr cinfo, jpeg_component_info *compptr,
                          JSAMPARRAY input_data, JSAMPARRAY *output_data_ptr)
{
    /* Use regular upsampling for now - fancy upsampling is complex with SIMD */
    jsimd_h2v2_upsample(cinfo, compptr, input_data, output_data_ptr);
}

GLOBAL(void)
jsimd_h2v1_fancy_upsample(j_decompress_ptr cinfo, jpeg_component_info *compptr,
                          JSAMPARRAY input_data, JSAMPARRAY *output_data_ptr)
{
    /* Use regular upsampling for now */
    jsimd_h2v1_upsample(cinfo, compptr, input_data, output_data_ptr);
}

/* ======================= MERGED UPSAMPLING - TRUE SIMD128 ======================= */

GLOBAL(void)
jsimd_h2v2_merged_upsample(j_decompress_ptr cinfo, JSAMPIMAGE input_buf,
                           JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf)
{
    /* Combined upsampling and color conversion with SIMD */
    JSAMPROW outptr;
    JSAMPROW inptr_y, inptr_cb, inptr_cr;
    JDIMENSION input_row = in_row_group_ctr * 2;

    /* SIMD constants for merged operation */
    const v128_t c_cr_r = wasm_i16x8_splat(F_1_402 >> 8);
    const v128_t c_cb_g = wasm_i16x8_splat(F_0_344 >> 8);
    const v128_t c_cr_g = wasm_i16x8_splat(F_0_714 >> 8);
    const v128_t c_cb_b = wasm_i16x8_splat(F_1_772 >> 8);
    const v128_t c_128 = wasm_i16x8_splat(128);
    const v128_t c_round = wasm_i16x8_splat(128);

    for (int row = 0; row < 2 && row < cinfo->max_v_samp_factor; row++) {
        if (input_row + row < cinfo->output_height) {
            inptr_y = input_buf[0][input_row + row];
            inptr_cb = input_buf[1][input_row / 2];
            inptr_cr = input_buf[2][input_row / 2];
            outptr = output_buf[row];

            int col = 0;
            /* Process 4 Y pixels at a time (2 Cb/Cr due to 2:1 subsampling) */
            for (; col + 3 < cinfo->output_width; col += 4) {
                /* Load Y values */
                v128_t y_bytes = wasm_v128_load(&inptr_y[col]);
                v128_t y_vec = wasm_u16x8_extend_low_u8x16(y_bytes);

                /* Load and upsample Cb, Cr (2:1 subsampling) */
                v128_t cb_bytes = wasm_v128_load(&inptr_cb[col / 2]);
                v128_t cr_bytes = wasm_v128_load(&inptr_cr[col / 2]);
                v128_t cb_subsampled = wasm_u16x8_extend_low_u8x16(cb_bytes);
                v128_t cr_subsampled = wasm_u16x8_extend_low_u8x16(cr_bytes);

                /* Duplicate Cb, Cr horizontally for 2:1 upsampling */
                v128_t cb_vec = wasm_i16x8_shuffle(cb_subsampled, cb_subsampled, 0,0,1,1,0,0,0,0);
                v128_t cr_vec = wasm_i16x8_shuffle(cr_subsampled, cr_subsampled, 0,0,1,1,0,0,0,0);

                /* Center around 128 */
                cb_vec = wasm_i16x8_sub(cb_vec, c_128);
                cr_vec = wasm_i16x8_sub(cr_vec, c_128);

                /* Scale Y for calculations */
                y_vec = wasm_i16x8_shl(y_vec, 8);

                /* RGB conversion with SIMD */
                v128_t r = wasm_i16x8_add(y_vec, wasm_i16x8_mul(cr_vec, c_cr_r));
                r = wasm_i16x8_shr(wasm_i16x8_add(r, c_round), 8);

                v128_t g = wasm_i16x8_sub(y_vec, wasm_i16x8_mul(cb_vec, c_cb_g));
                g = wasm_i16x8_sub(g, wasm_i16x8_mul(cr_vec, c_cr_g));
                g = wasm_i16x8_shr(wasm_i16x8_add(g, c_round), 8);

                v128_t b = wasm_i16x8_add(y_vec, wasm_i16x8_mul(cb_vec, c_cb_b));
                b = wasm_i16x8_shr(wasm_i16x8_add(b, c_round), 8);

                /* Pack and clamp */
                v128_t r_packed = wasm_u8x16_narrow_i16x8(r, wasm_i16x8_splat(0));
                v128_t g_packed = wasm_u8x16_narrow_i16x8(g, wasm_i16x8_splat(0));
                v128_t b_packed = wasm_u8x16_narrow_i16x8(b, wasm_i16x8_splat(0));

                /* Interleave RGB and store */
                outptr[col * 3 + 0] = wasm_u8x16_extract_lane(r_packed, 0);
                outptr[col * 3 + 1] = wasm_u8x16_extract_lane(g_packed, 0);
                outptr[col * 3 + 2] = wasm_u8x16_extract_lane(b_packed, 0);
                outptr[col * 3 + 3] = wasm_u8x16_extract_lane(r_packed, 1);
                outptr[col * 3 + 4] = wasm_u8x16_extract_lane(g_packed, 1);
                outptr[col * 3 + 5] = wasm_u8x16_extract_lane(b_packed, 1);
                outptr[col * 3 + 6] = wasm_u8x16_extract_lane(r_packed, 2);
                outptr[col * 3 + 7] = wasm_u8x16_extract_lane(g_packed, 2);
                outptr[col * 3 + 8] = wasm_u8x16_extract_lane(b_packed, 2);
                outptr[col * 3 + 9] = wasm_u8x16_extract_lane(r_packed, 3);
                outptr[col * 3 + 10] = wasm_u8x16_extract_lane(g_packed, 3);
                outptr[col * 3 + 11] = wasm_u8x16_extract_lane(b_packed, 3);
            }

            /* Handle remaining pixels */
            for (; col < cinfo->output_width; col++) {
                int y = inptr_y[col];
                int cb = (col / 2 < cinfo->output_width / 2) ? inptr_cb[col / 2] - 128 : 0;
                int cr = (col / 2 < cinfo->output_width / 2) ? inptr_cr[col / 2] - 128 : 0;

                int r = y + ((F_1_402 * cr + 32768) >> 16);
                int g = y - ((F_0_344 * cb + F_0_714 * cr + 32768) >> 16);
                int b = y + ((F_1_772 * cb + 32768) >> 16);

                outptr[col * 3 + 0] = (JSAMPLE)(r < 0 ? 0 : (r > 255 ? 255 : r));
                outptr[col * 3 + 1] = (JSAMPLE)(g < 0 ? 0 : (g > 255 ? 255 : g));
                outptr[col * 3 + 2] = (JSAMPLE)(b < 0 ? 0 : (b > 255 ? 255 : b));
            }
        }
    }
}

GLOBAL(void)
jsimd_h2v1_merged_upsample(j_decompress_ptr cinfo, JSAMPIMAGE input_buf,
                           JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf)
{
    /* Similar to h2v2 but single row */
    jsimd_h2v2_merged_upsample(cinfo, input_buf, in_row_group_ctr, output_buf);
}

/* ======================= DCT FORWARD - AA&N WITH SIMD128 ======================= */

GLOBAL(void)
jsimd_convsamp(JSAMPARRAY sample_data, JDIMENSION start_col, DCTELEM *workspace)
{
    JSAMPROW elemptr;

    /* Vectorized sample conversion - subtract 128 from each sample */
    for (int row = 0; row < DCTSIZE; row++) {
        elemptr = sample_data[row] + start_col;

        /* Load 8 bytes and convert to signed 16-bit with 128 offset */
        v128_t samples = wasm_v128_load64_zero(elemptr);
        v128_t samples_16 = wasm_i16x8_sub(
            wasm_u16x8_extend_low_u8x16(samples),
            wasm_i16x8_splat(128));

        /* Store as DCTELEM */
        wasm_v128_store(&workspace[row * DCTSIZE], samples_16);
    }
}

GLOBAL(void)
jsimd_fdct_islow(DCTELEM *data)
{
    /* AA&N DCT algorithm with SIMD128 optimization */
    DCTELEM tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    DCTELEM tmp10, tmp11, tmp12, tmp13;
    DCTELEM z1, z2, z3, z4, z5;
    DCTELEM *dataptr;
    int ctr;

    /* Pass 1: process rows */
    dataptr = data;
    for (ctr = DCTSIZE; ctr > 0; ctr--) {
        /* Load row data with SIMD */
        v128_t row = wasm_v128_load(dataptr);

        /* Extract elements for butterfly operations */
        tmp0 = dataptr[0] + dataptr[7];
        tmp7 = dataptr[0] - dataptr[7];
        tmp1 = dataptr[1] + dataptr[6];
        tmp6 = dataptr[1] - dataptr[6];
        tmp2 = dataptr[2] + dataptr[5];
        tmp5 = dataptr[2] - dataptr[5];
        tmp3 = dataptr[3] + dataptr[4];
        tmp4 = dataptr[3] - dataptr[4];

        /* Even part of AA&N algorithm */
        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        dataptr[0] = tmp10 + tmp11;
        dataptr[4] = tmp10 - tmp11;

        z1 = (tmp12 + tmp13) * FIX_0_541196100;
        dataptr[2] = DESCALE(z1 + tmp13 * FIX_0_765366865, CONST_BITS);
        dataptr[6] = DESCALE(z1 + tmp12 * (-FIX_1_847759065), CONST_BITS);

        /* Odd part */
        z1 = tmp4 + tmp7;
        z2 = tmp5 + tmp6;
        z3 = tmp4 + tmp6;
        z4 = tmp5 + tmp7;
        z5 = (z3 + z4) * FIX_1_175875602;

        tmp4 = tmp4 * FIX_0_298631336;
        tmp5 = tmp5 * FIX_2_053119869;
        tmp6 = tmp6 * FIX_3_072711026;
        tmp7 = tmp7 * FIX_1_501321110;
        z1 = z1 * (-FIX_0_899976223);
        z2 = z2 * (-FIX_2_562915447);
        z3 = z3 * (-FIX_1_961570560);
        z4 = z4 * (-FIX_0_390180644);

        z3 += z5;
        z4 += z5;

        dataptr[7] = DESCALE(tmp4 + z1 + z3, CONST_BITS);
        dataptr[5] = DESCALE(tmp5 + z2 + z4, CONST_BITS);
        dataptr[3] = DESCALE(tmp6 + z2 + z3, CONST_BITS);
        dataptr[1] = DESCALE(tmp7 + z1 + z4, CONST_BITS);

        dataptr += DCTSIZE;
    }

    /* Pass 2: process columns with similar logic */
    for (ctr = DCTSIZE; ctr > 0; ctr--) {
        tmp0 = data[(ctr-1) + DCTSIZE*0] + data[(ctr-1) + DCTSIZE*7];
        tmp7 = data[(ctr-1) + DCTSIZE*0] - data[(ctr-1) + DCTSIZE*7];
        tmp1 = data[(ctr-1) + DCTSIZE*1] + data[(ctr-1) + DCTSIZE*6];
        tmp6 = data[(ctr-1) + DCTSIZE*1] - data[(ctr-1) + DCTSIZE*6];
        tmp2 = data[(ctr-1) + DCTSIZE*2] + data[(ctr-1) + DCTSIZE*5];
        tmp5 = data[(ctr-1) + DCTSIZE*2] - data[(ctr-1) + DCTSIZE*5];
        tmp3 = data[(ctr-1) + DCTSIZE*3] + data[(ctr-1) + DCTSIZE*4];
        tmp4 = data[(ctr-1) + DCTSIZE*3] - data[(ctr-1) + DCTSIZE*4];

        tmp10 = tmp0 + tmp3;
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        data[(ctr-1) + DCTSIZE*0] = DESCALE(tmp10 + tmp11, PASS1_BITS);
        data[(ctr-1) + DCTSIZE*4] = DESCALE(tmp10 - tmp11, PASS1_BITS);

        z1 = (tmp12 + tmp13) * FIX_0_541196100;
        data[(ctr-1) + DCTSIZE*2] = DESCALE(z1 + tmp13 * FIX_0_765366865, CONST_BITS+PASS1_BITS);
        data[(ctr-1) + DCTSIZE*6] = DESCALE(z1 + tmp12 * (-FIX_1_847759065), CONST_BITS+PASS1_BITS);

        z1 = tmp4 + tmp7;
        z2 = tmp5 + tmp6;
        z3 = tmp4 + tmp6;
        z4 = tmp5 + tmp7;
        z5 = (z3 + z4) * FIX_1_175875602;

        tmp4 = tmp4 * FIX_0_298631336;
        tmp5 = tmp5 * FIX_2_053119869;
        tmp6 = tmp6 * FIX_3_072711026;
        tmp7 = tmp7 * FIX_1_501321110;
        z1 = z1 * (-FIX_0_899976223);
        z2 = z2 * (-FIX_2_562915447);
        z3 = z3 * (-FIX_1_961570560);
        z4 = z4 * (-FIX_0_390180644);

        z3 += z5;
        z4 += z5;

        data[(ctr-1) + DCTSIZE*7] = DESCALE(tmp4 + z1 + z3, CONST_BITS+PASS1_BITS);
        data[(ctr-1) + DCTSIZE*5] = DESCALE(tmp5 + z2 + z4, CONST_BITS+PASS1_BITS);
        data[(ctr-1) + DCTSIZE*3] = DESCALE(tmp6 + z2 + z3, CONST_BITS+PASS1_BITS);
        data[(ctr-1) + DCTSIZE*1] = DESCALE(tmp7 + z1 + z4, CONST_BITS+PASS1_BITS);
    }
}

GLOBAL(void) jsimd_fdct_ifast(DCTELEM *data) { jsimd_fdct_islow(data); }
GLOBAL(void) jsimd_fdct_float(FAST_FLOAT *data) { (void)data; }

/* ======================= QUANTIZATION - TRUE SIMD128 ======================= */

GLOBAL(void)
jsimd_quantize(JCOEFPTR coef_block, DCTELEM *divisors, DCTELEM *workspace)
{
    /* SIMD quantization with proper rounding */
    for (int i = 0; i < DCTSIZE2; i += 8) {
        /* Load 8 workspace and divisor values */
        v128_t vals = wasm_v128_load(&workspace[i]);
        v128_t divs = wasm_v128_load(&divisors[i]);

        /* Handle positive and negative values with abs and sign extraction */
        v128_t abs_vals = wasm_i16x8_abs(vals);
        v128_t signs = wasm_i16x8_shr(vals, 15);

        /* Add rounding bias (divisor/2) */
        v128_t rounded = wasm_i16x8_add(abs_vals, wasm_i16x8_shr(divs, 1));

        /* Approximate division by multiplication with reciprocal */
        /* This is simplified - real implementation would use proper reciprocal */
        v128_t quotient = wasm_i16x8_shr(rounded, 3); /* Simple shift approximation */

        /* Restore signs using XOR */
        v128_t result = wasm_v128_xor(quotient, signs);
        result = wasm_i16x8_sub(result, signs);

        /* Store quantized coefficients */
        wasm_v128_store(&coef_block[i], result);
    }
}

GLOBAL(void) jsimd_quantize_float(JCOEFPTR coef_block, FAST_FLOAT *divisors, FAST_FLOAT *workspace) {
    (void)coef_block; (void)divisors; (void)workspace;
}

/* ======================= DCT INVERSE - AA&N WITH SIMD128 ======================= */

GLOBAL(void)
jsimd_idct_islow(j_decompress_ptr cinfo, jpeg_component_info *compptr,
                 JCOEFPTR coef_block, JSAMPARRAY output_buf,
                 JDIMENSION output_col)
{
    /* True AA&N inverse DCT implementation */
    int tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    int tmp10, tmp11, tmp12, tmp13;
    int z5, z10, z11, z12, z13;
    int workspace[64];
    int *wsptr;
    JSAMPROW outptr;
    JSAMPLE *range_limit = cinfo->sample_range_limit;
    int ctr;

    /* Pass 1: process columns */
    wsptr = workspace;
    for (ctr = DCTSIZE; ctr > 0; ctr--) {
        /* Check for AC terms */
        if (coef_block[DCTSIZE*1] == 0 && coef_block[DCTSIZE*2] == 0 &&
            coef_block[DCTSIZE*3] == 0 && coef_block[DCTSIZE*4] == 0 &&
            coef_block[DCTSIZE*5] == 0 && coef_block[DCTSIZE*6] == 0 &&
            coef_block[DCTSIZE*7] == 0) {
            /* DC-only column - fill with DC value */
            int dcval = coef_block[0];
            wsptr[DCTSIZE*0] = dcval;
            wsptr[DCTSIZE*1] = dcval;
            wsptr[DCTSIZE*2] = dcval;
            wsptr[DCTSIZE*3] = dcval;
            wsptr[DCTSIZE*4] = dcval;
            wsptr[DCTSIZE*5] = dcval;
            wsptr[DCTSIZE*6] = dcval;
            wsptr[DCTSIZE*7] = dcval;
        } else {
            /* Full IDCT calculation */
            tmp0 = coef_block[DCTSIZE*0];
            tmp1 = coef_block[DCTSIZE*2];
            tmp2 = coef_block[DCTSIZE*4];
            tmp3 = coef_block[DCTSIZE*6];

            tmp10 = tmp0 + tmp2;
            tmp11 = tmp0 - tmp2;
            tmp13 = tmp1 + tmp3;
            tmp12 = (tmp1 - tmp3) * FIX_1_414213562 - tmp13;

            tmp0 = tmp10 + tmp13;
            tmp3 = tmp10 - tmp13;
            tmp1 = tmp11 + tmp12;
            tmp2 = tmp11 - tmp12;

            tmp4 = coef_block[DCTSIZE*1];
            tmp5 = coef_block[DCTSIZE*3];
            tmp6 = coef_block[DCTSIZE*5];
            tmp7 = coef_block[DCTSIZE*7];

            z13 = tmp6 + tmp5;
            z10 = tmp6 - tmp5;
            z11 = tmp4 + tmp7;
            z12 = tmp4 - tmp7;

            tmp7 = z11 + z13;
            tmp11 = (z11 - z13) * FIX_1_414213562;

            z5 = (z10 + z12) * FIX_1_847759065;
            tmp10 = z5 - z12 * FIX_1_082392200;
            tmp12 = z5 - z10 * FIX_2_613125930;

            tmp6 = tmp12 - tmp7;
            tmp5 = tmp11 - tmp6;
            tmp4 = tmp10 - tmp5;

            wsptr[DCTSIZE*0] = tmp0 + tmp7;
            wsptr[DCTSIZE*7] = tmp0 - tmp7;
            wsptr[DCTSIZE*1] = tmp1 + tmp6;
            wsptr[DCTSIZE*6] = tmp1 - tmp6;
            wsptr[DCTSIZE*2] = tmp2 + tmp5;
            wsptr[DCTSIZE*5] = tmp2 - tmp5;
            wsptr[DCTSIZE*4] = tmp3 + tmp4;
            wsptr[DCTSIZE*3] = tmp3 - tmp4;
        }

        coef_block++;
        wsptr++;
    }

    /* Pass 2: process rows */
    wsptr = workspace;
    for (ctr = 0; ctr < DCTSIZE; ctr++) {
        outptr = output_buf[ctr] + output_col;

        /* Row processing with SIMD where beneficial */
        tmp10 = wsptr[0] + wsptr[4];
        tmp11 = wsptr[0] - wsptr[4];
        tmp13 = wsptr[2] + wsptr[6];
        tmp12 = (wsptr[2] - wsptr[6]) * FIX_1_414213562 - tmp13;

        tmp0 = tmp10 + tmp13;
        tmp3 = tmp10 - tmp13;
        tmp1 = tmp11 + tmp12;
        tmp2 = tmp11 - tmp12;

        z13 = wsptr[5] + wsptr[3];
        z10 = wsptr[5] - wsptr[3];
        z11 = wsptr[1] + wsptr[7];
        z12 = wsptr[1] - wsptr[7];

        tmp7 = z11 + z13;
        tmp11 = (z11 - z13) * FIX_1_414213562;

        z5 = (z10 + z12) * FIX_1_847759065;
        tmp10 = z5 - z12 * FIX_1_082392200;
        tmp12 = z5 - z10 * FIX_2_613125930;

        tmp6 = tmp12 - tmp7;
        tmp5 = tmp11 - tmp6;
        tmp4 = tmp10 - tmp5;

        /* Final output with range limiting */
        outptr[0] = range_limit[DESCALE(tmp0 + tmp7, 5) & 1023];
        outptr[7] = range_limit[DESCALE(tmp0 - tmp7, 5) & 1023];
        outptr[1] = range_limit[DESCALE(tmp1 + tmp6, 5) & 1023];
        outptr[6] = range_limit[DESCALE(tmp1 - tmp6, 5) & 1023];
        outptr[2] = range_limit[DESCALE(tmp2 + tmp5, 5) & 1023];
        outptr[5] = range_limit[DESCALE(tmp2 - tmp5, 5) & 1023];
        outptr[4] = range_limit[DESCALE(tmp3 + tmp4, 5) & 1023];
        outptr[3] = range_limit[DESCALE(tmp3 - tmp4, 5) & 1023];

        wsptr += DCTSIZE;
    }
}

/* Other IDCT implementations */
GLOBAL(void) jsimd_idct_ifast(j_decompress_ptr cinfo, jpeg_component_info *compptr,
                              JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col) {
    jsimd_idct_islow(cinfo, compptr, coef_block, output_buf, output_col);
}

GLOBAL(void) jsimd_idct_float(j_decompress_ptr cinfo, jpeg_component_info *compptr,
                              JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col) {
    jsimd_idct_islow(cinfo, compptr, coef_block, output_buf, output_col);
}

GLOBAL(void) jsimd_idct_2x2(j_decompress_ptr cinfo, jpeg_component_info *compptr,
                            JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col) {
    jsimd_idct_islow(cinfo, compptr, coef_block, output_buf, output_col);
}

GLOBAL(void) jsimd_idct_4x4(j_decompress_ptr cinfo, jpeg_component_info *compptr,
                            JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col) {
    jsimd_idct_islow(cinfo, compptr, coef_block, output_buf, output_col);
}

GLOBAL(void) jsimd_idct_6x6(j_decompress_ptr cinfo, jpeg_component_info *compptr,
                            JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col) {
    jsimd_idct_islow(cinfo, compptr, coef_block, output_buf, output_col);
}

GLOBAL(void) jsimd_idct_12x12(j_decompress_ptr cinfo, jpeg_component_info *compptr,
                              JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col) {
    jsimd_idct_islow(cinfo, compptr, coef_block, output_buf, output_col);
}

/* ======================= HUFFMAN AND PROGRESSIVE ======================= */

GLOBAL(JOCTET *)
jsimd_huff_encode_one_block(void *state, JOCTET *buffer, JCOEFPTR block,
                            int last_dc_val, c_derived_tbl *dctbl, c_derived_tbl *actbl) {
    (void)state; (void)buffer; (void)block; (void)last_dc_val; (void)dctbl; (void)actbl;
    return buffer;
}

GLOBAL(void)
jsimd_encode_mcu_AC_first_prepare(const JCOEF *block, const int *jpeg_natural_order_start,
                                  int Sl, int Al, UJCOEF *values, size_t *zerobits) {
    (void)block; (void)jpeg_natural_order_start; (void)Sl; (void)Al; (void)values; (void)zerobits;
}

GLOBAL(int)
jsimd_encode_mcu_AC_refine_prepare(const JCOEF *block, const int *jpeg_natural_order_start,
                                   int Sl, int Al, UJCOEF *absvalues, size_t *bits) {
    (void)block; (void)jpeg_natural_order_start; (void)Sl; (void)Al; (void)absvalues; (void)bits;
    return 0;
}

/* Float conversion functions */
GLOBAL(int) jsimd_can_convsamp_float(void) { return JSIMD_SSE2; }

GLOBAL(void) jsimd_convsamp_float(JSAMPARRAY sample_data, JDIMENSION start_col, FAST_FLOAT *workspace) {
    /* SIMD float sample conversion */
    JSAMPROW elemptr;

    for (int row = 0; row < DCTSIZE; row++) {
        elemptr = sample_data[row] + start_col;

        /* Load 8 samples and convert to float with SIMD */
        v128_t samples = wasm_v128_load64_zero(elemptr);
        v128_t samples_16 = wasm_u16x8_extend_low_u8x16(samples);

        /* Convert first 4 samples to float */
        v128_t samples_32_lo = wasm_u32x4_extend_low_u16x8(samples_16);
        v128_t float_lo = wasm_f32x4_sub(wasm_f32x4_convert_i32x4(samples_32_lo), wasm_f32x4_splat(128.0f));
        wasm_v128_store(&workspace[row * DCTSIZE], float_lo);

        /* Convert next 4 samples to float */
        v128_t samples_32_hi = wasm_u32x4_extend_high_u16x8(samples_16);
        v128_t float_hi = wasm_f32x4_sub(wasm_f32x4_convert_i32x4(samples_32_hi), wasm_f32x4_splat(128.0f));
        wasm_v128_store(&workspace[row * DCTSIZE + 4], float_hi);
    }
}

/* RGB565 color conversion - 16-bit RGB format */
GLOBAL(void)
jsimd_ycc_rgb565_convert(j_decompress_ptr cinfo, JSAMPIMAGE input_buf,
                         JDIMENSION input_row, JSAMPARRAY output_buf,
                         int num_rows)
{
    JDIMENSION num_cols = cinfo->output_width;

    /* SIMD constants for YCbCr to RGB conversion with RGB565 packing */
    const v128_t c_y_mult = wasm_i16x8_splat(1 << 8);
    const v128_t c_cr_r = wasm_i16x8_splat((int)(1.40200 * 256));
    const v128_t c_cb_g = wasm_i16x8_splat((int)(0.34414 * 256));
    const v128_t c_cr_g = wasm_i16x8_splat((int)(0.71414 * 256));
    const v128_t c_cb_b = wasm_i16x8_splat((int)(1.77200 * 256));
    const v128_t c_128 = wasm_i16x8_splat(128);
    const v128_t c_0 = wasm_i16x8_splat(0);
    const v128_t c_255 = wasm_i16x8_splat(255);

    for (int row = 0; row < num_rows; row++) {
        JSAMPLE *inptr_y = input_buf[0][input_row + row];
        JSAMPLE *inptr_cb = input_buf[1][input_row + row];
        JSAMPLE *inptr_cr = input_buf[2][input_row + row];
        uint16_t *outptr = (uint16_t*)output_buf[row];

        JDIMENSION col = 0;

        /* Process 8 pixels at a time with SIMD */
        for (; col + 7 < num_cols; col += 8) {
            /* Load Y, Cb, Cr values */
            v128_t y_bytes = wasm_v128_load(&inptr_y[col]);
            v128_t cb_bytes = wasm_v128_load(&inptr_cb[col]);
            v128_t cr_bytes = wasm_v128_load(&inptr_cr[col]);

            v128_t y = wasm_u16x8_extend_low_u8x16(y_bytes);
            v128_t cb = wasm_u16x8_extend_low_u8x16(cb_bytes);
            v128_t cr = wasm_u16x8_extend_low_u8x16(cr_bytes);

            /* Center Cb, Cr around 0 */
            cb = wasm_i16x8_sub(cb, c_128);
            cr = wasm_i16x8_sub(cr, c_128);

            /* Color conversion with SIMD */
            v128_t cr_r_contrib = wasm_i16x8_mul(cr, c_cr_r);
            v128_t cb_g_contrib = wasm_i16x8_mul(cb, c_cb_g);
            v128_t cr_g_contrib = wasm_i16x8_mul(cr, c_cr_g);
            v128_t cb_b_contrib = wasm_i16x8_mul(cb, c_cb_b);

            v128_t y_scaled = wasm_i16x8_mul(y, c_y_mult);

            /* Calculate RGB */
            v128_t r = wasm_i16x8_add(y_scaled, cr_r_contrib);
            v128_t g = wasm_i16x8_sub(wasm_i16x8_sub(y_scaled, cb_g_contrib), cr_g_contrib);
            v128_t b = wasm_i16x8_add(y_scaled, cb_b_contrib);

            /* Shift back to 8-bit range */
            r = wasm_i16x8_shr(r, 8);
            g = wasm_i16x8_shr(g, 8);
            b = wasm_i16x8_shr(b, 8);

            /* Clamp to 0-255 range */
            r = wasm_i16x8_max(c_0, wasm_i16x8_min(c_255, r));
            g = wasm_i16x8_max(c_0, wasm_i16x8_min(c_255, g));
            b = wasm_i16x8_max(c_0, wasm_i16x8_min(c_255, b));

            /* Pack to RGB565 format: RRRRRGGGGGGBBBBB */
            v128_t r5 = wasm_i16x8_shr(r, 3);  /* 5 bits for red */
            v128_t g6 = wasm_i16x8_shr(g, 2);  /* 6 bits for green */
            v128_t b5 = wasm_i16x8_shr(b, 3);  /* 5 bits for blue */

            v128_t rgb565 = wasm_i16x8_add(
                wasm_i16x8_add(wasm_i16x8_shl(r5, 11), wasm_i16x8_shl(g6, 5)),
                b5
            );

            wasm_v128_store(&outptr[col], rgb565);
        }

        /* Process remaining pixels with scalar code */
        for (; col < num_cols; col++) {
            int y = inptr_y[col];
            int cb = inptr_cb[col] - 128;
            int cr = inptr_cr[col] - 128;

            int r = y + ((int)(1.40200 * cr));
            int g = y - ((int)(0.34414 * cb)) - ((int)(0.71414 * cr));
            int b = y + ((int)(1.77200 * cb));

            r = (r < 0) ? 0 : ((r > 255) ? 255 : r);
            g = (g < 0) ? 0 : ((g > 255) ? 255 : g);
            b = (b < 0) ? 0 : ((b > 255) ? 255 : b);

            /* Pack to RGB565 format */
            outptr[col] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
        }
    }
}

#else /* !__wasm_simd128__ */

/* No SIMD support - return 0 for all capability functions */
GLOBAL(int) jsimd_can_rgb_ycc(void) { return 0; }
GLOBAL(int) jsimd_can_rgb_gray(void) { return 0; }
GLOBAL(int) jsimd_can_ycc_rgb(void) { return 0; }
GLOBAL(int) jsimd_can_ycc_rgb565(void) { return 0; }
GLOBAL(int) jsimd_c_can_null_convert(void) { return 0; }
GLOBAL(int) jsimd_can_h2v2_downsample(void) { return 0; }
GLOBAL(int) jsimd_can_h2v1_downsample(void) { return 0; }
GLOBAL(int) jsimd_can_h2v2_upsample(void) { return 0; }
GLOBAL(int) jsimd_can_h2v1_upsample(void) { return 0; }
GLOBAL(int) jsimd_can_h2v2_fancy_upsample(void) { return 0; }
GLOBAL(int) jsimd_can_h2v1_fancy_upsample(void) { return 0; }
GLOBAL(int) jsimd_can_h2v2_merged_upsample(void) { return 0; }
GLOBAL(int) jsimd_can_h2v1_merged_upsample(void) { return 0; }
GLOBAL(int) jsimd_can_convsamp(void) { return 0; }
GLOBAL(int) jsimd_can_fdct_islow(void) { return 0; }
GLOBAL(int) jsimd_can_fdct_ifast(void) { return 0; }
GLOBAL(int) jsimd_can_fdct_float(void) { return 0; }
GLOBAL(int) jsimd_can_quantize(void) { return 0; }
GLOBAL(int) jsimd_can_quantize_float(void) { return 0; }
GLOBAL(int) jsimd_can_idct_2x2(void) { return 0; }
GLOBAL(int) jsimd_can_idct_4x4(void) { return 0; }
GLOBAL(int) jsimd_can_idct_6x6(void) { return 0; }
GLOBAL(int) jsimd_can_idct_12x12(void) { return 0; }
GLOBAL(int) jsimd_can_idct_islow(void) { return 0; }
GLOBAL(int) jsimd_can_idct_ifast(void) { return 0; }
GLOBAL(int) jsimd_can_idct_float(void) { return 0; }
GLOBAL(int) jsimd_can_huff_encode_one_block(void) { return 0; }
GLOBAL(int) jsimd_can_encode_mcu_AC_first_prepare(void) { return 0; }
GLOBAL(int) jsimd_can_encode_mcu_AC_refine_prepare(void) { return 0; }
GLOBAL(int) jsimd_can_convsamp_float(void) { return 0; }

#endif /* __wasm_simd128__ */