/*
 * Copyright © 2025 Superstruct Ltd, New Zealand
 * Licensed under libjpeg-turbo licenses (IJG + Modified BSD)
 *
 * TypeScript type definitions for libjpeg-turbo.wasm
 * High-performance JPEG processing with WebAssembly and SIMD acceleration
 */

/**
 * JPEG compression/decompression result
 */
export interface JPEGResult {
  /** RGB pixel data (R, G, B, R, G, B, ...) */
  data: Uint8Array;
  /** Image width in pixels */
  width: number;
  /** Image height in pixels */
  height: number;
  /** Number of color components (3 for RGB, 1 for grayscale) */
  components: number;
  /** Size of resulting data in bytes */
  size: number;
}

/**
 * JPEG compression options
 */
export interface JPEGCompressionOptions {
  /** Quality level (1-100, higher = better quality, larger file) */
  quality?: number;
  /** Whether to create progressive JPEG */
  progressive?: boolean;
  /** Whether to optimize Huffman tables */
  optimizeHuffman?: boolean;
}

/**
 * JPEG information (metadata) without full decoding
 */
export interface JPEGInfo {
  /** Image width in pixels */
  width: number;
  /** Image height in pixels */
  height: number;
  /** Number of color components */
  components: number;
  /** Whether image uses progressive encoding */
  progressive: boolean;
  /** Color space (e.g., "YCbCr", "RGB", "Grayscale") */
  colorSpace: string;
}

/**
 * Performance metrics from SIMD operations
 */
export interface JPEGSIMDMetrics {
  /** Whether SIMD acceleration is enabled */
  simdEnabled: boolean;
  /** Instruction set being used */
  instructionSet: string;
  /** DCT/IDCT acceleration factor */
  dctAcceleration?: string;
  /** Color conversion acceleration factor */
  colorConversionAcceleration?: string;
  /** Overall throughput improvement */
  throughputImprovement?: string;
}

/**
 * Error types for JPEG operations
 */
export class JPEGError extends Error {
  public override readonly name: string = "JPEGError";

  constructor(message: string, public readonly code?: number) {
    super(message);
  }
}

export class JPEGDecompressionError extends JPEGError {
  public override readonly name: string = "JPEGDecompressionError";

  constructor(message: string, code?: number) {
    super(`JPEG decompression failed: ${message}`, code);
  }
}

export class JPEGCompressionError extends JPEGError {
  public override readonly name: string = "JPEGCompressionError";

  constructor(message: string, code?: number) {
    super(`JPEG compression failed: ${message}`, code);
  }
}

export class JPEGInvalidDataError extends JPEGError {
  public override readonly name: string = "JPEGInvalidDataError";

  constructor(message: string) {
    super(`Invalid JPEG data: ${message}`);
  }
}

/**
 * Module factory function type for WASM module
 */
export interface LibJPEGTurboModuleFactory {
  (options?: {
    wasmBinary?: ArrayBuffer;
    locateFile?: (path: string) => string;
  }): Promise<LibJPEGTurboModule>;
}

/**
 * Raw WASM module interface
 */
export interface LibJPEGTurboModule {
  // C function wrappers
  _jpeg_wasm_init: () => void;
  _jpeg_decode_from_memory: (
    jpegPtr: number,
    jpegSize: number,
    rgbPtrPtr: number,
    widthPtr: number,
    heightPtr: number,
    componentsPtr: number,
  ) => number;
  _jpeg_get_info: (
    jpegPtr: number,
    jpegSize: number,
    widthPtr: number,
    heightPtr: number,
    componentsPtr: number,
  ) => number;
  _jpeg_encode_to_memory: (
    rgbPtr: number,
    width: number,
    height: number,
    quality: number,
    jpegPtrPtr: number,
    jpegSizePtr: number,
  ) => number;
  _jpeg_malloc: (size: number) => number;
  _jpeg_free: (ptr: number) => void;
  _jpeg_simd_get_metrics: () => number;

  // Emscripten runtime methods
  cwrap: (
    name: string,
    returnType: string,
    argTypes: string[],
  ) => (...args: unknown[]) => unknown;
  ccall: (
    name: string,
    returnType: string,
    argTypes: string[],
    args: unknown[],
  ) => unknown;
  UTF8ToString: (ptr: number) => string;
  getValue: (ptr: number, type: string) => number;
  setValue: (ptr: number, value: number, type: string) => void;

  // Memory management
  HEAP8: Int8Array;
  HEAP16: Int16Array;
  HEAP32: Int32Array;
  HEAPU8: Uint8Array;
  HEAPU16: Uint16Array;
  HEAPU32: Uint32Array;
}
