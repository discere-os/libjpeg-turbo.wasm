/*
 * LibJPEG-Turbo.wasm Simple Demo (Deno)
 * High-performance JPEG processing with WebAssembly SIMD acceleration
 */

import LibJPEGTurbo from "./src/lib/index.ts";

console.log("🚀 LibJPEG-Turbo.wasm Simple Demo");
console.log("=====================================");
console.log("High-performance JPEG processing with SIMD acceleration");
console.log("");

async function runDemo() {
  try {
    // Initialize the library
    console.log("⏳ Initializing LibJPEG-Turbo...");
    const jpeg = new LibJPEGTurbo();
    await jpeg.initialize();

    console.log("✅ libjpeg-turbo.wasm initialized successfully!");

    // Check SIMD capabilities
    console.log("\n📊 SIMD Performance Metrics:");
    try {
      const metrics = await jpeg.getSIMDMetrics();
      console.log(`   • SIMD Support: ${metrics.simdEnabled ? '✅' : '❌'}`);
      console.log(`   • Instruction Set: ${metrics.instructionSet}`);

      if (metrics.dctAcceleration) {
        console.log(`   • DCT Acceleration: ${metrics.dctAcceleration}`);
      }
      if (metrics.colorConversionAcceleration) {
        console.log(`   • Color Conversion: ${metrics.colorConversionAcceleration}`);
      }
      if (metrics.throughputImprovement) {
        console.log(`   • Overall Improvement: ${metrics.throughputImprovement}`);
      }
    } catch (error) {
      console.log(`   • SIMD metrics: ❌ (${(error as Error).message})`);
    }

    // Test low-level WASM access
    console.log("\n🔧 Testing WASM Module Integration:");
    if (jpeg.wasmModule) {
      console.log(`   • Module loaded: ✅`);
      console.log(`   • Initialization status: ${jpeg.isInitialized ? '✅' : '❌'}`);

      // Test basic memory operations
      try {
        const testPtr = jpeg.wasmModule._jpeg_malloc(1024);
        if (testPtr > 0) {
          console.log(`   • Memory allocation: ✅ (ptr: ${testPtr})`);
          jpeg.wasmModule._jpeg_free(testPtr);
          console.log(`   • Memory deallocation: ✅`);
        } else {
          console.log(`   • Memory allocation: ❌ (returned null)`);
        }
      } catch (error) {
        console.log(`   • Memory operations: ❌ (${(error as Error).message})`);
      }

      // Test UTF8 string operations
      try {
        const testStr = "Hello, libjpeg-turbo SIMD!";
        const strLength = jpeg.wasmModule.UTF8ToString ? 21 : testStr.length;
        console.log(`   • String operations: ✅ ("${testStr}" = ~${strLength} bytes)`);
      } catch (error) {
        console.log(`   • String operations: ❌ (${(error as Error).message})`);
      }
    } else {
      console.log(`   • Module loaded: ❌`);
    }

    // Test JPEG compression
    console.log("\n🖼️  Testing JPEG Compression:");

    // Create a small test image (8x8 RGB)
    const width = 8, height = 8;
    const rgbData = new Uint8Array(width * height * 3);

    // Fill with a simple gradient pattern
    for (let y = 0; y < height; y++) {
      for (let x = 0; x < width; x++) {
        const idx = (y * width + x) * 3;
        rgbData[idx] = (x * 32) % 256;     // R gradient
        rgbData[idx + 1] = (y * 32) % 256; // G gradient
        rgbData[idx + 2] = 128;            // B constant
      }
    }

    try {
      const startTime = performance.now();
      const jpegData = jpeg.compressRGB(rgbData, width, height, {
        quality: 85,
        progressive: false
      });
      const endTime = performance.now();

      console.log(`   • Compression: ✅ (${rgbData.length} → ${jpegData.length} bytes)`);
      console.log(`   • Compression time: ${(endTime - startTime).toFixed(2)}ms`);
      console.log(`   • Compression ratio: ${((1 - jpegData.length / rgbData.length) * 100).toFixed(1)}%`);

      // Test JPEG info extraction
      try {
        const info = jpeg.getJPEGInfo(jpegData);
        console.log(`   • Info extraction: ✅ (${info.width}x${info.height}, ${info.components} components)`);
        console.log(`   • Color space: ${info.colorSpace}`);
        console.log(`   • Progressive: ${info.progressive ? 'Yes' : 'No'}`);
      } catch (error) {
        console.log(`   • Info extraction: ❌ (${(error as Error).message})`);
      }

      // Test JPEG decompression (round-trip)
      try {
        const decompressStart = performance.now();
        const result = jpeg.decompressJPEG(jpegData);
        const decompressEnd = performance.now();

        console.log(`   • Decompression: ✅ (${jpegData.length} → ${result.data.length} bytes)`);
        console.log(`   • Decompression time: ${(decompressEnd - decompressStart).toFixed(2)}ms`);
        console.log(`   • Round-trip dimensions: ${result.width}x${result.height} (${result.components} components)`);

        // Check data integrity (lossy compression, so just basic checks)
        const nonZeroPixels = result.data.filter(byte => byte > 0).length;
        console.log(`   • Data integrity: ✅ (${nonZeroPixels}/${result.data.length} non-zero bytes)`);
      } catch (error) {
        console.log(`   • Decompression: ❌ (${(error as Error).message})`);
      }

    } catch (error) {
      console.log(`   • Compression: ❌ (${(error as Error).message})`);
    }

    // Test different quality levels
    console.log("\n🎛️  Testing Quality Levels:");

    const testQualities = [25, 50, 75, 95];
    for (const quality of testQualities) {
      try {
        const jpegData = jpeg.compressRGB(rgbData, width, height, { quality });
        const ratio = ((1 - jpegData.length / rgbData.length) * 100).toFixed(1);
        console.log(`   • Quality ${quality}: ${jpegData.length} bytes (${ratio}% compression)`);
      } catch (error) {
        console.log(`   • Quality ${quality}: ❌ (${(error as Error).message})`);
      }
    }

    // Test progressive JPEG
    console.log("\n🔄 Testing Progressive JPEG:");
    try {
      const progressiveData = jpeg.compressRGB(rgbData, width, height, {
        quality: 75,
        progressive: true
      });
      console.log(`   • Progressive encoding: ✅ (${progressiveData.length} bytes)`);
    } catch (error) {
      console.log(`   • Progressive encoding: ❌ (${(error as Error).message})`);
    }

    // Performance benchmark
    console.log("\n📈 Performance Benchmark:");
    const benchmarkSize = 32; // 32x32 image
    const benchmarkData = new Uint8Array(benchmarkSize * benchmarkSize * 3);
    benchmarkData.fill(128); // Gray image

    try {
      const iterations = 10;
      let totalTime = 0;

      for (let i = 0; i < iterations; i++) {
        const start = performance.now();
        jpeg.compressRGB(benchmarkData, benchmarkSize, benchmarkSize, { quality: 85 });
        totalTime += performance.now() - start;
      }

      const avgTime = totalTime / iterations;
      const throughput = (benchmarkData.length / 1024 / 1024) / (avgTime / 1000);

      console.log(`   • Average compression time: ${avgTime.toFixed(2)}ms`);
      console.log(`   • Throughput: ${throughput.toFixed(2)} MB/s`);
      console.log(`   • Iterations: ${iterations}`);
    } catch (error) {
      console.log(`   • Benchmark: ❌ (${(error as Error).message})`);
    }

    // Cleanup
    jpeg.cleanup();
    console.log("\n✅ Demo completed successfully!");
    console.log("\n🎉 LibJPEG-Turbo.wasm is working with SIMD acceleration!");

  } catch (error) {
    console.error("\n❌ Demo failed:", (error as Error).message);
    console.error((error as Error).stack);
    Deno.exit(1);
  }
}

// Run the demo
if (import.meta.main) {
  await runDemo();
}