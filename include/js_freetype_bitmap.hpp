#ifndef JS_FREETYPE_BITMAP_HPP
#define JS_FREETYPE_BITMAP_HPP

#ifdef HAVE_RAW_FREETYPE2

#include <ft2build.h>
#include FT_FREETYPE_H
#include <opencv2/core.hpp>
#include <algorithm>
#include <string>

/**
 * @brief Renders bitmap-strike (embedded fixed-size, non-scalable) glyphs
 * directly via raw FreeType2, bypassing cv::freetype::FreeType2.
 *
 * See BUGS: opencv-freetype-bitmap-strike-glyphs-not-rasterized -
 * cv::freetype::FreeType2::putText()/getTextSize() don't rasterize such
 * fonts correctly (every glyph collapses to a 1px sliver). This class loads
 * the same font file a second time through FreeType2's own C API, whose
 * bitmap-strike handling isn't affected by that bug, and blits/measures
 * glyphs from the real embedded bitmap data. Only meant for fonts where
 * `isBitmapStrike()` is true; scalable fonts should keep going through
 * cv::freetype::FreeType2, which rasterizes those correctly already.
 *
 * Caches a single loaded face, mirroring js_draw.cpp's existing
 * freetype2/freetype2_face single-slot cache for cv::freetype::FreeType2.
 */
class FreeTypeBitmapFont {
public:
  ~FreeTypeBitmapFont() {
    if(face)
      FT_Done_Face(face);
    if(library)
      FT_Done_FreeType(library);
  }

  // Loads (or reuses the cached) font file. Throws cv::Exception on failure.
  void load(const std::string& fontFile) {
    if(fontFile == loadedFile && face)
      return;

    if(!library) {
      if(FT_Init_FreeType(&library))
        throw cv::Exception(cv::Error::StsError, "FT_Init_FreeType failed", __func__, __FILE__, __LINE__);
    }

    if(face) {
      FT_Done_Face(face);
      face = nullptr;
    }

    if(FT_New_Face(library, fontFile.c_str(), 0, &face)) {
      face = nullptr;
      throw cv::Exception(cv::Error::StsError, "FT_New_Face failed: " + fontFile, __func__, __FILE__, __LINE__);
    }

    loadedFile = fontFile;
  }

  bool isBitmapStrike() const { return face && !FT_IS_SCALABLE(face); }

  // size.height = ascent (top-of-ink to baseline), *baseline = descent
  // (baseline to bottom-of-ink) - same convention js_get_text_size already
  // returns for the (working) scalable-font path, computed here from the
  // real per-glyph bitmap_top/bitmap.rows instead of substituted.
  cv::Size getTextSize(const std::string& text, int pixelSize, int* baseline) {
    selectSize(pixelSize);

    int width = 0, ascent = 0, descent = 0;

    forEachGlyph(text, [&](FT_GlyphSlot glyph) {
      width += glyph->advance.x >> 6;
      ascent = std::max(ascent, glyph->bitmap_top);
      descent = std::max(descent, (int)glyph->bitmap.rows - glyph->bitmap_top);
    });

    *baseline = descent;
    return cv::Size(width, ascent);
  }

  void putText(cv::Mat& dst, const std::string& text, cv::Point org, int pixelSize, const cv::Scalar& color, bool bottomLeftOrigin) {
    selectSize(pixelSize);

    int baseline;
    cv::Size size = getTextSize(text, pixelSize, &baseline);
    cv::Point pen(org.x, bottomLeftOrigin ? org.y : org.y + size.height);

    forEachGlyph(text, [&](FT_GlyphSlot glyph) {
      blit(dst, glyph->bitmap, pen.x + glyph->bitmap_left, pen.y - glyph->bitmap_top, color);
      pen.x += glyph->advance.x >> 6;
    });
  }

private:
  void selectSize(int pixelSize) {
    if(FT_Set_Pixel_Sizes(face, 0, pixelSize))
      throw cv::Exception(
          cv::Error::StsError, cv::format("FT_Set_Pixel_Sizes(%d) failed - not one of this font's fixed sizes", pixelSize), __func__, __FILE__, __LINE__);
  }

  // Decodes `text` as UTF-8 and invokes `fn` with each glyph's loaded and
  // rendered FT_GlyphSlot in turn, advancing FreeType's own pen for
  // FT_LOAD_RENDER's benefit (the bitmap itself doesn't depend on pen
  // position, but keeping the face's internal state consistent is cheap
  // and avoids relying on undocumented behavior).
  template<class Fn> void forEachGlyph(const std::string& text, Fn fn) {
    size_t i = 0;

    while(i < text.size()) {
      uint32_t cp;
      unsigned char c = text[i];

      if(c < 0x80) {
        cp = c;
        i += 1;
      } else if((c & 0xE0) == 0xC0 && i + 1 < text.size()) {
        cp = (c & 0x1F) << 6 | (text[i + 1] & 0x3F);
        i += 2;
      } else if((c & 0xF0) == 0xE0 && i + 2 < text.size()) {
        cp = (c & 0x0F) << 12 | (text[i + 1] & 0x3F) << 6 | (text[i + 2] & 0x3F);
        i += 3;
      } else if((c & 0xF8) == 0xF0 && i + 3 < text.size()) {
        cp = (c & 0x07) << 18 | (text[i + 1] & 0x3F) << 12 | (text[i + 2] & 0x3F) << 6 | (text[i + 3] & 0x3F);
        i += 4;
      } else {
        cp = c;
        i += 1;
      }

      FT_UInt glyph_index = FT_Get_Char_Index(face, cp);

      if(FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER))
        continue;

      fn(face->glyph);
    }
  }

  static void blit(cv::Mat& dst, const FT_Bitmap& bitmap, int x0, int y0, const cv::Scalar& color) {
    int channels = dst.channels();

    for(unsigned r = 0; r < bitmap.rows; ++r) {
      int y = y0 + (int)r;

      if(y < 0 || y >= dst.rows)
        continue;

      const unsigned char* row = bitmap.buffer + (size_t)r * (bitmap.pitch < 0 ? -bitmap.pitch : bitmap.pitch);

      for(unsigned c = 0; c < bitmap.width; ++c) {
        int x = x0 + (int)c;

        if(x < 0 || x >= dst.cols)
          continue;

        int coverage;

        if(bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
          coverage = (row[c >> 3] >> (7 - (c & 7))) & 1 ? 255 : 0;
        else
          coverage = row[c];

        if(coverage == 0)
          continue;

        double a = coverage / 255.0;
        unsigned char* px = dst.ptr<unsigned char>(y) + (size_t)x * channels;

        for(int ch = 0; ch < channels; ++ch)
          px[ch] = (unsigned char)(px[ch] * (1 - a) + color[ch] * a);
      }
    }
  }

  FT_Library library = nullptr;
  FT_Face face = nullptr;
  std::string loadedFile;
};

#endif /* HAVE_RAW_FREETYPE2 */
#endif /* JS_FREETYPE_BITMAP_HPP */
