#include <pebble.h>
#include "place_layer.h"
#include "place_description.h"
#include "settings.h"

void place_layer_update_time(place_layer *place, time_t *time){
  struct tm *tick_time = gmtime(time);
  strftime(place->watch_str, sizeof(place->watch_str), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Time in %s is updated to %s", place->place->place_name, place->watch_str); 
  layer_mark_dirty(text_layer_get_layer(place->place_time_layer));
}


void destroy_place_layer(place_layer *place){
  text_layer_destroy(place->place_time_layer);
  text_layer_destroy(place->place_name_layer);
  layer_destroy(place->place_layer);
}

struct PlaceLayerData{
  place_layer* place_l;
};


void draw_place_bubble(struct Layer *layer, GContext *ctx){
  struct PlaceLayerData* data = (struct PlaceLayerData*) layer_get_data(layer);
  place_layer *place = data->place_l;
  GRect bounds = layer_get_bounds(layer);
  //background
  graphics_context_set_antialiased(ctx, false);
  
  Settings *settings = place->settings;
  graphics_context_set_stroke_color(ctx, settings->BackgroundColor);
  graphics_context_set_fill_color(ctx, settings->BackgroundColor);
  graphics_fill_rect(ctx, bounds, place->radius, GCornersAll);
  
  //frame
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, settings->ForegroundColor);
  //graphics_context_set_stroke_color(ctx, GColorRed);
  graphics_draw_round_rect(ctx, bounds, place->radius);
}

void render_place_name(place_layer *place, bool show_offset){
  strcpy(place->place_str, place->place->place_name);
  if (show_offset){
    if (strlen(place->place_str) > 0){
      strcat(place->place_str, ", ");    
    }
    char offset_str[20];
    time_t now_t = time(NULL);    
    struct tm* l_time_tm = localtime(&now_t);
    int local_offset = l_time_tm->tm_gmtoff;
    int summer_time_offset = l_time_tm->tm_isdst*3600;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "local offset: %d", local_offset); 
    int rel_offset = place->place->offset - local_offset - summer_time_offset;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Relative offset for %s: %d - (%d) - %d = %d", place->place->place_name, 
            place->place->offset, local_offset, summer_time_offset, rel_offset);
    int offset_hours = rel_offset / 3600;
    int offset_min = rel_offset % 3600 / 60;
    //some weird time zone
    if (offset_min != 0){
      int abs_offset = rel_offset;
      if (abs_offset < 0){
         abs_offset = -abs_offset;
      }
      time_t offset_t = (time_t) abs_offset;
      tm* time_offset = gmtime(&offset_t);
      strftime(offset_str, 20,  (rel_offset >= 0) ?
                                          "+%H:%M" : "-%H:%M", time_offset);
      strcat(place->place_str, offset_str);
      return;
    }
      
      
    if (offset_hours >= 0){
      snprintf(offset_str, 20, "+%d", offset_hours);      
    }else{
      snprintf(offset_str, 20, "%d", offset_hours);      
    }
    strcat(place->place_str, offset_str);
    if (offset_hours == 1 || offset_hours == -1){
      strcat(place->place_str, "HR");
    }else{
      strcat(place->place_str, "HRS");
    }
  }
}

void update_place_layer(place_layer *place){
  //snprintf(place->watch_str, sizeof(place->watch_str), "%d", (int)place->place->offset);
  //text_layer_set_text(place->place_time_layer, place->watch_str);
  //text_layer_set_text(place->place_name_layer, place->place->place_name);
  render_place_name(place, true);
}

void create_place_layer_default(place_layer *place, 
                                struct place_descrition *description, 
                                int16_t top, 
                                Settings *settings,
                                Layer *parent){
  GRect bounds = layer_get_bounds(parent);
  place->place_layer = layer_create_with_data(GRect(0, top, bounds.size.w, 48), sizeof(struct PlaceLayerData));
  struct PlaceLayerData* data = (struct PlaceLayerData*) layer_get_data(place->place_layer);
  data->place_l = place;
  place->place = description;
  place->settings = settings;
  place->radius = SCREEN_RADIUS;
  //place->place_layer = layer_create(GRect(0, top, 96, 48));
  layer_set_update_proc(place->place_layer, draw_place_bubble);
  layer_add_child(parent, place->place_layer);
  
  //place->color = settings.ForegroundColor;
  
  //   strncpy(place->watch_str, "00:00", 6);
  //   strncpy(place->place->place_name, "Test", 5);
  
  bounds = layer_get_bounds(place->place_layer);
  
  place->place_name_layer = text_layer_create(GRect(0, 0, bounds.size.w, 16));
  text_layer_set_text_color(place->place_name_layer, place->settings->TextColor);
  text_layer_set_background_color(place->place_name_layer, GColorClear);
  //text_layer_set_background_color(place->place_name_layer, GColorBlue);
  text_layer_set_font(place->place_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(place->place_name_layer, GTextAlignmentCenter);
  
  text_layer_set_text(place->place_name_layer, place->place_str);
  layer_add_child(place->place_layer, text_layer_get_layer(place->place_name_layer));

  //updating the layer name
  render_place_name(place, true);
  
  place->place_time_layer = text_layer_create(GRect(0, 12, bounds.size.w, 34));
  text_layer_set_text_color(place->place_time_layer, place->settings->TextColor);
  text_layer_set_background_color(place->place_time_layer, GColorClear);
  //text_layer_set_background_color(place->place_time_layer, GColorGreen);
  text_layer_set_font(place->place_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(place->place_time_layer, GTextAlignmentCenter);
  text_layer_set_text(place->place_time_layer, place->watch_str);
  layer_add_child(place->place_layer, text_layer_get_layer(place->place_time_layer));
  
  layer_mark_dirty(parent);
  layer_mark_dirty(place->place_layer);
  layer_mark_dirty(text_layer_get_layer(place->place_time_layer));
  layer_mark_dirty(text_layer_get_layer(place->place_name_layer));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Watch string: %s", text_layer_get_text(place->place_time_layer)); 
  //return place->place_layer;
}


// static void switch_panels_if_required(){
//   GRect rect1 = layer_get_frame(place1.place_layer);
//   GRect rect2 = layer_get_frame(place2.place_layer);
//   APP_LOG(APP_LOG_LEVEL_DEBUG, "rect1y: %d, rect2y: %d, y1: %d, y2 %d", 
//           (int)rect1.origin.y, (int)rect2.origin.y, (int)place1.place->y,(int)place2.place->y); 
  
//   if ((place1.place->y > place2.place->y) != (rect1.origin.y > rect2.origin.y)){
//     layer_set_frame(place1.place_layer, rect2);
//     layer_set_frame(place2.place_layer, rect1);
//     layer_mark_dirty(window_get_root_layer(s_window));
//   }
//   if (place1.place->y > place2.place->y){
//     bottom_place_layer = place1.place_layer;
//   }else{
//     bottom_place_layer = place2.place_layer;
//   }
// }