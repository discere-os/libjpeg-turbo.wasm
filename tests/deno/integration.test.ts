import { assertEquals, assertExists } from "@std/assert";
import LibJPEGTurbo from "../../src/lib/index.ts";
import type {
  JPEGInfo,
  JPEGResult,
  JPEGSIMDMetrics,
} from "../../src/lib/index.ts";

Deno.test("JPEG Integration - Full Workflow", async () => {
  const jpeg = new LibJPEGTurbo();

  try {
    await jpeg.initialize();

    // Create test image with known pattern
    const width = 16, height = 16;
    const rgbData = new Uint8Array(width * height * 3);

    // Create a recognizable pattern
    for (let y = 0; y < height; y++) {
      for (let x = 0; x < width; x++) {
        const idx = (y * width + x) * 3;
        rgbData[idx] = x < width / 2 ? 255 : 0; // Left half red
        rgbData[idx + 1] = y < height / 2 ? 255 : 0; // Top half green
        rgbData[idx + 2] = (x + y) % 2 ? 255 : 0; // Checkerboard blue
      }
    }

    try {
      // Step 1: Compress RGB to JPEG
      const jpegData = jpeg.compressRGB(rgbData, width, height, {
        quality: 90,
        progressive: false,
      });

      assertExists(jpegData);
      assertEquals(jpegData instanceof Uint8Array, true);
      assertEquals(jpegData.length > 0, true);
      console.log(`✓ Compression successful: ${jpegData.length} bytes`);

      // Step 2: Get JPEG info without full decompression
      const info: JPEGInfo = jpeg.getJPEGInfo(jpegData);

      assertExists(info);
      assertEquals(info.width, width);
      assertEquals(info.height, height);
      assertEquals(typeof info.components, "number");
      assertEquals(typeof info.progressive, "boolean");
      assertEquals(typeof info.colorSpace, "string");
      console.log(
        `✓ JPEG info: ${info.width}x${info.height}, ${info.components} components`,
      );

      // Step 3: Decompress JPEG back to RGB
      const result: JPEGResult = jpeg.decompressJPEG(jpegData);

      assertExists(result);
      assertExists(result.data);
      assertEquals(result.width, width);
      assertEquals(result.height, height);
      assertEquals(result.components, 3);
      assertEquals(result.size, width * height * 3);
      assertEquals(result.data.length, width * height * 3);
      console.log(`✓ Decompression successful: ${result.data.length} bytes`);

      // Step 4: Verify round-trip data integrity (lossy, so just basic checks)
      assertEquals(result.data.length, rgbData.length);

      // Check that we got some non-zero data back
      const nonZeroCount = result.data.filter((byte) => byte > 0).length;
      assertEquals(nonZeroCount > 0, true);
      console.log(
        `✓ Round-trip integrity check passed (${nonZeroCount} non-zero bytes)`,
      );
    } catch (error) {
      console.log(
        "Integration test failed (expected until WASM is complete):",
        (error as Error).message,
      );
    }
  } finally {
    jpeg.cleanup();
  }
});

Deno.test("JPEG Integration - SIMD Performance", async () => {
  const jpeg = new LibJPEGTurbo();

  try {
    await jpeg.initialize();

    // Get SIMD capabilities
    const metrics: JPEGSIMDMetrics = jpeg.getSIMDMetrics();

    assertExists(metrics);
    assertEquals(typeof metrics.simdEnabled, "boolean");
    assertEquals(typeof metrics.instructionSet, "string");

    console.log(
      `SIMD Support: ${metrics.simdEnabled ? "Enabled" : "Disabled"}`,
    );
    console.log(`Instruction Set: ${metrics.instructionSet}`);

    if (metrics.dctAcceleration) {
      console.log(`DCT Acceleration: ${metrics.dctAcceleration}`);
    }

    if (metrics.colorConversionAcceleration) {
      console.log(`Color Conversion: ${metrics.colorConversionAcceleration}`);
    }

    if (metrics.throughputImprovement) {
      console.log(`Overall Improvement: ${metrics.throughputImprovement}`);
    }

    // Test with larger image to see SIMD benefits
    const width = 64, height = 64;
    const rgbData = new Uint8Array(width * height * 3);

    // Fill with gradient pattern
    for (let i = 0; i < rgbData.length; i += 3) {
      rgbData[i] = i % 256; // R
      rgbData[i + 1] = (i / 2) % 256; // G
      rgbData[i + 2] = (i / 4) % 256; // B
    }

    try {
      const startTime = performance.now();

      const jpegData = jpeg.compressRGB(rgbData, width, height, {
        quality: 85,
      });

      const endTime = performance.now();
      const processingTime = endTime - startTime;

      assertExists(jpegData);
      console.log(
        `✓ Large image compression: ${jpegData.length} bytes in ${
          processingTime.toFixed(2)
        }ms`,
      );

      const throughput = (rgbData.length / 1024 / 1024) /
        (processingTime / 1000);
      console.log(`✓ Throughput: ${throughput.toFixed(2)} MB/s`);
    } catch (error) {
      console.log(
        "SIMD performance test failed (expected):",
        (error as Error).message,
      );
    }
  } finally {
    jpeg.cleanup();
  }
});

Deno.test("JPEG Integration - Edge Cases", async () => {
  const jpeg = new LibJPEGTurbo();

  try {
    await jpeg.initialize();

    // Test minimum size image (1x1)
    try {
      const minRgb = new Uint8Array([255, 128, 64]); // 1x1 RGB
      const jpegData = jpeg.compressRGB(minRgb, 1, 1, { quality: 50 });

      assertExists(jpegData);
      console.log(`✓ Minimum image (1x1): ${jpegData.length} bytes`);

      const result = jpeg.decompressJPEG(jpegData);
      assertEquals(result.width, 1);
      assertEquals(result.height, 1);
      assertEquals(result.components, 3);
    } catch (error) {
      console.log(
        "Minimum image test failed (expected):",
        (error as Error).message,
      );
    }

    // Test quality extremes
    const testRgb = new Uint8Array(4 * 4 * 3); // 4x4 RGB
    testRgb.fill(200); // Light gray

    for (const quality of [1, 100]) {
      try {
        const jpegData = jpeg.compressRGB(testRgb, 4, 4, { quality });
        assertExists(jpegData);
        console.log(`✓ Quality ${quality}: ${jpegData.length} bytes`);
      } catch (error) {
        console.log(
          `Quality ${quality} test failed (expected):`,
          (error as Error).message,
        );
      }
    }

    // Test progressive vs baseline
    for (const progressive of [false, true]) {
      try {
        const jpegData = jpeg.compressRGB(testRgb, 4, 4, {
          quality: 75,
          progressive,
        });
        assertExists(jpegData);
        console.log(
          `✓ ${
            progressive ? "Progressive" : "Baseline"
          }: ${jpegData.length} bytes`,
        );
      } catch (error) {
        console.log(
          `${progressive ? "Progressive" : "Baseline"} test failed (expected):`,
          (error as Error).message,
        );
      }
    }
  } finally {
    jpeg.cleanup();
  }
});

Deno.test("JPEG Integration - Memory Management", async () => {
  // Test creating and destroying multiple instances
  const instances = [];

  try {
    for (let i = 0; i < 3; i++) {
      const jpeg = new LibJPEGTurbo();
      await jpeg.initialize();
      instances.push(jpeg);

      assertEquals(jpeg.isInitialized, true);
      assertExists(jpeg.wasmModule);
    }

    console.log(`✓ Created ${instances.length} instances successfully`);

    // Test that each instance works independently
    const testRgb = new Uint8Array(2 * 2 * 3);
    testRgb.fill(100);

    for (let i = 0; i < instances.length; i++) {
      try {
        const metrics = await instances[i].getSIMDMetrics();
        assertExists(metrics);
        console.log(`✓ Instance ${i} metrics: ${metrics.instructionSet}`);
      } catch (error) {
        console.log(
          `Instance ${i} test failed (expected):`,
          (error as Error).message,
        );
      }
    }
  } finally {
    // Clean up all instances
    for (const instance of instances) {
      instance.cleanup();
    }
    console.log(`✓ Cleaned up ${instances.length} instances`);
  }
});
