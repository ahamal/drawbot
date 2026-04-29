// Binarize an ImageData. Pixels with luminance < `value` become 1
// (foreground, "the lines we want to plot"). Set `invert: true` to
// flip — useful for white-on-black sources like chalkboards or
// photographic negatives.
//
// Luminance uses the standard Rec. 601 weights so colored sources
// behave like a human-eye grayscale conversion.

export function threshold(imageData, value, invert = false) {
  const { data, width, height } = imageData;
  const out = new Uint8Array(width * height);
  for (let i = 0; i < width * height; i++) {
    const r = data[i * 4];
    const g = data[i * 4 + 1];
    const b = data[i * 4 + 2];
    const lum = 0.299 * r + 0.587 * g + 0.114 * b;
    const dark = lum < value;
    out[i] = (invert ? !dark : dark) ? 1 : 0;
  }
  return out;
}
