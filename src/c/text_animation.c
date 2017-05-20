#ifdef USE_TEXT_ANIMATION_2
#include <pebble.h>
#include "text_animation.h"

// struct TextAnimationContext{
//   char full_text[80];
//   uint16_t display_text_len;
//   uint16_t old_text_len;
//   uint16_t new_text_start;
//   uint16_t new_text_len;
//   TextLayer *text_layer;
//   char *final_string;
// };

#define SPACES "                                                                                                 "




Animation* text_animation_create(struct TextAnimationContext *context){
  Animation* my_anim = animation_create();
  
  //calculating lengths
  uint16_t old_len = strlen(context->old_text);
  int16_t spaces_len = context->display_text_len - old_len;
  if (spaces_len < 5){
    spaces_len = 5;
  }
  
  //copy to full_text
  char *cur = &context->full_text[0];
  strcpy(cur, context->old_text);
  char needed_spaces[spaces_len+1];
  strncpy(needed_spaces, SPACES, spaces_len);
  strcat(cur, needed_spaces);
  strcat(cur, context->new_text);
  
  //adding parameters to the animation
  AnimationCall
  animation_set_handlers(my_anim, , void *context)
  
  return my_anim;
}

#endif