#include "bmp.h"
#include "../math/color.h"
#include "gtest/gtest.h"
#include <cstdio>
#include <fstream>
#include <iterator>

namespace {

std::vector<uint8_t> read_file(const char *name) {
  std::ifstream file(name, std::ios::binary);
  return std::vector<uint8_t>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

TEST(BMP, FitsRejectsInvalidAndHugeImages) {
  EXPECT_TRUE( bmp_fits(1280, 720) );
  EXPECT_FALSE( bmp_fits(0, 720) );
  EXPECT_FALSE( bmp_fits(1280, -1) );
  EXPECT_FALSE( bmp_fits(50000, 50000) );   // 7,5 GB, mehr als 4 GiB
}

// 3 x 2 Pixel: jede Zeile hat 9 Byte Farbe und 3 Fuellbytes, die Zeilen stehen von unten nach oben in der Datei
TEST(BMP, WritesHeaderPaddingAndBottomUpRows) {
  std::vector<uint32_t> pixels = {0xff0000, 0x00ff00, 0x0000ff,
                                  0x102030, 0x405060, 0x708090};
  ASSERT_TRUE( save_bmp("test_image.bmp", 3, 2, pixels) );
  std::vector<uint8_t> data = read_file("test_image.bmp");
  std::remove("test_image.bmp");

  ASSERT_EQ(54u + 2u * 12u, data.size());
  EXPECT_EQ('B', data[0]);
  EXPECT_EQ('M', data[1]);
  EXPECT_EQ(24, data[28]);   // Bits pro Pixel

  // erste gespeicherte Zeile ist die untere Bildzeile, Reihenfolge Blau, Gruen, Rot
  EXPECT_EQ(0x30, data[54]);
  EXPECT_EQ(0x20, data[55]);
  EXPECT_EQ(0x10, data[56]);
  EXPECT_EQ(0, data[63]);    // Fuellbyte
  // zweite gespeicherte Zeile beginnt mit dem roten Pixel oben links
  EXPECT_EQ(0x00, data[66]);
  EXPECT_EQ(0x00, data[67]);
  EXPECT_EQ(0xff, data[68]);
}

TEST(COLOR, RenderPixelClampsChannels) {
  std::vector<uint32_t> pixels(4, 0u);
  render_pixel(pixels, 2, {2.f, 0.5f, -1.f}, 1, 1);
  EXPECT_EQ(0xff7f00u, pixels[3]);
}

}
