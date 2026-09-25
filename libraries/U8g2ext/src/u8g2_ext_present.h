#pragma once
#include <U8g2lib.h>

/* Optional post-frame callback for software backends (SDL, T-Display blit).
 * Invoked when a full firstPage/nextPage cycle completes (NextPage returns 0). */
extern void (*u8g2_ext_present_cb)(void);

static inline uint8_t ext_NextPage(u8g2_t *u8g2)
{
  uint8_t r = u8g2_NextPage(u8g2);
  if (r == 0 && u8g2_ext_present_cb)
    u8g2_ext_present_cb();
  return r;
}
