#!/usr/bin/env -S deno run --allow-all

/**
 * NPM Package Builder for libjpeg-turbo.wasm
 * Creates NPM-compatible package from Deno library
 */

import { build, emptyDir } from "https://deno.land/x/dnt@0.38.1/mod.ts";

await emptyDir("./npm");

await build({
  entryPoints: ["./src/lib/index.ts"],
  outDir: "./npm",
  shims: {
    // provide deno runtime APIs
    deno: true,
  },
  package: {
    // package.json properties
    name: "@discere-os/libjpeg-turbo.wasm",
    version: "1.0.0",
    description: "WebAssembly port of libjpeg-turbo with SIMD optimization",
    keywords: [
      "jpeg",
      "compression",
      "image",
      "webassembly",
      "wasm",
      "simd",
      "turbo",
      "performance"
    ],
    homepage: "https://github.com/discere-os/libjpeg-turbo.wasm",
    repository: {
      type: "git",
      url: "git+https://github.com/discere-os/libjpeg-turbo.wasm.git"
    },
    bugs: {
      url: "https://github.com/discere-os/libjpeg-turbo.wasm/issues"
    },
    author: "Superstruct Ltd <developers@superstruct.tech>",
    license: "IJG AND BSD-3-Clause",
    main: "./esm/lib/index.js",
    module: "./esm/lib/index.js",
    types: "./esm/lib/index.d.ts",
    exports: {
      ".": {
        import: "./esm/lib/index.js",
        require: "./script/lib/index.js",
        types: "./esm/lib/index.d.ts"
      },
      "./types": {
        import: "./esm/lib/types.js",
        require: "./script/lib/types.js",
        types: "./esm/lib/types.d.ts"
      }
    },
    files: [
      "esm/",
      "script/",
      "install/",
      "README.md",
      "LICENSE"
    ],
    engines: {
      node: ">=16.0.0"
    }
  },
  postBuild() {
    // Copy WASM artifacts to npm package
    Deno.copyFileSync("install/wasm/libjpeg-turbo-side.wasm", "npm/install/wasm/libjpeg-turbo-side.wasm");
    Deno.copyFileSync("install/wasm/libjpeg-turbo-release.js", "npm/install/wasm/libjpeg-turbo-release.js");
    Deno.copyFileSync("install/wasm/libjpeg-turbo-fallback.js", "npm/install/wasm/libjpeg-turbo-fallback.js");
    Deno.copyFileSync("README.md", "npm/README.md");
    Deno.copyFileSync("LICENSE", "npm/LICENSE");
  },
});

console.log("📦 NPM package built successfully in ./npm/");