#include <pebble.h>
#include "utils.h"

int get_local_time_offset_sec_from_tm(struct tm *l_time_tm){
    int local_offset = l_time_tm->tm_gmtoff;
    int summer_time_offset = l_time_tm->tm_isdst*3600;
    int rel_offset = local_offset + summer_time_offset;
     APP_LOG(APP_LOG_LEVEL_DEBUG, "Relative offset for here: %d + %d = %d",  
             local_offset, summer_time_offset, rel_offset);
    return rel_offset;
}

int get_local_time_offset_sec(){
  if (clock_is_timezone_set()){
    time_t now_t = time(NULL);    
    struct tm* l_time_tm = localtime(&now_t);
    return get_local_time_offset_sec_from_tm(l_time_tm);
  }
  else
  {
    return 0;
  }
}


void layer_set_center(Layer* layer, GPoint p){
  GSize size = layer_get_frame(layer).size;
  int w = size.w;
  int h = size.h;
  GRect new_frame = GRect(p.x - (w+1)/2, p.y-(h+1)/2, w, h);  
  layer_set_frame(layer, new_frame);
}

// static void draw_number(GContext *ctx, GPoint middle, int16_t num){
//   GRect box =  GRect(middle.x - radius*3/2, middle.y - radius, 
//                                 radius*3, radius*2);
//   graphics_fill_rect(ctx, box, radius, GCornersAll);
//   graphics_context_set_text_color(ctx, settings.TextColor);
//   char *text = "00";
//   snprintf(text, 2, "%d", (int) num);
//   graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), box, 
//                      GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
// }

// static void handle_zone_change(){
//   //layer_set_hidden(current.place_layer, !settings.show_local_time);
//   if (clock_is_timezone_set()){
//     clock_get_timezone(tmp_time_zone, TIMEZONE_NAME_LENGTH);
//     if (strcmp(tmp_time_zone, settings.place_cur.place_name) != 0){
//       //request_locaion();
//       strcpy(settings.place_cur.place_name, tmp_time_zone);
//     }
//   }
// }
