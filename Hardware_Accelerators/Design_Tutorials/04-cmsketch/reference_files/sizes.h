#pragma once

#define hash_bloom 0x7ffff
#define cm_cols 14
#define cm_rows 4
#define docTag 0xffffffff

const unsigned int cm_col_count = (1<<cm_cols);
const unsigned int cm_seeds[] = {
  0xdbf60a44, 0xe8c413a4, 0xe8475470, 0x987d025c,
  0x9d4fd14e, 0xdc5866fb, 0x3fead523, 0x6e0b5c09,
  0xf6b9b30b, 0xfd6dabc6, 0xba3eb757, 0xd1dd1308
};

