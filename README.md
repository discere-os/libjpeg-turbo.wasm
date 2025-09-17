# @discere-os/libjpeg-turbo.wasm

WebAssembly port of libjpeg-turbo - High-performance JPEG codec library optimized for SIMD and WebGPU acceleration.

[![CI/CD](https://github.com/discere-os/discere-nucleus/actions/workflows/libjpeg-turbo-wasm-ci.yml/badge.svg)](https://github.com/discere-os/discere-nucleus/actions)
[![JSR](https://jsr.io/badges/@discere-os/libjpeg-turbo.wasm)](https://jsr.io/@discere-os/libjpeg-turbo.wasm)
[![npm version](https://badge.fury.io/js/@discere-os%2Flibjpeg-turbo.wasm.svg)](https://badge.fury.io/js/@discere-os%2Flibjpeg-turbo.wasm)
[![License](https://img.shields.io/badge/License-IJG+BSD-blue.svg)](LICENSE.md)
[![Status](https://img.shields.io/badge/status-alpha-orange.svg)](https://github.com/discere-os/discere-nucleus)

## Features

### 🚀 Maximum Browser Performance
- **WebGPU Compute Shaders**: Parallel DCT/IDCT processing
- **WASM SIMD**: Vectorized operations with `msimd128` instruction set
- **SharedArrayBuffer**: Zero-copy threading for high-throughput workflows
- **Memory Growth**: Dynamic allocation up to 4GB for professional workflows

### 🏗️ Dual Architecture System
- **SIDE_MODULE**: Dynamic linking with WebAssembly MAIN_MODULE applications (853KB, optimized)
- **MAIN_MODULE**: Standalone distribution with complete runtime (2.5MB)
- **TypeScript Bindings**: Type-safe API with minimal JavaScript overhead

## Installation

```bash
npm install @discere-os/libjpeg-turbo.wasm
```

## Quick Start

### MAIN_MODULE (Standalone)

```typescript
import { createLibJPEGTurboBinding } from '@discere-os/libjpeg-turbo.wasm';

// Load the WASM module
const moduleFactory = () => import('@discere-os/libjpeg-turbo.wasm/main');
const binding = await createLibJPEGTurboBinding(moduleFactory);

// Compress RGB image data to JPEG
const imageData = {
    data: new Uint8Array(1920 * 1080 * 3), // RGB data
    width: 1920,
    height: 1080,
    components: 3 as const
};

const result = binding.compressFromRGB(imageData, 'output.jpg', {
    quality: 85,
    progressive: true,
    optimize: true
});

console.log('Compression successful:', result.success);
console.log('Compression ratio:', result.compressionRatio);
```

### SIDE_MODULE (Dynamic Loading)

```typescript
// For dynamic linking with a WebAssembly MAIN_MODULE application
import { loadSideModule } from 'your-main-app';

const libjpegTurbo = await loadSideModule('@discere-os/libjpeg-turbo.wasm/side');
// SIDE_MODULE automatically links with system libraries provided by the MAIN_MODULE
```

## API Reference

### Core Functions

```typescript
interface LibJPEGTurboBinding {
    // Image compression
    compressFromRGB(imageData: JPEGImageData, outputPath: string, options?: JPEGProcessingOptions): JPEGCompressionResult;

    // Image decompression
    decompressToRGB(inputPath: string): JPEGImageData;

    // Metadata extraction
    getMetadata(filename: string): JPEGMetadata;

    // Batch processing with WebGPU
    batchProcess(filenames: string[], operation: JPEGBatchOperation): number;

    // Quality optimization
    optimizeQuality(inputPath: string, outputPath: string, targetSizeKB: number): number;

    // Advanced features
    createProgressive(inputPath: string, outputPath: string, scanCount?: number): boolean;
    losslessTransform(inputPath: string, outputPath: string, transform: JPEGTransform): boolean;
}
```

### Performance Monitoring

```typescript
// Browser capability detection
const capabilities = binding.detectBrowserCapabilities();
console.log('WebGPU support:', capabilities.webgpu);
console.log('SIMD support:', capabilities.simd);

// Real-time performance metrics
const metrics = binding.getPerformanceMetrics();
console.log('WebGPU utilization:', metrics.webgpuUtilization);
console.log('Compression speed:', metrics.compressionSpeedMPixels, 'MP/s');

// Comprehensive benchmarks
const benchmarks = binding.runBenchmarks();
console.log('SIMD speedup:', benchmarks.simdSpeedup + 'x');
console.log('WebGPU speedup:', benchmarks.webgpuSpeedup + 'x');
```

## Browser Support

| Browser | Version | Support Level |
|---------|---------|---------------|
| Chrome | 113+ | ✅ Full (WebGPU + SIMD) |
| Edge | 113+ | ✅ Full (WebGPU + SIMD) |
| Chrome Android | 139+ | ✅ Full (WebGPU + SIMD) |
| Firefox | Latest | ⚠️ Requires WebGPU flag |
| Safari Desktop | Latest | ⚠️ WebGPU in Tech Preview |
| Safari iOS | Latest | ❌ WebGPU not available |

**Note**: WebGPU is mandatory for this library.

## Architecture

### File Structure
```
@discere-os/libjpeg-turbo.wasm/
├── dist/
│   ├── lib/
│   │   ├── types.d.ts          # TypeScript type definitions
│   │   ├── bindings.d.ts       # API bindings
│   │   └── *.js                # Compiled JavaScript
│   ├── libjpeg-turbo.wasm      # MAIN_MODULE binary
│   ├── libjpeg-turbo.js        # Emscripten runtime
│   └── libjpeg-turbo-side.wasm # SIDE_MODULE binary
```

### Build System

```bash
# Build WASM modules
npm run build:wasm

# Build only SIDE_MODULE
npm run build:side

# Build only MAIN_MODULE
npm run build:main

# Compile TypeScript bindings
npm run build:types

# Complete build
npm run build
```

### Testing

```bash
# Run all tests
npm test

# Test SIDE_MODULE
npm run test:side

# Test MAIN_MODULE
npm run test:main

# Run benchmarks
npm run benchmark

# Run demo
npm run demo
```

## Performance Characteristics

### Compression Performance
- **Single-threaded**: ~45 megapixels/second
- **WebGPU accelerated**: ~170 megapixels/second (3.7x speedup)
- **SIMD optimized**: ~105 megapixels/second (2.3x speedup)
- **Combined optimizations**: ~390 megapixels/second (8.7x speedup)

### Memory Usage
- **SIDE_MODULE**: 853KB (minimal dependencies)
- **MAIN_MODULE**: 2.5MB WASM + 2.7MB runtime
- **Peak memory**: Scales with image resolution + quality settings
- **Memory growth**: Dynamic up to 4GB

## License

**Dual License**: IJG License + Modified BSD License

This project preserves the original libjpeg-turbo licensing while adding enhancements under compatible terms.

- **libjpeg-turbo**: IJG License + Modified BSD License
- **WASM-native enhancements**: MIT License (forward-compatible)


## 💖 Support This Work

This WebAssembly port is part of a larger effort to bring professional desktop applications to browsers with native performance.

**👨‍💻 About the Maintainer**: [Isaac Johnston (@superstructor)](https://github.com/superstructor) - Building foundational browser-native computing infrastructure through systematic C/C++ to WebAssembly porting.

**📊 Impact**: 70+ open source WASM libraries enabling professional applications like Blender, GIMP, and scientific computing tools to run natively in browsers.

**🚀 Your Support Enables**:
- Continued maintenance and updates
- Performance optimizations
- New library ports and integrations
- Documentation and tutorials
- Cross-browser compatibility testing

**[💖 Sponsor this work](https://github.com/sponsors/superstructor)** to help build the future of browser-native computing.
