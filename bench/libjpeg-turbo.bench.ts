/**
 * LibjpegTurbo WASM Benchmarks
 */

import LibjpegTurboWASM from "../src/lib/index.ts"

Deno.bench("libjpeg-turbo initialization", {
  baseline: true
}, async () => {
  const lib = new LibjpegTurboWASM()
  await lib.initialize()
})
