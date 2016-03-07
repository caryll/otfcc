#include <stdio.h>
#include <stdlib.h>

#include "caryll-sfnt.h"
#include "caryll-font.h"

int main (void)
{
  caryll_sfnt * sfnt = caryll_sfnt_open("SIMLI.TTF");
  caryll_font * font = caryll_font_open(sfnt, 0);
  printf("head.version      = %d\n", font->head->version);
  printf("head.uintsPerEm   = %d\n", font->head->unitsPerEm);
  printf("maxp.numGlyphs    = %d\n", font->maxp->numGlyphs);
  printf("post.version      = %d\n", font->post->version);
  printf("post.isFixedPitch = %d\n", font->post->isFixedPitch);
  caryll_font_close(font);
  caryll_sfnt_close(sfnt);
  return 0;
}
