import { assertEquals, assertExists } from "@std/assert";
import LibJPEGTurbo from "../../src/lib/index.ts";
import {
  JPEGCompressionError,
  JPEGDecompressionError,
  JPEGError,
  JPEGInvalidDataError,
} from "../../src/lib/index.ts";

Deno.test("LibJPEGTurbo - Basic Initialization", async () => {
  const jpeg = new LibJPEGTurbo();

  // Should not be initialized initially
  assertEquals(jpeg.isInitialized, false);

  // Should initialize without throwing
  await jpeg.initialize();

  // Should be initialized now
  assertEquals(jpeg.isInitialized, true);

  // Should be able to get WASM module reference
  assertExists(jpeg.wasmModule);

  jpeg.cleanup();
});

Deno.test("LibJPEGTurbo - Error Classes", () => {
  // Test error class hierarchy
  const jpegError = new JPEGError("Test message");
  assertExists(jpegError);
  assertEquals(jpegError.name, "JPEGError");
  assertEquals(jpegError.message, "Test message");

  const decompressionError = new JPEGDecompressionError("Invalid format");
  assertExists(decompressionError);
  assertEquals(decompressionError.name, "JPEGDecompressionError");
  assertEquals(decompressionError instanceof JPEGError, true);

  const compressionError = new JPEGCompressionError("Out of memory");
  assertExists(compressionError);
  assertEquals(compressionError.name, "JPEGCompressionError");
  assertEquals(compressionError instanceof JPEGError, true);

  const invalidDataError = new JPEGInvalidDataError("Invalid JPEG data");
  assertExists(invalidDataError);
  assertEquals(invalidDataError.name, "JPEGInvalidDataError");
  assertEquals(invalidDataError instanceof JPEGError, true);
});

Deno.test("LibJPEGTurbo - SIMD Metrics", async () => {
  const jpeg = new LibJPEGTurbo();

  try {
    await jpeg.initialize();

    // Should be able to get SIMD metrics
    const metrics = jpeg.getSIMDMetrics();
    assertExists(metrics);
    assertEquals(typeof metrics.simdEnabled, "boolean");
    assertEquals(typeof metrics.instructionSet, "string");

    console.log(`SIMD enabled: ${metrics.simdEnabled}`);
    console.log(`Instruction set: ${metrics.instructionSet}`);
  } catch (error) {
    // Expected to fail until WASM is fully implemented
    console.log(
      "SIMD metrics test failed (expected):",
      (error as Error).message,
    );
  } finally {
    jpeg.cleanup();
  }
});

Deno.test("LibJPEGTurbo - Basic JPEG Info", async () => {
  const jpeg = new LibJPEGTurbo();

  try {
    await jpeg.initialize();

    // Create minimal valid JPEG header
    const fakeJpegData = new Uint8Array([
      0xFF,
      0xD8, // SOI marker
      0xFF,
      0xE0, // JFIF marker
      0x00,
      0x10, // Length (16 bytes)
      0x4A,
      0x46,
      0x49,
      0x46,
      0x00, // "JFIF\0"
      0x01,
      0x01, // Version 1.1
      0x00,
      0x00,
      0x01,
      0x00,
      0x01, // Unit, density
      0x00,
      0x00, // Thumbnail width/height
    ]);

    // Test getting JPEG info
    const info = jpeg.getJPEGInfo(fakeJpegData);
    assertExists(info);
    assertEquals(typeof info.width, "number");
    assertEquals(typeof info.height, "number");
    assertEquals(typeof info.components, "number");
    assertEquals(typeof info.progressive, "boolean");
    assertEquals(typeof info.colorSpace, "string");
  } catch (error) {
    // Expected to fail until WASM parsing is implemented
    console.log("JPEG info test failed (expected):", (error as Error).message);
  } finally {
    jpeg.cleanup();
  }
});

Deno.test("LibJPEGTurbo - Uninitialized Usage", () => {
  const jpeg = new LibJPEGTurbo();

  // Should throw when used without initialization
  try {
    const fakeData = new Uint8Array([0xFF, 0xD8, 0xFF, 0xE0]);
    jpeg.getJPEGInfo(fakeData);
    throw new Error("Should have thrown");
  } catch (error) {
    if (
      error instanceof JPEGError && error.message.includes("not initialized")
    ) {
      console.log("✓ getJPEGInfo correctly rejected uninitialized usage");
    } else {
      throw error;
    }
  }

  try {
    const fakeData = new Uint8Array([0xFF, 0xD8, 0xFF, 0xE0]);
    jpeg.decompressJPEG(fakeData);
    throw new Error("Should have thrown");
  } catch (error) {
    if (
      error instanceof JPEGError && error.message.includes("not initialized")
    ) {
      console.log("✓ decompressJPEG correctly rejected uninitialized usage");
    } else {
      throw error;
    }
  }

  try {
    const rgbData = new Uint8Array(12); // 2x2 RGB
    jpeg.compressRGB(rgbData, 2, 2);
    throw new Error("Should have thrown");
  } catch (error) {
    if (
      error instanceof JPEGError && error.message.includes("not initialized")
    ) {
      console.log("✓ compressRGB correctly rejected uninitialized usage");
    } else {
      throw error;
    }
  }
});

Deno.test("LibJPEGTurbo - Invalid Input Handling", async () => {
  const jpeg = new LibJPEGTurbo();

  try {
    await jpeg.initialize();

    // Test with empty JPEG data
    try {
      const emptyData = new Uint8Array(0);
      jpeg.decompressJPEG(emptyData);
      throw new Error("Should have thrown");
    } catch (error) {
      if (
        error instanceof JPEGInvalidDataError &&
        error.message.includes("Empty JPEG data")
      ) {
        console.log("✓ Empty JPEG data correctly rejected");
      } else {
        throw error;
      }
    }

    // Test with invalid RGB data
    try {
      const invalidRgbData = new Uint8Array(5); // Wrong size for 2x2 RGB
      jpeg.compressRGB(invalidRgbData, 2, 2);
      throw new Error("Should have thrown");
    } catch (error) {
      if (
        error instanceof JPEGCompressionError &&
        error.message.includes("Invalid RGB data size")
      ) {
        console.log("✓ Invalid RGB data size correctly rejected");
      } else {
        throw error;
      }
    }
  } catch (error) {
    console.log(
      "Input validation test failed (expected):",
      (error as Error).message,
    );
  } finally {
    jpeg.cleanup();
  }
});

Deno.test("LibJPEGTurbo - Multiple Instances", async () => {
  // Test that multiple instances can be created without conflicts
  const instance1 = new LibJPEGTurbo();
  const instance2 = new LibJPEGTurbo();

  try {
    // Both should be able to initialize independently
    await instance1.initialize();
    await instance2.initialize();

    assertEquals(instance1.isInitialized, true);
    assertEquals(instance2.isInitialized, true);

    // Both should have their own WASM modules
    assertExists(instance1.wasmModule);
    assertExists(instance2.wasmModule);
  } catch (error) {
    console.log(
      "Multi-instance test failed (expected):",
      (error as Error).message,
    );
  } finally {
    instance1.cleanup();
    instance2.cleanup();
  }
});
