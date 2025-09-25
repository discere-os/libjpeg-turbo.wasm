#include <emscripten.h>
#include "wasm/jversion.h"

EMSCRIPTEN_KEEPALIVE
const char* libjpeg_wasm_version(void) {
  return JVERSION;
}

