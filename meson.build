project('libjpeg-turbo.wasm', 'c', version: '3.0.5', meson_version: '>=1.3.0')

simd = get_option('simd')
cargs = []
if simd
  cargs += ['-msimd128']
endif

inc = include_directories('src', '.')

# Generate config headers
conf_data = configuration_data()
conf_data.set('JPEG_LIB_VERSION', 80)
conf_data.set('VERSION', '3.0.5')
conf_data.set('LIBJPEG_TURBO_VERSION_NUMBER', 3000500)
conf_data.set('C_ARITH_CODING_SUPPORTED', 1)
conf_data.set('D_ARITH_CODING_SUPPORTED', 1)
conf_data.set('WITH_SIMD', simd ? 1 : false)
conf_data.set('RIGHT_SHIFT_IS_UNSIGNED', false)

configure_file(input: 'src/jconfig.h.in', output: 'jconfig.h',
               configuration: conf_data, format: 'cmake')
configure_file(input: 'src/jconfigint.h.in', output: 'jconfigint.h',
               configuration: conf_data, format: 'cmake')

jpeg_sources = files(
  'src/jcapimin.c',
  'src/wrapper/jcapistd-8.c','src/wrapper/jcapistd-12.c','src/wrapper/jcapistd-16.c',
  'src/wrapper/jccoefct-8.c','src/wrapper/jccoefct-12.c',
  'src/wrapper/jccolor-8.c','src/wrapper/jccolor-12.c','src/wrapper/jccolor-16.c',
  'src/wrapper/jcdctmgr-8.c','src/wrapper/jcdctmgr-12.c',
  'src/wrapper/jcdiffct-8.c','src/wrapper/jcdiffct-12.c','src/wrapper/jcdiffct-16.c',
  'src/jchuff.c','src/jcicc.c','src/jcinit.c','src/jclhuff.c',
  'src/wrapper/jclossls-8.c','src/wrapper/jclossls-12.c','src/wrapper/jclossls-16.c',
  'src/wrapper/jcmainct-8.c','src/wrapper/jcmainct-12.c','src/wrapper/jcmainct-16.c',
  'src/jcmarker.c','src/jcmaster.c','src/jcomapi.c','src/jcparam.c','src/jcphuff.c',
  'src/wrapper/jcprepct-8.c','src/wrapper/jcprepct-12.c','src/wrapper/jcprepct-16.c',
  'src/wrapper/jcsample-8.c','src/wrapper/jcsample-12.c','src/wrapper/jcsample-16.c',
  'src/jctrans.c','src/jdapimin.c',
  'src/wrapper/jdapistd-8.c','src/wrapper/jdapistd-12.c','src/wrapper/jdapistd-16.c',
  'src/jdatadst.c','src/jdatasrc.c',
  'src/wrapper/jdcoefct-8.c','src/wrapper/jdcoefct-12.c',
  'src/wrapper/jdcolor-8.c','src/wrapper/jdcolor-12.c','src/wrapper/jdcolor-16.c',
  'src/wrapper/jddctmgr-8.c','src/wrapper/jddctmgr-12.c',
  'src/wrapper/jddiffct-8.c','src/wrapper/jddiffct-12.c','src/wrapper/jddiffct-16.c',
  'src/jdhuff.c','src/jdicc.c','src/jdinput.c','src/jdlhuff.c',
  'src/wrapper/jdlossls-8.c','src/wrapper/jdlossls-12.c','src/wrapper/jdlossls-16.c',
  'src/wrapper/jdmainct-8.c','src/wrapper/jdmainct-12.c','src/wrapper/jdmainct-16.c',
  'src/jdmarker.c','src/jdmaster.c',
  'src/wrapper/jdmerge-8.c','src/wrapper/jdmerge-12.c',
  'src/jdphuff.c',
  'src/wrapper/jdpostct-8.c','src/wrapper/jdpostct-12.c','src/wrapper/jdpostct-16.c',
  'src/wrapper/jdsample-8.c','src/wrapper/jdsample-12.c','src/wrapper/jdsample-16.c',
  'src/jdtrans.c','src/jerror.c','src/jfdctflt.c',
  'src/wrapper/jfdctfst-8.c','src/wrapper/jfdctfst-12.c',
  'src/wrapper/jfdctint-8.c','src/wrapper/jfdctint-12.c',
  'src/wrapper/jidctflt-8.c','src/wrapper/jidctflt-12.c',
  'src/wrapper/jidctfst-8.c','src/wrapper/jidctfst-12.c',
  'src/wrapper/jidctint-8.c','src/wrapper/jidctint-12.c',
  'src/wrapper/jidctred-8.c','src/wrapper/jidctred-12.c',
  'src/jmemmgr.c','src/jmemnobs.c','src/jpeg_nbits.c',
  'src/wrapper/jquant1-8.c','src/wrapper/jquant1-12.c',
  'src/wrapper/jquant2-8.c','src/wrapper/jquant2-12.c',
  'src/wrapper/jutils-8.c','src/wrapper/jutils-12.c','src/wrapper/jutils-16.c'
)

# MAIN compiles the full library + wrapper
executable('libjpeg-main',
  files('wasm/libjpeg_wasm_module.c') + jpeg_sources,
  include_directories: inc,
  c_args: cargs,
  link_args: [
    '-sWASM=1','-sMODULARIZE=1','-sEXPORT_ES6=1','-sEXPORT_NAME=LibjpegModule',
    '-sALLOW_MEMORY_GROWTH=1','-sNO_FILESYSTEM=1','-sEXPORTED_RUNTIME_METHODS=["cwrap","ccall","UTF8ToString"]','-O3','-flto'
  ],
  install: true,
  install_dir: get_option('bindir')
)

# SIDE compiles the full library as -sSIDE_MODULE=2 for dlopen
shared_module('libjpeg-side',
  files('wasm/libjpeg_wasm_side.c') + jpeg_sources,
  include_directories: inc,
  name_prefix: '',
  name_suffix: 'wasm',
  c_args: cargs,
  link_args: ['-sSIDE_MODULE=2','-fPIC','-O3','-flto','-sERROR_ON_UNDEFINED_SYMBOLS=0','-sEXPORTED_FUNCTIONS=["_libjpeg_wasm_version"]'],
  install: true,
  install_dir: get_option('libdir')
)

# Generate WASM manifest after build
manifest_script = find_program('../../../../scripts/generate-manifest-meson.sh', required: false)
if manifest_script.found()
  run_target('manifest',
    command: [manifest_script, meson.project_source_root(), meson.project_build_root()]
  )
endif
