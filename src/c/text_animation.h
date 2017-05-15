#ifdef USE_TEXT_ANIMATION
#pragma once
#include <pebble.h>

struct TextAnimationContext{
  char full_text[80];
  char cur_text[80];
  char *old_text;
  char *new_text;
  uint16_t display_text_len;
  TextLayer *text_layer;
};



Animation* text_animation_create(struct TextAnimationContext *context);

#endif