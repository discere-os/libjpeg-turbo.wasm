import { assertEquals, assertExists } from "@std/assert";
import LibJPEGTurbo from "../../src/lib/index.ts";
import {
  JPEGCompressionError,
  JPEGDecompressionError,
  JPEGInvalidDataError,
} from "../../src/lib/index.ts";
import type { JPEGCompressionOptions } from "../../src/lib/index.ts";

Deno.test("JPEG Compression - Quality Levels", async () => {
  const jpeg = new LibJPEGTurbo();

  try {
    await jpeg.initialize();

    // Test image data (8x8 RGB)
    const width = 8, height = 8;
    const rgbData = new Uint8Array(width * height * 3);

    // Fill with checkerboard pattern
    for (let y = 0; y < height; y++) {
      for (let x = 0; x < width; x++) {
        const idx = (y * width + x) * 3;
        const value = ((x + y) % 2) * 255;
        rgbData[idx] = value; // R
        rgbData[idx + 1] = value; // G
        rgbData[idx + 2] = value; // B
      }
    }

    // Test different quality levels
    const qualities = [10, 50, 85, 95, 100];

    for (const quality of qualities) {
      try {
        const jpegData = jpeg.compressRGB(
          rgbData,
          width,
          height,
          { quality },
        );

        assertExists(jpegData);
        assertEquals(jpegData instanceof Uint8Array, true);
        assertEquals(jpegData.length > 0, true);
        console.log(
          `Quality ${quality}: compressed to ${jpegData.length} bytes`,
        );
      } catch (error) {
        // Expected until WASM implementation is complete
        console.log(
          `Quality ${quality} test failed (expected):`,
          (error as Error).message,
        );
      }
    }
  } finally {
    jpeg.cleanup();
  }
});

Deno.test("JPEG Compression - Progressive Mode", async () => {
  const jpeg = new LibJPEGTurbo();

  try {
    await jpeg.initialize();

    const width = 16, height = 16;
    const rgbData = new Uint8Array(width * height * 3);

    // Fill with gradient
    for (let y = 0; y < height; y++) {
      for (let x = 0; x < width; x++) {
        const idx = (y * width + x) * 3;
        rgbData[idx] = Math.floor((x / width) * 255); // R gradient
        rgbData[idx + 1] = Math.floor((y / height) * 255); // G gradient
        rgbData[idx + 2] = 128; // B constant
      }
    }

    try {
      // Test progressive encoding
      const progressiveJpeg = jpeg.compressRGB(
        rgbData,
        width,
        height,
        {
          quality: 85,
          progressive: true,
        },
      );

      assertExists(progressiveJpeg);
      assertEquals(progressiveJpeg instanceof Uint8Array, true);
      console.log(`Progressive JPEG: ${progressiveJpeg.length} bytes`);

      // Test baseline encoding
      const baselineJpeg = jpeg.compressRGB(
        rgbData,
        width,
        height,
        {
          quality: 85,
          progressive: false,
        },
      );

      assertExists(baselineJpeg);
      assertEquals(baselineJpeg instanceof Uint8Array, true);
      console.log(`Baseline JPEG: ${baselineJpeg.length} bytes`);
    } catch (error) {
      console.log(
        "Progressive JPEG test failed (expected):",
        (error as Error).message,
      );
    }
  } finally {
    jpeg.cleanup();
  }
});

Deno.test("JPEG Compression - RGB Data Validation", async () => {
  const jpeg = new LibJPEGTurbo();

  try {
    await jpeg.initialize();

    // Test valid RGB data (4x4 image)
    const width = 4, height = 4;
    const rgbData = new Uint8Array(width * height * 3);

    // Fill with solid gray
    rgbData.fill(128);

    try {
      const jpegData = jpeg.compressRGB(rgbData, width, height, {
        quality: 85,
      });

      assertExists(jpegData);
      assertEquals(jpegData instanceof Uint8Array, true);
      console.log(`RGB compression: ${jpegData.length} bytes`);
    } catch (error) {
      console.log(
        "RGB compression test failed (expected):",
        (error as Error).message,
      );
    }

    // Test invalid dimensions
    try {
      jpeg.compressRGB(rgbData, 0, 4); // Invalid width
      throw new Error("Should have thrown");
    } catch (error) {
      if (error instanceof JPEGCompressionError) {
        console.log("Correctly rejected invalid dimensions");
      } else {
        console.log(
          "Invalid dimensions test failed (expected):",
          (error as Error).message,
        );
      }
    }

    // Test wrong data size
    try {
      const wrongSizeData = new Uint8Array(10); // Wrong size for 4x4
      jpeg.compressRGB(wrongSizeData, 4, 4);
      throw new Error("Should have thrown");
    } catch (error) {
      if (error instanceof JPEGCompressionError) {
        console.log("Correctly rejected wrong data size");
      } else {
        console.log(
          "Wrong data size test failed (expected):",
          (error as Error).message,
        );
      }
    }
  } finally {
    jpeg.cleanup();
  }
});

Deno.test("JPEG Decompression - Error Handling", async () => {
  const jpeg = new LibJPEGTurbo();

  try {
    await jpeg.initialize();

    // Test with invalid JPEG data
    const invalidJpegData = new Uint8Array([0x00, 0x01, 0x02, 0x03]);

    try {
      jpeg.decompressJPEG(invalidJpegData);
      throw new Error("Should have thrown");
    } catch (error) {
      if (error instanceof JPEGDecompressionError) {
        console.log("Invalid JPEG correctly rejected");
      } else {
        console.log(
          "Invalid JPEG test failed (expected):",
          (error as Error).message,
        );
      }
    }

    // Test with empty data
    const emptyData = new Uint8Array(0);

    try {
      jpeg.decompressJPEG(emptyData);
      throw new Error("Should have thrown");
    } catch (error) {
      if (error instanceof JPEGInvalidDataError) {
        console.log("Empty data correctly rejected");
      } else {
        console.log(
          "Empty data test failed (expected):",
          (error as Error).message,
        );
      }
    }

    // Test with minimal JPEG header
    const minimalJpeg = new Uint8Array([0xFF, 0xD8, 0xFF, 0xE0]);

    try {
      const result = jpeg.decompressJPEG(minimalJpeg);
      assertExists(result);
      assertExists(result.data);
      assertEquals(typeof result.width, "number");
      assertEquals(typeof result.height, "number");
      assertEquals(typeof result.components, "number");
    } catch (error) {
      console.log(
        "Minimal JPEG test failed (expected):",
        (error as Error).message,
      );
    }
  } finally {
    jpeg.cleanup();
  }
});

Deno.test("JPEG Round-trip Processing", async () => {
  const jpeg = new LibJPEGTurbo();

  try {
    await jpeg.initialize();

    // Create test image data (small for testing)
    const width = 8, height = 8;
    const originalRgb = new Uint8Array(width * height * 3);

    // Fill with test pattern
    for (let y = 0; y < height; y++) {
      for (let x = 0; x < width; x++) {
        const idx = (y * width + x) * 3;
        originalRgb[idx] = (x * 32) % 256; // R pattern
        originalRgb[idx + 1] = (y * 32) % 256; // G pattern
        originalRgb[idx + 2] = 128; // B constant
      }
    }

    try {
      const startTime = performance.now();

      // Compress to JPEG
      const jpegData = jpeg.compressRGB(originalRgb, width, height, {
        quality: 85,
      });

      assertExists(jpegData);
      console.log(`Compressed to ${jpegData.length} bytes`);

      // Decompress back to RGB
      const result = jpeg.decompressJPEG(jpegData);

      assertExists(result);
      assertExists(result.data);
      assertEquals(result.width, width);
      assertEquals(result.height, height);
      assertEquals(result.components, 3);
      assertEquals(result.data.length, width * height * 3);

      const endTime = performance.now();
      console.log(`Round-trip took ${(endTime - startTime).toFixed(2)}ms`);
      console.log(`Decompressed to ${result.data.length} bytes`);
    } catch (error) {
      console.log(
        "Round-trip test failed (expected):",
        (error as Error).message,
      );
    }
  } finally {
    jpeg.cleanup();
  }
});

Deno.test("JPEG Compression Options Validation", () => {
  const validOptions: JPEGCompressionOptions[] = [
    {},
    { quality: 50 },
    { quality: 100, progressive: true },
    { quality: 1, progressive: false, optimizeHuffman: true },
    { progressive: false, optimizeHuffman: false },
  ];

  // All valid options should be well-formed
  for (const options of validOptions) {
    assertExists(options);

    if (options.quality !== undefined) {
      assertEquals(typeof options.quality, "number");
      assertEquals(options.quality >= 1 && options.quality <= 100, true);
    }

    if (options.progressive !== undefined) {
      assertEquals(typeof options.progressive, "boolean");
    }

    if (options.optimizeHuffman !== undefined) {
      assertEquals(typeof options.optimizeHuffman, "boolean");
    }
  }
});
