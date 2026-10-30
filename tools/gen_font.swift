// Converts a few characters of a TTF into an LVGL 8 font (like lv_font_conv's output), rendered with macOS
// CoreText. No kerning. Used for the big home clock and the big letters of the kids' alphabet game.
//
//   [CHARS="..."] [BPP=2|4] swift tools/gen_font.swift <font.ttf[,more.ttf...]> <size px> <name> <out.c> [preview.png]
//
// CHARS: the characters to include (default: the clock's " -0123456789:"). A character missing from the first font
// is taken from the next one (Google Fonts split Vietnamese letters into a separate file). BPP: 4 (default) or 2.
// The preview shows the clock strings, or CHARS when it is given.
import CoreGraphics
import CoreText
import Foundation
import ImageIO
import UniformTypeIdentifiers

let args = CommandLine.arguments
guard args.count >= 5, let size = Double(args[2]) else {
  print("usage: [CHARS=...] [BPP=2|4] gen_font.swift <font.ttf[,more.ttf]> <size> <name> <out.c> [preview.png]")
  exit(1)
}
let name = args[3]
let env = ProcessInfo.processInfo.environment
let bpp = Int(env["BPP"] ?? "4") ?? 4
let fonts: [CTFont] = args[1].split(separator: ",").map { path in
  guard let descs = CTFontManagerCreateFontDescriptorsFromURL(URL(fileURLWithPath: String(path)) as CFURL) as? [CTFontDescriptor],
    let desc = descs.first
  else {
    print("cannot read \(path)")
    exit(1)
  }
  return CTFontCreateWithFontDescriptor(desc, size, nil)
}
let charsText = env["CHARS"]
var chars: [UInt32] = charsText.map { Array(Set($0.unicodeScalars.map { $0.value })).sorted() } ?? ([0x20, 0x2D] + Array(0x30...0x3A))

struct Glyph {
  var cp: UInt32, adv: Double, w: Int, h: Int, ox: Int, oy: Int, px: [UInt8]
}

func render(_ cp: UInt32) -> Glyph {
  var u = [UniChar(cp)], g: [CGGlyph] = [0]
  var font = fonts[0]
  for f in fonts where CTFontGetGlyphsForCharacters(f, &u, &g, 1) {
    font = f
    break
  }
  if g[0] == 0 { print("warning: U+\(String(cp, radix: 16)) is in none of the fonts") }
  var adv = CGSize.zero
  CTFontGetAdvancesForGlyphs(font, .horizontal, &g, &adv, 1)
  let r = CTFontGetBoundingRectsForGlyphs(font, .horizontal, &g, nil, 1)
  if cp == 0x20 || r.width <= 0 { return Glyph(cp: cp, adv: adv.width, w: 0, h: 0, ox: 0, oy: 0, px: []) }
  let x0 = Int(floor(r.minX)), y0 = Int(floor(r.minY))
  let w = Int(ceil(r.maxX)) - x0, h = Int(ceil(r.maxY)) - y0
  let ctx = CGContext(data: nil, width: w, height: h, bitsPerComponent: 8, bytesPerRow: w, space: CGColorSpaceCreateDeviceGray(),
                      bitmapInfo: CGImageAlphaInfo.none.rawValue)!
  ctx.setFillColor(gray: 0, alpha: 1)
  ctx.fill(CGRect(x: 0, y: 0, width: w, height: h))
  ctx.setFillColor(gray: 1, alpha: 1)
  var pos = CGPoint(x: -Double(x0), y: -Double(y0))
  CTFontDrawGlyphs(font, &g, &pos, 1, ctx)
  let data = ctx.data!.bindMemory(to: UInt8.self, capacity: w * h)  // row 0 = top
  return Glyph(cp: cp, adv: adv.width, w: w, h: h, ox: x0, oy: y0, px: Array(UnsafeBufferPointer(start: data, count: w * h)))
}

let glyphs = chars.map(render)
let inked = glyphs.filter { $0.h > 0 }
let top = inked.map { $0.oy + $0.h }.max()!, bottom = inked.map { $0.oy }.min()!

// a continuous bit stream per glyph (no row padding, first pixel in the high bits), as LVGL's bitmap_format 0 expects
var bitmap: [UInt8] = [], index: [Int] = []
let maxV = (1 << bpp) - 1, perByte = 8 / bpp
for g in glyphs {
  index.append(bitmap.count)
  var v: [Int] = g.px.map { (Int($0) * maxV + 127) / 255 }
  while v.count % perByte != 0 { v.append(0) }
  for i in stride(from: 0, to: v.count, by: perByte) {
    var b = 0
    for k in 0..<perByte { b = b << bpp | v[i + k] }
    bitmap.append(UInt8(b))
  }
}

var out = """
  /* \(name): \(args[1].split(separator: ",").map { URL(fileURLWithPath: String($0)).lastPathComponent }.joined(separator: " + ")) at \(Int(size)) px, \(bpp) bpp, \(chars.count) characters.
   * Made by tools/gen_font.swift. */
  #include "lvgl.h"

  static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {

  """
for i in stride(from: 0, to: bitmap.count, by: 16) {
  out += "    " + bitmap[i..<min(i + 16, bitmap.count)].map { String(format: "0x%02x", $0) }.joined(separator: ", ") + ",\n"
}
out += "};\n\nstatic const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {\n"
out += "    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,\n"
for (i, g) in glyphs.enumerated() {
  out += "    {.bitmap_index = \(index[i]), .adv_w = \(Int((g.adv * 16).rounded())), .box_w = \(g.w), .box_h = \(g.h), .ofs_x = \(g.ox), .ofs_y = \(g.oy)},\n"
}
let first = chars.first!
let ofs = chars.map { String($0 - first) }.joined(separator: ", ")
out += """
  };

  static const uint16_t unicode_list_0[] = {\(ofs)};

  static const lv_font_fmt_txt_cmap_t cmaps[] = {
      {.range_start = \(first), .range_length = \(chars.last! - first + 1), .glyph_id_start = 1, .unicode_list = unicode_list_0,
       .glyph_id_ofs_list = NULL, .list_length = \(chars.count), .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY},
  };

  static lv_font_fmt_txt_glyph_cache_t cache;
  static const lv_font_fmt_txt_dsc_t font_dsc = {
      .glyph_bitmap = glyph_bitmap,
      .glyph_dsc = glyph_dsc,
      .cmaps = cmaps,
      .kern_dsc = NULL,
      .kern_scale = 0,
      .cmap_num = 1,
      .bpp = \(bpp),
      .kern_classes = 0,
      .bitmap_format = 0,
      .cache = &cache,
  };

  const lv_font_t \(name) = {
      .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
      .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
      .line_height = \(top - bottom),
      .base_line = \(-bottom),
      .subpx = LV_FONT_SUBPX_NONE,
      .underline_position = 0,
      .underline_thickness = 0,
      .dsc = &font_dsc,
  };

  """
try! out.write(toFile: args[4], atomically: true, encoding: .utf8)
let sample = charsText == nil ? "12:34" : String(charsText!.prefix(6))
let w12 = sample.unicodeScalars.map { u in glyphs.first { $0.cp == u.value }?.adv ?? 0 }.reduce(0, +)
print("\(name): line height \(top - bottom), \"\(sample)\" is \(Int(w12)) px wide, \(bitmap.count) bytes of bitmaps")

// preview: white text on dark blue, the glyph boxes placed exactly like LVGL places them
if args.count >= 6 {
  let lines = charsText == nil ? ["12:34", "05:67", "89:--"] : stride(from: 0, to: charsText!.count, by: 10).map {
    String(Array(charsText!)[$0..<min($0 + 10, charsText!.count)])
  }
  let lh = top - bottom
  let W = Int(size) * 8, H = lines.count * (lh + 12) + 12
  let ctx = CGContext(data: nil, width: W, height: H, bitsPerComponent: 8, bytesPerRow: W * 4, space: CGColorSpaceCreateDeviceRGB(),
                      bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue)!
  ctx.setFillColor(red: 0.07, green: 0.1, blue: 0.25, alpha: 1)
  ctx.fill(CGRect(x: 0, y: 0, width: W, height: H))
  for (li, line) in lines.enumerated() {
    var x = 10.0
    let base = Double(H - 12 - li * (lh + 12) - top)  // CG y up
    for u in line.unicodeScalars {
      let g = glyphs.first { $0.cp == u.value }!
      for yy in 0..<g.h {
        for xx in 0..<g.w {
          let v = Double(g.px[yy * g.w + xx]) / 255
          if v == 0 { continue }
          ctx.setFillColor(red: 1, green: 1, blue: 1, alpha: v)
          ctx.fill(CGRect(x: x + Double(g.ox + xx), y: base + Double(g.oy + g.h - 1 - yy), width: 1, height: 1))
        }
      }
      x += (g.adv * 16).rounded() / 16
    }
  }
  let dest = CGImageDestinationCreateWithURL(URL(fileURLWithPath: args[5]) as CFURL, UTType.png.identifier as CFString, 1, nil)!
  CGImageDestinationAddImage(dest, ctx.makeImage()!, nil)
  CGImageDestinationFinalize(dest)
}
