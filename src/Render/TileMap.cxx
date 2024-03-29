#include "TileMap.hxx"
#include "../Util/Assert.hxx"
#include <cassert>
#include <cstdint>
#include <cstdio>

TileMap::TileMap(uint tile_w, uint tile_h, uint num_rows) {
  assume(num_rows < UINT8_MAX && tile_w < UINT8_MAX && tile_h < UINT8_MAX,
         "exceeds uint8");
  this->tile_w = tile_w;
  this->tile_h = tile_h;
  this->bitmap = std::vector<Row>(num_rows);
  this->bitmap_damaged = std::vector<Row>(num_rows);
  Clear();
}

void TileMap::Clear() {
  for (uint8_t i = 0; i < bitmap.size(); i++) {
    bitmap[i] = ~(Row)0;
    bitmap_damaged[i] = ~(Row)0;
  }
}

/* TODO: use atleast_8 */
Point TileMap::AllocateRect(uint w_px, uint h_px) {
  /* convert px to tiles, rounding upward */
  const uint8_t w_tiles = (w_px + tile_w - 1) / tile_w;
  const uint8_t h_tiles = (h_px + tile_h - 1) / tile_h;

  /* TODO: move this check elsewhere */
  assume(w_tiles < 32 && h_tiles < bitmap.size(), "too large for bitmap");

  Row *row = bitmap.data();
  Row *end = bitmap.data() + bitmap.size();

  uint8_t col_idx = 0;
  uint8_t rows_matched = 0;

  while (row < end && rows_matched < h_tiles) {
    /* check if the bitstring at col_idx into the current row has atleast
     * w_tiles bits set/tiles free */
    uint8_t bits_free = __builtin_clz(~(*row << col_idx));
    bool row_match = (w_tiles <= bits_free);

    /* if there was no match, move to the next column */
    uint8_t bits_used = __builtin_clz(((*row << col_idx) | (1 >> col_idx)));
    col_idx += !row_match * (bits_free + bits_used);

    row = row
          /* if we have reached the column limit, move to the next row */
          + (col_idx >= TILEMAP_ROW_BITS)
          /* if the row did not match, backtrack by the amount of rows we've
           * matched so far */
          - (!row_match * rows_matched)
          /* otherwise, advance the row pointer by one */
          + row_match;

    /* if the row matched, increment the number of rows matched by one,
     * otherwise zero it */
    rows_matched = row_match * (rows_matched + 1);

    /* wrap the column around to zero (compiles to &=) */
    if (col_idx >= TILEMAP_ROW_BITS)
      col_idx = 0;
    // col_idx %= TILEMAP_ROW_BITS;
  }
  /* TODO: if (row == end) */

  const uint8_t row_idx = (row - bitmap.data()) - h_tiles;

  /* mark bits as used by setting them to zero */
  const Row write_mask = (~(Row)0 << (TILEMAP_ROW_BITS - w_tiles)) >> (col_idx);
  for (uint8_t y = 0; y < h_tiles; y++) {
    bitmap[row_idx + y] &= ~write_mask;
    bitmap_damaged[row_idx + y] &= ~write_mask;
  }

  return {(uint)col_idx * tile_w, (uint)row_idx * tile_h};
}

bool TileMap::IsRectValid(uint8_t r_epoch, Rect r) {
  if (r_epoch == epoch)
    return true;

  /* coordinates must be at a specific tile */
  assert(r.x % tile_w == 0 && r.y % tile_h);

  const uint8_t y_tiles = r.y / tile_w;
  const uint8_t w_tiles = (r.w + tile_w - 1) / tile_w;
  const uint8_t h_tiles = (r.h + tile_h - 1) / tile_h;
  const Row row_mask = ~(Row)0 << (TILEMAP_ROW_BITS - w_tiles);
  for (uint8_t y = 0; y < h_tiles; y++) {
    if (row_mask != (bitmap_damaged[y_tiles + y] & row_mask)) {
      return false;
    }
  }

  return true;
}