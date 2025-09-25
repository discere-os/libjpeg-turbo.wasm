project('libjpeg-turbo.wasm', 'c', version: '3.0.5', meson_version: '>=1.3.0')

simd = get_option('simd')
cargs = []
if simd
  cargs += ['-msimd128']
endif

executable('libjpeg-main',
  files('wasm/libjpeg_wasm_module.c'),
  c_args: cargs,
  link_args: [
    '-sUSE_LIBJPEG=1','-sWASM=1','-sMODULARIZE=1','-sEXPORT_ES6=1','-sEXPORT_NAME=LibjpegModule',
    '-sALLOW_MEMORY_GROWTH=1','-sNO_FILESYSTEM=1','-sEXPORTED_RUNTIME_METHODS=["cwrap","ccall","UTF8ToString"]','-O3','-flto'
  ],
  install: true,
  install_dir: get_option('bindir')
)

shared_module('libjpeg-side',
  files('wasm/libjpeg_wasm_side.c'),
  name_prefix: '',
  c_args: cargs,
  link_args: ['-sSIDE_MODULE=2','-fPIC','-O3','-flto','-sEXPORTED_FUNCTIONS=["_libjpeg_wasm_version"]'],
  install: true,
  install_dir: get_option('libdir')
)

