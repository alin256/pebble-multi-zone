#ifdef PLACE_CURRENT_LAYER_ENABLED
#include <pebble.h>
#include "place_layer_current.h"

const int16_t offset_mid = 2;

static GRect get_offset_rect(int16_t height, int16_t width, int16_t offset){
  GRect bounds = GRect(offset, offset, width - offset*2, height - offset*2);
  return bounds;  
}

static GRect get_offset_rect_right(int16_t height, int16_t width, int16_t offset, int16_t total_width){
  GRect bounds = GRect(offset+total_width-width, offset, width - offset*2, height - offset*2);
  return bounds;  
}


static void draw_floating_layer_to_left(struct Layer *layer, GContext *ctx){
  GRect bounds = layer_get_bounds(layer);

  //fill insides
  graphics_context_set_antialiased(ctx, false);
  graphics_context_set_stroke_color(ctx, settings.BackgroundColor);
  graphics_context_set_fill_color(ctx, settings.BackgroundColor);
  graphics_fill_rect(ctx, get_offset_rect(bounds.size.h, bounds.size.w-radius*2, 0), 
                     radius, GCornersLeft);
  for (int16_t i = 0; i<=(bounds.size.h-1)/2; ++i){
      graphics_draw_round_rect(ctx, get_offset_rect_right(bounds.size.h, bounds.size.h+i, 1, bounds.size.w), radius-1);    
  }
    
  //draw counters
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, settings.ForegroundColor);
  graphics_draw_round_rect(ctx, bounds, radius);
  graphics_draw_round_rect(ctx, get_offset_rect_right(bounds.size.h, bounds.size.h, 2, bounds.size.w), radius-2);
  graphics_draw_round_rect(ctx, get_offset_rect_right(bounds.size.h, bounds.size.h, radius-1, bounds.size.w), 1);
}

static void draw_floating_layer_to_right(struct Layer *layer, GContext *ctx){
  GRect bounds = layer_get_bounds(layer);

  //fill insides
  graphics_context_set_antialiased(ctx, false);
  graphics_context_set_stroke_color(ctx, settings.BackgroundColor);
  graphics_context_set_fill_color(ctx, settings.BackgroundColor);
  graphics_fill_rect(ctx, get_offset_rect_right(bounds.size.h, bounds.size.w-radius*2, 0, bounds.size.w), 
                     radius, GCornersRight);
  for (int16_t i = 0; i<=(bounds.size.h-1)/2; ++i){
      graphics_draw_round_rect(ctx, get_offset_rect(bounds.size.h, bounds.size.h+i, 1), radius-1);    
  }
    
  //draw contours
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, settings.ForegroundColor);
  graphics_draw_round_rect(ctx, bounds, radius);
  graphics_draw_round_rect(ctx, get_offset_rect(bounds.size.h, bounds.size.h, 2), radius-2);
  graphics_draw_round_rect(ctx, get_offset_rect(bounds.size.h, bounds.size.h, radius-1), 1);
}

static void draw_floating_layer_undefined(struct Layer *layer, GContext *ctx){
  GRect bounds = layer_get_bounds(layer);

  //fill insides
  graphics_context_set_antialiased(ctx, false);
  graphics_context_set_stroke_color(ctx, settings.BackgroundColor);
  graphics_context_set_fill_color(ctx, settings.BackgroundColor);
  graphics_fill_rect(ctx, get_offset_rect(bounds.size.h, bounds.size.w, 0), 
                     radius, GCornersAll);
  //draw contours
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, settings.ForegroundColor);
  graphics_draw_round_rect(ctx, bounds, radius);
  graphics_context_set_text_color(ctx, settings.ForegroundColor);
  graphics_draw_text(ctx, &question[0], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), get_offset_rect(bounds.size.h, bounds.size.h, -1), GTextOverflowModeWordWrap, GTextAlignmentCenter, 0);
  //graphics_draw_round_rect(ctx, get_offset_rect(bounds.size.h, bounds.size.h, 2), radius-2);
  //graphics_draw_round_rect(ctx, get_offset_rect(bounds.size.h, bounds.size.h, radius-1), 1);
}


static void update_floating_place(place_descr *place){
  GSize my_size = layer_get_bounds(place->place_layer).size;
  GPoint p_m;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Elapsed since update %d, max %d", (int) (time(NULL) - settings.last_update), 
          (int) OUTDATE_TIME); 
  if (time(NULL) - settings.last_update > OUTDATE_TIME){
    p_m.x = WIDTH/2 + my_size.h;
    p_m.y = HEIGHT - my_size.h/2;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Forced floating location %d, %d", (int) p_m.x, (int) p_m.y); 
    request_locaion();
  }
  else
  {
    //convert coords
    p_m = get_point_on_map(place->place->x, place->place->y, GSize(WIDTH, HEIGHT));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Computed floating location %d, %d", (int) p_m.x, (int) p_m.y); 
  }

  
  //place->offset = offset_t->value->int32;
  
  //compare to place 1 and place 2
  if ((abs(place->place->x-place1.place->x)<radius/2 && abs(place->place->x-place1.place->x)<radius/2) ||
    (abs(place->place->x-place2.place->x)<radius/2 && abs(place->place->x-place2.place->x)<radius/2)){
    //too close
    layer_set_hidden(place->place_layer, true);
    return;
  }

  layer_set_hidden(place->place_layer, false);
  text_layer_set_text(place->place_time_layer, place->watch_str);
  
  
  int16_t new_top = p_m.y - my_size.h/2;
  if (new_top<1){
    new_top = 1;
  }
  if (new_top>HEIGHT-1-my_size.h){
    new_top = HEIGHT-1-my_size.h;
  }
  int16_t left_tmp = 0;
    
  //deduce orientation:
  if (p_m.x < WIDTH - my_size.w){
    //normal oriention to the right
    int16_t new_left = p_m.x - my_size.h/2;
    if (new_left<1){
      new_left = 1;
    }
    left_tmp = new_left;
    
    layer_set_frame(place->place_layer, GRect(new_left, new_top, my_size.w, my_size.h));
    layer_set_update_proc(place->place_layer, draw_floating_layer_to_right);
    layer_set_frame(text_layer_get_layer(place->place_time_layer), GRect(radius*2, -1, my_size.w, my_size.h));
    text_layer_set_text_alignment(place->place_time_layer, GTextAlignmentLeft);
  }else{
    //reverse oriention to the left
    int16_t new_left = p_m.x - (my_size.w - my_size.h/2);
    if (new_left>WIDTH-1-my_size.w){
      new_left = WIDTH-1-my_size.w;
    }
    left_tmp = new_left;
    
    layer_set_frame(place->place_layer, GRect(new_left, new_top, my_size.w, my_size.h));
    layer_set_update_proc(place->place_layer, draw_floating_layer_to_left);
    layer_set_frame(text_layer_get_layer(place->place_time_layer), GRect(0, -1, my_size.w-radius*2, my_size.h));
    text_layer_set_text_alignment(place->place_time_layer, GTextAlignmentRight);
  }
  
  
  if (time(NULL) - settings.last_update > OUTDATE_TIME){
    layer_set_update_proc(place->place_layer, draw_floating_layer_undefined);
  }
    
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Left: %d; Top: %d", (int) left_tmp, (int) new_top); 
    
}

static void create_place_layer_floating(place_descr *place, struct place_descrition *place_d, Layer* parent){
  GRect bounds = layer_get_bounds(parent);
  place->place_layer = layer_create(GRect(0, 0, 52, radius*2));
  place->place = place_d;
  //place->place_layer = layer_create(GRect(0, top, 96, 48));
  layer_set_update_proc(place->place_layer, draw_floating_layer_to_left);
  //layer_set_hidden(place->place_layer, true);
  layer_add_child(parent, place->place_layer);
  
  //place->color = settings.ForegroundColor;
  
  //strncpy(place->watch_str, "00:00", 6);
  //strncpy(place->place->place_name, "Test", 5);
  
  bounds = layer_get_bounds(place->place_layer);
 
  //The layer below is not in use
  place->place_name_layer = text_layer_create(GRect(radius*2+1, -2+1, bounds.size.w-radius*2, bounds.size.h));
  //text_layer_set_text_color(place->place_name_layer, settings.TextColor);
  //text_layer_set_background_color(place->place_name_layer, GColorClear);
  //text_layer_set_background_color(place->place_name_layer, GColorBlue);
  //text_layer_set_font(place->place_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  //text_layer_set_text_alignment(place->place_name_layer, GTextAlignmentLeft);
  //text_layer_set_text(place->place_name_layer, place->watch_str);
  //layer_add_child(place->place_layer, text_layer_get_layer(place->place_name_layer));

  place->place_time_layer = text_layer_create(GRect(radius*2, -1, bounds.size.w-radius*2, bounds.size.h));
  text_layer_set_text_color(place->place_time_layer, settings.TextColor);
  text_layer_set_background_color(place->place_time_layer, GColorClear);
  //text_layer_set_background_color(place->place_time_layer, GColorGreen);
  text_layer_set_font(place->place_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(place->place_time_layer, GTextAlignmentLeft);
  text_layer_set_text(place->place_time_layer, place->watch_str);
  layer_add_child(place->place_layer, text_layer_get_layer(place->place_time_layer));
  
  layer_mark_dirty(place->place_layer);

}
#endif

