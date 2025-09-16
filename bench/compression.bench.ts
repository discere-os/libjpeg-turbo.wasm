import LibJPEGTurbo from "../src/lib/index.ts";

// Helper function to create test image data
function createTestImage(
  width: number,
  height: number,
  channels: number,
): Uint8Array {
  const imageData = new Uint8Array(width * height * channels);

  // Create a more realistic test pattern with gradients and noise
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const idx = (y * width + x) * channels;

      // Base gradient
      const r = (x / width) * 255;
      const g = (y / height) * 255;
      const b = ((x + y) / (width + height)) * 255;

      // Add some noise for more realistic compression
      const noise = Math.sin(x * 0.1) * Math.cos(y * 0.1) * 20;

      imageData[idx] = Math.max(0, Math.min(255, r + noise));
      if (channels > 1) {
        imageData[idx + 1] = Math.max(0, Math.min(255, g + noise));
      }
      if (channels > 2) {
        imageData[idx + 2] = Math.max(0, Math.min(255, b + noise));
      }
      if (channels > 3) imageData[idx + 3] = 255; // Alpha channel
    }
  }

  return imageData;
}

// Global JPEG instance for reuse across benchmarks
const jpeg = new LibJPEGTurbo();

// Initialize before benchmarks
await jpeg.initialize();

Deno.bench("JPEG Compression - Small Image (64x64 RGB)", () => {
  const width = 64, height = 64, channels = 3;
  const imageData = createTestImage(width, height, channels);

  jpeg.compressRGB(imageData, width, height, {
    quality: 85,
  });
});

Deno.bench("JPEG Compression - Medium Image (256x256 RGB)", () => {
  const width = 256, height = 256, channels = 3;
  const imageData = createTestImage(width, height, channels);

  jpeg.compressRGB(imageData, width, height, {
    quality: 85,
  });
});

Deno.bench("JPEG Compression - Large Image (512x512 RGB)", () => {
  const width = 512, height = 512, channels = 3;
  const imageData = createTestImage(width, height, channels);

  jpeg.compressRGB(imageData, width, height, {
    quality: 85,
  });
});

Deno.bench("JPEG Compression - Quality 25 (High Compression)", () => {
  const width = 128, height = 128, channels = 3;
  const imageData = createTestImage(width, height, channels);

  jpeg.compressRGB(imageData, width, height, {
    quality: 25,
  });
});

Deno.bench("JPEG Compression - Quality 95 (Low Compression)", () => {
  const width = 128, height = 128, channels = 3;
  const imageData = createTestImage(width, height, channels);

  jpeg.compressRGB(imageData, width, height, {
    quality: 95,
  });
});

Deno.bench("JPEG Compression - Progressive Mode", () => {
  const width = 128, height = 128, channels = 3;
  const imageData = createTestImage(width, height, channels);

  jpeg.compressRGB(imageData, width, height, {
    quality: 85,
    progressive: true,
  });
});

Deno.bench("JPEG Round-trip (Compression + Decompression)", () => {
  const width = 64, height = 64, channels = 3;
  const imageData = createTestImage(width, height, channels);

  // Compress
  const jpegData = jpeg.compressRGB(imageData, width, height, {
    quality: 85,
  });

  // Decompress
  jpeg.decompressJPEG(jpegData);
});

Deno.bench("JPEG Info Extraction", () => {
  const width = 64, height = 64, channels = 3;
  const imageData = createTestImage(width, height, channels);

  // Compress first
  const jpegData = jpeg.compressRGB(imageData, width, height, {
    quality: 85,
  });

  // Extract info (should be fast)
  try {
    jpeg.getJPEGInfo(jpegData);
  } catch (_error) {
    // Expected to fail until JPEG parsing is fully implemented
  }
});

Deno.bench("SIMD Metrics Retrieval", () => {
  jpeg.getSIMDMetrics();
});

Deno.bench("Memory Management (Allocate/Free Cycle)", () => {
  const size = 1024 * 1024; // 1MB

  if (jpeg.wasmModule) {
    const ptr = jpeg.wasmModule._jpeg_malloc(size);
    if (ptr) {
      jpeg.wasmModule._jpeg_free(ptr);
    }
  }
});

// Cleanup after benchmarks
globalThis.addEventListener("unload", () => {
  jpeg?.cleanup();
});
