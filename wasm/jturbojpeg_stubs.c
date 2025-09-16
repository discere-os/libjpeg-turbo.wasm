/*
 * TurboJPEG and extended precision stubs for WebAssembly build
 * These functions are referenced but not implemented in the standard build
 */

#include "../src/jinclude.h"
#include "../src/jpeglib.h"

/* TurboJPEG memory function stubs */
void *
jpeg_mem_src_tj(j_decompress_ptr cinfo, const unsigned char *inbuffer,
                 unsigned long insize)
{
  jpeg_mem_src(cinfo, (const unsigned char *)inbuffer, insize);
  return NULL;
}

void *
jpeg_mem_dest_tj(j_compress_ptr cinfo, unsigned char **outbuffer,
                  unsigned long *outsize)
{
  jpeg_mem_dest(cinfo, outbuffer, outsize);
  return NULL;
}

/* ICC profile stubs */
GLOBAL(boolean)
jpeg_read_icc_profile(j_decompress_ptr cinfo, JOCTET **icc_data_ptr,
                      unsigned int *icc_data_len)
{
  *icc_data_ptr = NULL;
  *icc_data_len = 0;
  return FALSE;
}

GLOBAL(void)
jpeg_write_icc_profile(j_compress_ptr cinfo, const JOCTET *icc_data_ptr,
                       unsigned int icc_data_len)
{
  (void)cinfo; (void)icc_data_ptr; (void)icc_data_len;
}

/* Extended precision color converter stubs */
GLOBAL(void) j12init_color_converter(j_compress_ptr cinfo) { (void)cinfo; }
GLOBAL(void) j16init_color_converter(j_compress_ptr cinfo) { (void)cinfo; }

/* Extended precision downsampler stubs */
GLOBAL(void) j12init_downsampler(j_compress_ptr cinfo) { (void)cinfo; }
GLOBAL(void) j16init_downsampler(j_compress_ptr cinfo) { (void)cinfo; }

/* Extended precision prep controller stubs */
GLOBAL(void) j12init_c_prep_controller(j_compress_ptr cinfo, boolean need_full_buffer) {
    (void)cinfo; (void)need_full_buffer;
}
GLOBAL(void) j16init_c_prep_controller(j_compress_ptr cinfo, boolean need_full_buffer) {
    (void)cinfo; (void)need_full_buffer;
}

/* Lossless compression stubs */
GLOBAL(void) jinit_lossless_compressor(j_compress_ptr cinfo) { (void)cinfo; }
GLOBAL(void) j12init_lossless_compressor(j_compress_ptr cinfo) { (void)cinfo; }
GLOBAL(void) j16init_lossless_compressor(j_compress_ptr cinfo) { (void)cinfo; }

/* Float conversion functions available in jsimd_wasm_final.c */

/* Extended precision DCT controller stubs */
GLOBAL(void) j12init_forward_dct(j_compress_ptr cinfo) { (void)cinfo; }

/* Extended precision merged upsampler stubs */
GLOBAL(void) j12init_merged_upsampler(j_decompress_ptr cinfo) { (void)cinfo; }

/* Extended precision color deconverter stubs */
GLOBAL(void) j12init_color_deconverter(j_decompress_ptr cinfo) { (void)cinfo; }
GLOBAL(void) j16init_color_deconverter(j_decompress_ptr cinfo) { (void)cinfo; }

/* Extended precision upsampler stubs */
GLOBAL(void) j12init_upsampler(j_decompress_ptr cinfo) { (void)cinfo; }
GLOBAL(void) j16init_upsampler(j_decompress_ptr cinfo) { (void)cinfo; }

/* Extended precision post controller stubs */
GLOBAL(void) j12init_d_post_controller(j_decompress_ptr cinfo, boolean need_full_buffer) {
    (void)cinfo; (void)need_full_buffer;
}
GLOBAL(void) j16init_d_post_controller(j_decompress_ptr cinfo, boolean need_full_buffer) {
    (void)cinfo; (void)need_full_buffer;
}

/* Lossless decompressor stubs */
GLOBAL(void) jinit_lossless_decompressor(j_decompress_ptr cinfo) { (void)cinfo; }
GLOBAL(void) j12init_lossless_decompressor(j_decompress_ptr cinfo) { (void)cinfo; }
GLOBAL(void) j16init_lossless_decompressor(j_decompress_ptr cinfo) { (void)cinfo; }

/* Extended precision inverse DCT stubs */
GLOBAL(void) j12init_inverse_dct(j_decompress_ptr cinfo) { (void)cinfo; }

/* Arithmetic coding table stub */
const int jpeg_aritab[1] = {0};