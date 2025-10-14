/**
 * Type definitions for LibjpegTurbo WASM
 */

export interface LIBJPEG_TURBOModule {
  _malloc: (size: number) => number
  _free: (ptr: number) => void
  HEAPU8: Uint8Array
  setValue: (ptr: number, value: number, type: string) => void
  getValue: (ptr: number, type: string) => number
}

export class LIBJPEG_TURBOError extends Error {
  constructor(message: string) {
    super(message)
    this.name = 'LIBJPEG_TURBOError'
  }
}
