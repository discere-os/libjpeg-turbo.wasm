/* jconfig.h for WebAssembly build of libjpeg-turbo */

#define JPEG_LIB_VERSION  62
#define LIBJPEG_TURBO_VERSION  3.0.5
#define LIBJPEG_TURBO_VERSION_NUMBER  3000005

/* Support arithmetic encoding/decoding */
#define C_ARITH_CODING_SUPPORTED 1
#define D_ARITH_CODING_SUPPORTED 1

/* Support in-memory source/destination managers */
#define MEM_SRCDST_SUPPORTED  1

/* Use accelerated SIMD routines - controlled by build flags */
/* #undef WITH_SIMD */

/* Data precision */
#ifndef BITS_IN_JSAMPLE
#define BITS_IN_JSAMPLE  8
#endif

/* For WASM/Emscripten */
#undef RIGHT_SHIFT_IS_UNSIGNED

/* Define basic types for WASM */
typedef unsigned char boolean;
#define HAVE_BOOLEAN

typedef short INT16;
typedef signed int INT32;
#define XMD_H

/* Memory allocation */
#define HAVE_STDLIB_H
#define HAVE_STDDEF_H

/* WASM-specific optimizations */
#define INLINE inline
/* LOCAL macro will be defined by jmorecfg.h */

/* Fallthrough annotation for switch statements */
#define FALLTHROUGH /* fallthrough */

/* TurboJPEG support */
#ifdef WITH_TURBOJPEG
#define WITH_TURBOJPEG 1
#endif