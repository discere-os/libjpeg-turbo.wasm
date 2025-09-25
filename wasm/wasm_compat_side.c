/*
 * wasm_compat_side.c - SIDE_MODULE compatibility layer for libjpeg-turbo
 * Copyright (c) 2025 Superstruct Ltd, New Zealand
 * Licensed under same license as libjpeg-turbo
 */

#include <emscripten.h>

/* SIDE_MODULE export declarations for libjpeg-turbo */
EMSCRIPTEN_KEEPALIVE int wasm_side_module_init(void) {
    return 1; /* Success */
}

EMSCRIPTEN_KEEPALIVE const char* wasm_side_module_version(void) {
    return "3.1.2-wasm-native";
}

/* Forward declarations for SIDE_MODULE exports */
extern int jpeg_wasm_init(void);
extern void jpeg_wasm_cleanup(void);

EMSCRIPTEN_KEEPALIVE int libjpeg_turbo_side_init(void) {
    return jpeg_wasm_init();
}

EMSCRIPTEN_KEEPALIVE void libjpeg_turbo_side_cleanup(void) {
    jpeg_wasm_cleanup();
}