/* jconfigint.h for WebAssembly build of libjpeg-turbo */

#define BUILD "20250101"
#define PACKAGE_NAME "libjpeg-turbo"
#define VERSION "3.0.5"

/* Symbol visibility */
#define HIDDEN __attribute__((visibility("hidden")))

/* Disable platform-specific intrinsics for WASM */
#undef HAVE_INTRIN_H
#undef HAVE_IMMINTRIN_H
#define HAVE_BUILTIN_CTZL
#define SIZEOF_SIZE_T 4

/* Inline assembly not available in WASM */
#define INLINE_ASM 0

/* Memory alignment */
#define ALIGN_SIZE 16

/* Architecture-specific settings */
#define SIZEOF_INT 4
#define SIZEOF_LONG 4
#if defined(__wasm64__)
#define SIZEOF_LONG_LONG 8
#define SIZEOF_UNSIGNED_LONG 8
#define SIZEOF_SIZE_T 8
#else
#define SIZEOF_LONG_LONG 8
#define SIZEOF_UNSIGNED_LONG 4
#define SIZEOF_SIZE_T 4
#endif

/* Threading not supported in basic WASM build */
#define THREAD_LOCAL

/* SIMD support */
#ifdef WITH_SIMD
  #if defined(__wasm_simd128__)
    #define HAVE_WASM_SIMD 1
  #endif
#endif