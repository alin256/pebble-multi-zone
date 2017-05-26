#ifdef ENABLE_EVENT_ANIMATION

#include <pebble.h>
#include "event_animation.h"

//information about animation
static bool condensing = true;


static void prv_unobstructed_will_change(GRect final_unobstructed_screen_area,
void *context) {
  struct UnobstructedAreaContext *my_context = context;
  uint16_t height = my_context->old_height;
  uint16_t new_height = final_unobstructed_screen_area.size.h;
  
  //place
  GRect place_rect = layer_get_frame(my_context->place2);
  my_context->place_init_pos = place_rect.origin.y;
  my_context->place_new_pos = new_height-place_rect.size.h;
  
  //map
  GRect map_rect = layer_get_frame(my_context->map_layer);
  my_context->map_init_pos = map_rect.origin.y;
  my_context->map_new_pos = map_rect.origin.y+(new_height-height)/2;
  
  //finally
  my_context->old_height = new_height;
  
  
  // Get the total available screen real-estate
  //GRect bounds = layer_get_unobstructed_bounds(window_get_root_layer(s_window));
  //   if (bounds.size.h > final_unobstructed_screen_area.size.h){
  //     condensing = true;
  //   }else{
  //     condensing = false;
  //   }
  //GRect full_bounds = layer_get_bounds(window_get_root_layer(s_window));
  //   if (!grect_equal(&full_bounds, &final_unobstructed_screen_area)) {
  //     // Screen is about to become obstructed, hide the date
  //     layer_set_hidden(text_layer_get_layer(s_date_layer), true);
  //   }
}

static void prv_unobstructed_change(AnimationProgress progress, void *context) {
  // Get the total available screen real-estate
  GRect bounds = layer_get_unobstructed_bounds(window_get_root_layer(s_window));
  //Get frame of bubbles
  GRect bottom_bounds = layer_get_frame(bottom_place_layer);
  
  //move bottom layer
  bottom_bounds.origin.y = bounds.size.h - bottom_bounds.size.h;
  layer_set_frame(bottom_place_layer, bottom_bounds);
  
  //move map layer
  //GRect map_frame = map_leyer_get_frame(&map_layer_struct);
  int32_t directed_progress;
  if (condensing){
    directed_progress = progress-ANIMATION_NORMALIZED_MIN;
  }else{
    directed_progress = ANIMATION_NORMALIZED_MAX - progress;
  }

  //   GRect floating_bounds = layer_get_frame(current.place_layer);
  //   map_bounds.origin.y = bottom_bounds.size.h + 
  //     (- floating_bounds.origin.y + 2)
  //     *(directed_progress)/(ANIMATION_NORMALIZED_MAX-ANIMATION_NORMALIZED_MIN);
  //   APP_LOG(APP_LOG_LEVEL_DEBUG, "Animation progress: %d, directed: %d", (int) progress, (int)directed_progress);
  
  //   APP_LOG(APP_LOG_LEVEL_DEBUG, "New y: %d", (int) map_bounds.origin.y);
  
  //   layer_set_frame(bitmap_layer_get_layer(map_layer), map_bounds);
    
}


UnobstructedAreaHandlers get_unobstructed_handlers(){
  UnobstructedAreaHandlers handlers{
    .will_change = prv_unobstructed_will_change,
    .change = prv_unobstructed_change
  };
}
#endif