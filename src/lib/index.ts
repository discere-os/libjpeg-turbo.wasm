/*
 * Copyright © 2025 Superstruct Ltd, New Zealand
 * Licensed under libjpeg-turbo licenses (IJG + Modified BSD)
 *
 * High-performance JPEG processing with WebAssembly and SIMD acceleration
 * TypeScript-first API following @discere-os patterns
 */

import {
  JPEGCompressionError,
  JPEGDecompressionError,
  JPEGError,
  JPEGInvalidDataError,
} from "./types.ts";
import type {
  JPEGCompressionOptions,
  JPEGInfo,
  JPEGResult,
  JPEGSIMDMetrics,
  LibJPEGTurboModule,
  LibJPEGTurboModuleFactory,
} from "./types.ts";

export {
  JPEGCompressionError,
  JPEGDecompressionError,
  JPEGError,
  JPEGInvalidDataError,
} from "./types.ts";
export type {
  JPEGCompressionOptions,
  JPEGInfo,
  JPEGResult,
  JPEGSIMDMetrics,
} from "./types.ts";

/**
 * High-performance JPEG processing library with WebAssembly and SIMD acceleration
 *
 * Features:
 * - WebAssembly SIMD optimization for 2-4x performance improvement
 * - Support for both compression and decompression
 * - Progressive JPEG support
 * - Metadata extraction without full decoding
 * - Memory-efficient processing
 * - TypeScript-first API with comprehensive error handling
 *
 * @example
 * ```typescript
 * const jpeg = new LibJPEGTurbo();
 * await jpeg.initialize();
 *
 * // Decompress JPEG to RGB
 * const jpegData = await Deno.readFile('image.jpg');
 * const result = await jpeg.decompressJPEG(jpegData);
 * console.log(`Decoded ${result.width}x${result.height} image`);
 *
 * // Compress RGB to JPEG
 * const jpegBytes = await jpeg.compressRGB(result.data, result.width, result.height);
 * await Deno.writeFile('output.jpg', jpegBytes);
 * ```
 */
export default class LibJPEGTurbo {
  private module: LibJPEGTurboModule | null = null;
  private initialized = false;

  /**
   * Check if the library is initialized and ready for use
   */
  get isInitialized(): boolean {
    return this.initialized;
  }

  /**
   * Get the underlying WASM module (for advanced usage)
   */
  get wasmModule(): LibJPEGTurboModule | null {
    return this.module;
  }

  /**
   * Initialize the JPEG processing library
   * Must be called before using any processing functions
   *
   * @throws {JPEGError} If initialization fails
   */
  async initialize(): Promise<void> {
    if (this.initialized) {
      return;
    }

    try {
      const moduleFactory = await this.loadModuleFactory();

      // For SINGLE_FILE builds, don't provide wasmBinary since it's embedded in the JS
      this.module = await moduleFactory({
        locateFile: (path: string): string => {
          if (path.endsWith(".wasm")) {
            return new URL("../../install/wasm/" + path, import.meta.url).href;
          }
          return path;
        },
      });

      // Initialize the WASM module
      this.module._jpeg_wasm_init();

      this.initialized = true;

    } catch (error) {
      throw new JPEGError(
        `Failed to initialize LibJPEG-Turbo: ${
          error instanceof Error ? error.message : String(error)
        }`,
      );
    }
  }

  /**
   * Compress RGB pixel data to JPEG format
   *
   * @param rgbData RGB pixel data (R, G, B, R, G, B, ...)
   * @param width Image width in pixels
   * @param height Image height in pixels
   * @param options Compression options
   * @returns JPEG compressed data
   * @throws {JPEGCompressionError} If compression fails
   */
  compressRGB(
    rgbData: Uint8Array,
    width: number,
    height: number,
    options: JPEGCompressionOptions = {},
  ): Uint8Array {
    this.ensureInitialized();

    const quality = Math.max(1, Math.min(100, options.quality ?? 85));

    if (!rgbData || rgbData.length !== width * height * 3) {
      throw new JPEGCompressionError("Invalid RGB data size");
    }

    if (width <= 0 || height <= 0) {
      throw new JPEGCompressionError("Invalid image dimensions");
    }

    try {
      // Allocate memory for input RGB data
      const rgbPtr = this.module!._jpeg_malloc(rgbData.length);
      if (!rgbPtr) {
        throw new JPEGCompressionError(
          "Failed to allocate memory for RGB data",
        );
      }

      // Copy RGB data to WASM memory
      this.module!.HEAPU8.set(rgbData, rgbPtr);

      // Allocate pointers for output
      const jpegPtrPtr = this.module!._jpeg_malloc(4);
      const jpegSizePtr = this.module!._jpeg_malloc(4);

      try {
        // Call compression function
        const result = this.module!._jpeg_encode_to_memory(
          rgbPtr,
          width,
          height,
          quality,
          jpegPtrPtr,
          jpegSizePtr,
        );

        if (result !== 0) {
          throw new JPEGCompressionError(
            `Compression failed with code ${result}`,
          );
        }

        // Get output data
        const jpegPtr = this.module!.getValue(jpegPtrPtr, "i32");
        const jpegSize = this.module!.getValue(jpegSizePtr, "i32");

        if (!jpegPtr || jpegSize <= 0) {
          throw new JPEGCompressionError("No output data generated");
        }

        // Copy result from WASM memory
        const jpegData = new Uint8Array(jpegSize);
        jpegData.set(this.module!.HEAPU8.subarray(jpegPtr, jpegPtr + jpegSize));

        // Free WASM output memory
        this.module!._jpeg_free(jpegPtr);

        return jpegData;
      } finally {
        // Free temporary pointers
        this.module!._jpeg_free(jpegPtrPtr);
        this.module!._jpeg_free(jpegSizePtr);
        this.module!._jpeg_free(rgbPtr);
      }
    } catch (error) {
      if (error instanceof JPEGCompressionError) {
        throw error;
      }
      throw new JPEGCompressionError(
        `Compression failed: ${
          error instanceof Error ? error.message : String(error)
        }`,
      );
    }
  }

  /**
   * Decompress JPEG data to RGB pixel data
   *
   * @param jpegData JPEG compressed data
   * @returns Decompressed RGB data and metadata
   * @throws {JPEGDecompressionError} If decompression fails
   */
  decompressJPEG(jpegData: Uint8Array): JPEGResult {
    this.ensureInitialized();

    if (!jpegData || jpegData.length === 0) {
      throw new JPEGInvalidDataError("Empty JPEG data");
    }

    try {
      // Allocate memory for JPEG data
      const jpegPtr = this.module!._jpeg_malloc(jpegData.length);
      if (!jpegPtr) {
        throw new JPEGDecompressionError(
          "Failed to allocate memory for JPEG data",
        );
      }

      // Copy JPEG data to WASM memory
      this.module!.HEAPU8.set(jpegData, jpegPtr);

      // Allocate pointers for output
      const rgbPtrPtr = this.module!._jpeg_malloc(4);
      const widthPtr = this.module!._jpeg_malloc(4);
      const heightPtr = this.module!._jpeg_malloc(4);
      const componentsPtr = this.module!._jpeg_malloc(4);

      try {
        // Call decompression function
        const result = this.module!._jpeg_decode_from_memory(
          jpegPtr,
          jpegData.length,
          rgbPtrPtr,
          widthPtr,
          heightPtr,
          componentsPtr,
        );

        if (result !== 0) {
          throw new JPEGDecompressionError(
            `Decompression failed with code ${result}`,
          );
        }

        // Get output metadata
        const rgbPtr = this.module!.getValue(rgbPtrPtr, "i32");
        const width = this.module!.getValue(widthPtr, "i32");
        const height = this.module!.getValue(heightPtr, "i32");
        const components = this.module!.getValue(componentsPtr, "i32");

        if (!rgbPtr || width <= 0 || height <= 0 || components <= 0) {
          throw new JPEGDecompressionError("Invalid output data");
        }

        // Calculate size and copy result
        const dataSize = width * height * components;
        const data = new Uint8Array(dataSize);
        data.set(this.module!.HEAPU8.subarray(rgbPtr, rgbPtr + dataSize));

        // Free WASM output memory
        this.module!._jpeg_free(rgbPtr);

        return {
          data,
          width,
          height,
          components,
          size: dataSize,
        };
      } finally {
        // Free temporary pointers
        this.module!._jpeg_free(rgbPtrPtr);
        this.module!._jpeg_free(widthPtr);
        this.module!._jpeg_free(heightPtr);
        this.module!._jpeg_free(componentsPtr);
        this.module!._jpeg_free(jpegPtr);
      }
    } catch (error) {
      if (
        error instanceof JPEGDecompressionError ||
        error instanceof JPEGInvalidDataError
      ) {
        throw error;
      }
      throw new JPEGDecompressionError(
        `Decompression failed: ${
          error instanceof Error ? error.message : String(error)
        }`,
      );
    }
  }

  /**
   * Get JPEG metadata without full decompression
   * Much faster than full decompression when you only need dimensions
   *
   * @param jpegData JPEG compressed data
   * @returns JPEG metadata
   * @throws {JPEGInvalidDataError} If JPEG data is invalid
   */
  getJPEGInfo(jpegData: Uint8Array): JPEGInfo {
    this.ensureInitialized();

    if (!jpegData || jpegData.length === 0) {
      throw new JPEGInvalidDataError("Empty JPEG data");
    }

    try {
      // Allocate memory for JPEG data
      const jpegPtr = this.module!._jpeg_malloc(jpegData.length);
      if (!jpegPtr) {
        throw new JPEGError("Failed to allocate memory");
      }

      // Copy JPEG data to WASM memory
      this.module!.HEAPU8.set(jpegData, jpegPtr);

      // Allocate pointers for output
      const widthPtr = this.module!._jpeg_malloc(4);
      const heightPtr = this.module!._jpeg_malloc(4);
      const componentsPtr = this.module!._jpeg_malloc(4);

      try {
        // Call info function
        const result = this.module!._jpeg_get_info(
          jpegPtr,
          jpegData.length,
          widthPtr,
          heightPtr,
          componentsPtr,
        );

        if (result !== 0) {
          throw new JPEGInvalidDataError("Invalid JPEG format");
        }

        // Get metadata
        const width = this.module!.getValue(widthPtr, "i32");
        const height = this.module!.getValue(heightPtr, "i32");
        const components = this.module!.getValue(componentsPtr, "i32");

        return {
          width,
          height,
          components,
          progressive: false, // Would be determined by actual JPEG parsing
          colorSpace: components === 3 ? "YCbCr" : "Grayscale",
        };
      } finally {
        this.module!._jpeg_free(widthPtr);
        this.module!._jpeg_free(heightPtr);
        this.module!._jpeg_free(componentsPtr);
        this.module!._jpeg_free(jpegPtr);
      }
    } catch (error) {
      if (error instanceof JPEGInvalidDataError) {
        throw error;
      }
      throw new JPEGInvalidDataError(
        `Failed to read JPEG info: ${
          error instanceof Error ? error.message : String(error)
        }`,
      );
    }
  }

  /**
   * Get SIMD performance metrics
   *
   * @returns SIMD capabilities and performance information
   */
  getSIMDMetrics(): JPEGSIMDMetrics {
    this.ensureInitialized();

    try {
      const metricsPtr = this.module!._jpeg_simd_get_metrics();
      const metricsJson = this.module!.UTF8ToString(metricsPtr);
      return JSON.parse(metricsJson);
    } catch (_error) {
      return {
        simdEnabled: false,
        instructionSet: "unknown",
      };
    }
  }

  /**
   * Clean up resources
   * Call this when you're done using the library
   */
  cleanup(): void {
    this.module = null;
    this.initialized = false;
  }

  private ensureInitialized(): void {
    if (!this.initialized || !this.module) {
      throw new JPEGError(
        "LibJPEG-Turbo not initialized. Call initialize() first.",
      );
    }
  }

  private async loadModuleFactory(): Promise<LibJPEGTurboModuleFactory> {
    const modulePath =
      new URL("../../install/wasm/libjpeg-turbo-release.js", import.meta.url)
        .href;
    try {
      const module = await import(modulePath);
      return module.default || module.LibJPEGTurboModule;
    } catch {
      // Fallback for testing
      const fallbackPath =
        new URL("../../build/libjpeg-turbo-optimized.js", import.meta.url).href;
      const module = await import(fallbackPath);
      return module.default || module.LibJPEGTurboModule;
    }
  }
}
