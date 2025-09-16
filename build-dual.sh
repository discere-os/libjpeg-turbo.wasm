#!/bin/bash
# build-dual.sh - Dual build system for libjpeg-turbo.wasm (SIDE_MODULE + MAIN_MODULE)
#
# Copyright © 2025 Superstruct Ltd, New Zealand
# Licensed under libjpeg-turbo licenses (IJG + Modified BSD)
#
# Standard dual-build architecture for WASM libraries

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE="${BUILD_TYPE:-Release}"
INSTALL_PREFIX="${INSTALL_PREFIX:-./install}"
BUILD_DIR="${BUILD_DIR:-./build-dual}"
VARIANT="${1:-all}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Clean previous builds
clean_builds() {
    log_info "Cleaning previous builds..."
    rm -rf "${BUILD_DIR}-side" "${BUILD_DIR}-main-release" "${BUILD_DIR}-main-fallback"
    rm -rf "${INSTALL_PREFIX}/wasm"
    mkdir -p "${INSTALL_PREFIX}/wasm"
}

# Build SIDE_MODULE (production deployment)
build_side_module() {
    log_info "Building libjpeg-turbo.wasm as SIDE_MODULE for production dynamic loading..."

    local BUILD_SIDE="${BUILD_DIR}-side"
    mkdir -p "${BUILD_SIDE}"
    cd "${BUILD_SIDE}"

    # WASM-Native JPEG processing implementation with libjpeg-turbo
    local WASM_SOURCES=(
        "../wasm/jpeg_decode_wasm.c"
        "../wasm/jpeg_simd_wasm.c"
        "../wasm/jsimd_wasm_final.c"
        "../wasm/jturbojpeg_stubs.c"
        "../src/turbojpeg.c"
    )

    # Include libjpeg-turbo source files
    local LIBJPEG_SOURCES=(
        "../src/jcapimin.c"
        "../src/jcapistd.c"
        "../src/jccoefct.c"
        "../src/jccolor.c"
        "../src/jcdctmgr.c"
        "../src/jchuff.c"
        "../src/jcinit.c"
        "../src/jcmainct.c"
        "../src/jcmarker.c"
        "../src/jcmaster.c"
        "../src/jcomapi.c"
        "../src/jcparam.c"
        "../src/jcphuff.c"
        "../src/jcprepct.c"
        "../src/jcsample.c"
        "../src/jctrans.c"
        "../src/jdapimin.c"
        "../src/jdapistd.c"
        "../src/jdatadst.c"
        "../src/jdatasrc.c"
        "../src/jdcoefct.c"
        "../src/jdcolor.c"
        "../src/jddctmgr.c"
        "../src/jdhuff.c"
        "../src/jdinput.c"
        "../src/jdmainct.c"
        "../src/jdmarker.c"
        "../src/jdmaster.c"
        "../src/jdmerge.c"
        "../src/jdphuff.c"
        "../src/jdpostct.c"
        "../src/jdsample.c"
        "../src/jdtrans.c"
        "../src/jerror.c"
        "../src/jfdctflt.c"
        "../src/jfdctfst.c"
        "../src/jfdctint.c"
        "../src/jidctflt.c"
        "../src/jidctfst.c"
        "../src/jidctint.c"
        "../src/jquant1.c"
        "../src/jquant2.c"
        "../src/jutils.c"
        "../src/jmemmgr.c"
        "../src/jmemnobs.c"
        "../src/jdlhuff.c"
        "../src/jcarith.c"
        "../src/jclhuff.c"
        "../src/jdarith.c"
        "../src/jcdiffct.c"
        "../src/jddiffct.c"
        "../src/jidctred.c"
        "../src/jpeg_nbits.c"
        "../src/wrapper/jcdiffct-12.c"
        "../src/wrapper/jcdiffct-16.c"
        "../src/wrapper/jddiffct-12.c"
        "../src/wrapper/jddiffct-16.c"
        "../src/wrapper/jccoefct-12.c"
        "../src/wrapper/jdcoefct-12.c"
        "../src/wrapper/jcmainct-12.c"
        "../src/wrapper/jdmainct-12.c"
        "../src/wrapper/jcmainct-16.c"
        "../src/wrapper/jdmainct-16.c"
        "../src/wrapper/jquant1-12.c"
        "../src/wrapper/jquant2-12.c"
        "../src/wrapper/jidctint-12.c"
    )

    # Combine all sources
    local ALL_SOURCES=("${WASM_SOURCES[@]}" "${LIBJPEG_SOURCES[@]}")

    # Build SIDE_MODULE with basic optimization first
    emcc "${ALL_SOURCES[@]}" \
        -O3 -flto -DNDEBUG \
        -fPIC \
        -DWITH_SIMD=1 -DWITH_TURBOJPEG=1 -msimd128 \
        -DNO_GETENV -DHAVE_INTRIN_H=0 -DHAVE_IMMINTRIN_H=0 \
        -I../src -I../wasm \
        -s SIDE_MODULE=2 \
        -s STANDALONE_WASM=1 \
        -s EXPORTED_FUNCTIONS='["_jpeg_wasm_init","_jpeg_decode_from_memory","_jpeg_get_info","_jpeg_encode_to_memory","_jpeg_malloc","_jpeg_free","_jpeg_simd_get_metrics","_jpeg_cleanup"]' \
        -o libjpeg-turbo-side.wasm

    cd ..

    # Copy to install directory
    cp "${BUILD_SIDE}/libjpeg-turbo-side.wasm" "${INSTALL_PREFIX}/wasm/"
    log_success "SIDE_MODULE build completed: ${BUILD_SIDE}/libjpeg-turbo-side.wasm ($(stat -f%z "${BUILD_SIDE}/libjpeg-turbo-side.wasm" 2>/dev/null || stat -c%s "${BUILD_SIDE}/libjpeg-turbo-side.wasm") bytes)"
}

# Build MAIN_MODULE (testing and NPM distribution)
build_main_module() {
    local variant="$1"  # "release" or "fallback"
    log_info "Building libjpeg-turbo.wasm as MAIN_MODULE for ${variant}..."

    local BUILD_MAIN="${BUILD_DIR}-main-${variant}"
    mkdir -p "${BUILD_MAIN}"
    cd "${BUILD_MAIN}"

    # WASM-Native JPEG processing implementation with libjpeg-turbo
    local WASM_SOURCES=(
        "../wasm/jpeg_decode_wasm.c"
        "../wasm/jpeg_simd_wasm.c"
        "../wasm/jsimd_wasm_final.c"
        "../wasm/jturbojpeg_stubs.c"
        "../src/turbojpeg.c"
    )

    # Include libjpeg-turbo source files
    local LIBJPEG_SOURCES=(
        "../src/jcapimin.c"
        "../src/jcapistd.c"
        "../src/jccoefct.c"
        "../src/jccolor.c"
        "../src/jcdctmgr.c"
        "../src/jchuff.c"
        "../src/jcinit.c"
        "../src/jcmainct.c"
        "../src/jcmarker.c"
        "../src/jcmaster.c"
        "../src/jcomapi.c"
        "../src/jcparam.c"
        "../src/jcphuff.c"
        "../src/jcprepct.c"
        "../src/jcsample.c"
        "../src/jctrans.c"
        "../src/jdapimin.c"
        "../src/jdapistd.c"
        "../src/jdatadst.c"
        "../src/jdatasrc.c"
        "../src/jdcoefct.c"
        "../src/jdcolor.c"
        "../src/jddctmgr.c"
        "../src/jdhuff.c"
        "../src/jdinput.c"
        "../src/jdmainct.c"
        "../src/jdmarker.c"
        "../src/jdmaster.c"
        "../src/jdmerge.c"
        "../src/jdphuff.c"
        "../src/jdpostct.c"
        "../src/jdsample.c"
        "../src/jdtrans.c"
        "../src/jerror.c"
        "../src/jfdctflt.c"
        "../src/jfdctfst.c"
        "../src/jfdctint.c"
        "../src/jidctflt.c"
        "../src/jidctfst.c"
        "../src/jidctint.c"
        "../src/jquant1.c"
        "../src/jquant2.c"
        "../src/jutils.c"
        "../src/jmemmgr.c"
        "../src/jmemnobs.c"
        "../src/jdlhuff.c"
        "../src/jcarith.c"
        "../src/jclhuff.c"
        "../src/jdarith.c"
        "../src/jcdiffct.c"
        "../src/jddiffct.c"
        "../src/jidctred.c"
        "../src/jpeg_nbits.c"
        "../src/wrapper/jcdiffct-12.c"
        "../src/wrapper/jcdiffct-16.c"
        "../src/wrapper/jddiffct-12.c"
        "../src/wrapper/jddiffct-16.c"
        "../src/wrapper/jccoefct-12.c"
        "../src/wrapper/jdcoefct-12.c"
        "../src/wrapper/jcmainct-12.c"
        "../src/wrapper/jdmainct-12.c"
        "../src/wrapper/jcmainct-16.c"
        "../src/wrapper/jdmainct-16.c"
        "../src/wrapper/jquant1-12.c"
        "../src/wrapper/jquant2-12.c"
        "../src/wrapper/jidctint-12.c"
    )

    # Combine all sources
    local ALL_SOURCES=("${WASM_SOURCES[@]}" "${LIBJPEG_SOURCES[@]}")

    local EXTRA_FLAGS=""
    local OUTPUT_NAME="libjpeg-turbo-${variant}"

    if [[ "${variant}" == "release" ]]; then
        # Release build with WASM SIMD enabled
        EXTRA_FLAGS="-DWITH_SIMD=1 -msimd128"
    else
        # Fallback build with WASM SIMD enabled
        EXTRA_FLAGS="-DWITH_SIMD=1 -msimd128"
    fi

    # Common flags for both variants
    EXTRA_FLAGS="${EXTRA_FLAGS} -DNO_GETENV -DHAVE_INTRIN_H=0 -DHAVE_IMMINTRIN_H=0"

    # Build MAIN_MODULE
    emcc "${ALL_SOURCES[@]}" \
        -O3 -flto -DNDEBUG \
        ${EXTRA_FLAGS} -DWITH_TURBOJPEG=1 \
        -I../src -I../wasm \
        -s MODULARIZE=1 \
        -s EXPORT_ES6=1 \
        -s EXPORT_NAME="LibJPEGTurboModule" \
        -s EXPORTED_FUNCTIONS='["_jpeg_wasm_init","_jpeg_decode_from_memory","_jpeg_get_info","_jpeg_encode_to_memory","_jpeg_malloc","_jpeg_free","_jpeg_simd_get_metrics","_jpeg_cleanup"]' \
        -s EXPORTED_RUNTIME_METHODS='["cwrap","ccall","UTF8ToString","getValue","setValue","HEAPU8","HEAP8","HEAP16","HEAP32","HEAPU16","HEAPU32"]' \
        -s ALLOW_MEMORY_GROWTH=1 \
        -s INITIAL_MEMORY=16MB \
        -s MAXIMUM_MEMORY=2GB \
        -s SINGLE_FILE=1 \
        -o "${OUTPUT_NAME}.js"

    cd ..

    # Copy to install directory
    cp "${BUILD_MAIN}/${OUTPUT_NAME}.js" "${INSTALL_PREFIX}/wasm/"
    cp "${BUILD_MAIN}/${OUTPUT_NAME}.wasm" "${INSTALL_PREFIX}/wasm/" 2>/dev/null || true
    log_success "MAIN_MODULE build completed: ${BUILD_MAIN}/${OUTPUT_NAME}.js ($(stat -f%z "${BUILD_MAIN}/${OUTPUT_NAME}.js" 2>/dev/null || stat -c%s "${BUILD_MAIN}/${OUTPUT_NAME}.js") bytes)"
}

# Install artifacts
install_artifacts() {
    log_info "Installing build artifacts..."

    # Create install directory structure
    mkdir -p "${INSTALL_PREFIX}/wasm" "${INSTALL_PREFIX}/include"

    # Create compatibility symlinks for testing
    mkdir -p build/
    if [[ -f "${INSTALL_PREFIX}/wasm/libjpeg-turbo-release.js" ]]; then
        cp "${INSTALL_PREFIX}/wasm/libjpeg-turbo-release.js" "build/libjpeg-turbo-optimized.js"
        cp "${INSTALL_PREFIX}/wasm/libjpeg-turbo-release.wasm" "build/libjpeg-turbo-optimized.wasm" 2>/dev/null || true
    fi

    log_success "Installed SIDE_MODULE: ${INSTALL_PREFIX}/wasm/libjpeg-turbo-side.wasm"
    log_success "Installed MAIN_MODULE: ${INSTALL_PREFIX}/wasm/libjpeg-turbo-release.js"
    log_success "Installed FALLBACK_MODULE: ${INSTALL_PREFIX}/wasm/libjpeg-turbo-fallback.js"
    log_success "Copied optimized build to build/ for test compatibility"
    log_success "Installation complete in ${INSTALL_PREFIX}/"
}

# Main build logic
main() {
    case "${VARIANT}" in
        side)
            clean_builds
            build_side_module
            install_artifacts
            ;;
        main)
            clean_builds
            build_main_module "release"
            build_main_module "fallback"
            install_artifacts
            ;;
        all)
            clean_builds
            build_side_module
            build_main_module "release"
            build_main_module "fallback"
            install_artifacts
            ;;
        *)
            log_error "Unknown variant: ${VARIANT}"
            log_error "Usage: $0 [side|main|all]"
            exit 1
            ;;
    esac

    log_success "Build completed for variant: ${VARIANT}"
}

# Execute main function
main