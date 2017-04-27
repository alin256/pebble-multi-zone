// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// Copyleft: Sergey (aliner) Alyaev, 2017

#include <pebble.h>
#include "map_layer.h"


#define REDRAW_INTERVAL 15

const uint16_t radius = 8;

static int redraw_counter = 500;

static Window *s_window;  


//map things
static BitmapLayer *map_layer;
static GBitmap *three_worlds;
static GBitmap *image;
static BitmapLayer *tmp_layer;

//arrows thing
static Layer *pointer_layer;




struct place_descrition{
  int32_t offset;
  int32_t x, y;
  char place_name[80];
};

struct place_visualization{
  Layer *place_layer;
  TextLayer *place_name_layer;
  TextLayer *place_time_layer;
  char watch_str[8];
  char place_str[100];
  struct place_descrition *place;
  //GColor color;  
};

static char question[1] = "?";

// Persistent storage key
#define SETTINGS_KEY 1
#define SETTINGS_VERSION_KEY 2

static const int32_t current_settings_version = 5;

// Define our settings struct
typedef struct ClaySettings {
  GColor BackgroundColor;
  GColor ForegroundColor;
  GColor TextColor;
  struct place_descrition place1;
  struct place_descrition place2;
  struct place_descrition place_cur;
  int8_t show_local_time;
  time_t last_update;
} ClaySettings;

//static bool position_known = false;
const time_t OUTDATE_TIME = 1200;

static char tmp_time_zone[TIMEZONE_NAME_LENGTH];

// An instance of the struct
static ClaySettings settings;

// Save the settings to persistent storage
static void prv_save_settings() {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
  persist_write_int(SETTINGS_VERSION_KEY, current_settings_version);
}





typedef struct place_visualization place_descr;

static place_descr place1, place2, current;

//this is a reference to botom place
static Layer *bottom_place_layer;
static bool condensing = true;


// Write message to buffer & send
static void send_position_request(void){
  if (settings.show_local_time){
    DictionaryIterator *iter;
  
    app_message_outbox_begin(&iter);
    dict_write_int16(iter, MESSAGE_KEY_Request, 1);
    dict_write_end(iter);
    app_message_outbox_send();
  }
}


static void switch_panels_if_required(){
  GRect rect1 = layer_get_frame(place1.place_layer);
  GRect rect2 = layer_get_frame(place2.place_layer);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "rect1y: %d, rect2y: %d, y1: %d, y2 %d", 
          (int)rect1.origin.y, (int)rect2.origin.y, (int)place1.place->y,(int)place2.place->y); 
  
  if ((place1.place->y > place2.place->y) != (rect1.origin.y > rect2.origin.y)){
    layer_set_frame(place1.place_layer, rect2);
    layer_set_frame(place2.place_layer, rect1);
    layer_mark_dirty(window_get_root_layer(s_window));
  }
  if (place1.place->y > place2.place->y){
    bottom_place_layer = place1.place_layer;
  }else{
    bottom_place_layer = place2.place_layer;
  }
    
}

static void update_place_partial(struct place_descrition *place_d, Tuple* x_t, Tuple* y_t){
  if (!(x_t && y_t))
    return;
  //TODO update only on substansial cahnges; make ifs
  place_d->x = x_t->value->int32;
  place_d->y = y_t->value->int32;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Place %s changed to: x: %d, y: %d", place_d->place_name, place_d->x, place_d->y); 
}


static void update_place(struct place_descrition *place_d, Tuple *city_t, Tuple *offset_t, Tuple* x_t, Tuple* y_t){
  if (!(city_t && offset_t && x_t && y_t))
    return;
  //TODO update only on substansial cahnges; make ifs
  update_place_partial(place_d, x_t, y_t);
  place_d->offset = offset_t->value->int32;
  strncpy(place_d->place_name, city_t->value->cstring, sizeof(place_d->place_name));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Place %s changed to: x: %d, y: %d", place_d->place_name, place_d->x, place_d->y); 
}

static void render_place_name(place_descr *place, bool show_offset){
  strcpy(place->place_str, place->place->place_name);
  if (show_offset){
    if (strlen(place->place_str) > 0){
      strcat(place->place_str, ", ");    
    }
    char offset_str[20];
    int offset_hours = place->place->offset / 3600;
    int offset_min = place->place->offset % 3600 / 60;
    //some weird time zone
    if (offset_min != 0){
      int abs_offset = place->place->offset;
      if (abs_offset < 0){
         abs_offset = -abs_offset;
      }
      time_t offset_t = (time_t) abs_offset;
      tm* time_offset = gmtime(&offset_t);
      strftime(offset_str, 20,  (place->place->offset >= 0) ?
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

static void update_place_layer(place_descr *place){
  //snprintf(place->watch_str, sizeof(place->watch_str), "%d", (int)place->place->offset);
  //text_layer_set_text(place->place_time_layer, place->watch_str);
  //text_layer_set_text(place->place_name_layer, place->place->place_name);
  render_place_name(place, true);
}




// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {  
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
}



static void draw_place_bubble(struct Layer *layer, GContext *ctx){
  GRect bounds = layer_get_bounds(layer);
  //background
  graphics_context_set_antialiased(ctx, false);
  graphics_context_set_stroke_color(ctx, settings.BackgroundColor);
  graphics_context_set_fill_color(ctx, settings.BackgroundColor);
  graphics_fill_rect(ctx, bounds, radius, GCornersAll);
  
  //frame
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, settings.ForegroundColor);
  graphics_draw_round_rect(ctx, bounds, radius);
}

static GPoint get_point_on_map(int32_t x, int32_t y, GSize bounds){
  int16_t x_m = x*bounds.w/TRIG_MAX_ANGLE;
  int16_t y_m = y*bounds.h*2/TRIG_MAX_ANGLE; //up to pi
  return GPoint(x_m, y_m);
}

static int16_t get_x_within_bounds(int16_t old_x, GRect bounds){
  int16_t x = old_x;
  if (x<=bounds.origin.x+radius*2){
    x = bounds.origin.x+radius*2;
  }
  if (x+radius>=bounds.origin.x+bounds.size.w-radius*2){
    x = bounds.origin.x+bounds.size.w-radius*3;
  }
  return x;
}

static void graphics_draw_lines(GContext *gtx, GPoint start, GPoint end, int16_t end_x_range){
  for (int16_t i = 0 ;i<=end_x_range; i+=2){
    graphics_draw_line(gtx, start, GPoint(end.x + i, end.y));
  }
}

static void draw_number(GContext *ctx, GPoint middle, int16_t num){
  GRect box =  GRect(middle.x - radius*3/2, middle.y -radius, 
                                radius*3, radius*2);
  graphics_fill_rect(ctx, box, radius, GCornersAll);
  graphics_context_set_text_color(ctx, settings.TextColor);
  char *text = "00";
  snprintf(text, 2, "%d", (int) num);
  graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), box, 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void draw_arrows(struct Layer *layer, GContext *ctx){
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, settings.ForegroundColor);
  
  //set up coords
  GPoint p1_m = get_point_on_map(place1.place->x, place1.place->y, bounds.size);
  GPoint p2_m = get_point_on_map(place2.place->x, place2.place->y, bounds.size);

  int16_t y1_con = 0;
  int16_t y2_con = bounds.size.h;
  if (place1.place->y>place2.place->y){
    y2_con = 0;
    y1_con = bounds.size.h;
  }
  
  int16_t x1_con = get_x_within_bounds(p1_m.x, layer_get_frame(place1.place_layer));
  int16_t x2_con = get_x_within_bounds(p2_m.x, layer_get_frame(place2.place_layer));

  //draw lines 1
  graphics_draw_lines(ctx, p1_m, GPoint(x1_con, y1_con), radius);
  //draw lines 2
  graphics_draw_lines(ctx, p2_m, GPoint(x2_con, y2_con), radius);
  
  //TODO consider drawing multiple bubbles
  //draw time
  //draw_number(ctx, GPoint(layer_get_frame(current.place_layer).origin.x, HEIGHT-radius*2), 12);
  
}



static void create_place_layer_default(place_descr *place, struct place_descrition *description, int16_t top, Layer *parent){
  GRect bounds = layer_get_bounds(parent);
  place->place_layer = layer_create(GRect(0, top, bounds.size.w, 48));
  place->place = description;
  //place->place_layer = layer_create(GRect(0, top, 96, 48));
  layer_set_update_proc(place->place_layer, draw_place_bubble);
  layer_add_child(parent, place->place_layer);
  
  //place->color = settings.ForegroundColor;
  
  //   strncpy(place->watch_str, "00:00", 6);
  //   strncpy(place->place->place_name, "Test", 5);
  
  bounds = layer_get_bounds(place->place_layer);
 
  place->place_name_layer = text_layer_create(GRect(0, 0, bounds.size.w, 16));
  text_layer_set_text_color(place->place_name_layer, settings.TextColor);
  text_layer_set_background_color(place->place_name_layer, GColorClear);
  //text_layer_set_background_color(place->place_name_layer, GColorBlue);
  text_layer_set_font(place->place_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(place->place_name_layer, GTextAlignmentCenter);
  
  text_layer_set_text(place->place_name_layer, place->place_str);
  layer_add_child(place->place_layer, text_layer_get_layer(place->place_name_layer));

  //updating the layer name
  render_place_name(place, true);
  
  place->place_time_layer = text_layer_create(GRect(0, 12, bounds.size.w, 34));
  text_layer_set_text_color(place->place_time_layer, settings.TextColor);
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


static void request_locaion(){
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Location requested !"); 
  send_position_request();
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

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
  Tuple *reason_t = dict_find(received, MESSAGE_KEY_UpdateReason);
  
  Tuple *show_local_t = dict_find(received, MESSAGE_KEY_ShowLocalTime);
  if (show_local_t){
    settings.show_local_time = show_local_t->value->int16;
  }
  
  {
    Tuple *back_color_t = dict_find(received, MESSAGE_KEY_BackgroundColor);
    if (back_color_t){
      settings.BackgroundColor = GColorFromHEX(back_color_t->value->int32);
    }
    else{
      settings.BackgroundColor = GColorBlack;
    }
  }
  
  {
    Tuple *front_color_t = dict_find(received, MESSAGE_KEY_ForegroundColor);
    if (front_color_t){
      settings.ForegroundColor = GColorFromHEX(front_color_t->value->int32);
    }
    else{
      settings.ForegroundColor = GColorOrange;
    }
  }

  {
    Tuple *text_color_t = dict_find(received, MESSAGE_KEY_TextColor);
    if (text_color_t){
      settings.TextColor = GColorFromHEX(text_color_t->value->int32);
    }
    else{
      settings.TextColor = GColorWhite;
    }
    text_layer_set_text_color(place1.place_time_layer, settings.TextColor);
    text_layer_set_text_color(place1.place_name_layer, settings.TextColor);

    text_layer_set_text_color(place2.place_time_layer, settings.TextColor);
    text_layer_set_text_color(place2.place_name_layer, settings.TextColor);

    text_layer_set_text_color(current.place_time_layer, settings.TextColor);
    text_layer_set_text_color(current.place_name_layer, settings.TextColor);

  }
  
  if (reason_t){
    uint32_t reason_id = reason_t->value->uint32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Status: %d", (int) reason_id);
    {
      Tuple *city_t;
      Tuple *offset_t;
      Tuple *x_t;
      Tuple *y_t;

      //updated place 1
      city_t = dict_find(received, MESSAGE_KEY_Place1);
      offset_t = dict_find(received, MESSAGE_KEY_ZoneOffset1);
      x_t = dict_find(received, MESSAGE_KEY_P1X);
      y_t = dict_find(received, MESSAGE_KEY_P1Y);
      update_place(&settings.place1, city_t, offset_t, x_t, y_t);
      update_place_layer(&place1);

      //updated place 2
      city_t = dict_find(received, MESSAGE_KEY_Place2);
      offset_t = dict_find(received, MESSAGE_KEY_ZoneOffset2);
      x_t = dict_find(received, MESSAGE_KEY_P2X);
      y_t = dict_find(received, MESSAGE_KEY_P2Y);
      update_place(&settings.place2, city_t, offset_t, x_t, y_t);
      update_place_layer(&place2);

      switch_panels_if_required();
      
      if (reason_id == 42){//GPS
        //updated place Curent
        //city_t = dict_find(received, MESSAGE_KEY_CurPlace);
        //offset_t = dict_find(received, MESSAGE_KEY_ZoneOffsetCur);
        x_t = dict_find(received, MESSAGE_KEY_P_CUR_X);
        y_t = dict_find(received, MESSAGE_KEY_P_CUR_Y);
        //update_place(&settings.place_cur, &settings.place_cur.place_name, offset_t, x_t, y_t);
        update_place_partial(&settings.place_cur, x_t, y_t);
        update_floating_place(&current);
        settings.last_update = time(NULL);
        
      }
      else if  (reason_id == 43){
        //position_known = false;         
      }
    }
    prv_save_settings();
  }
  
  //send_message();
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

static void destroy_place_layer(place_descr *place){
  text_layer_destroy(place->place_time_layer);
  text_layer_destroy(place->place_name_layer);
  layer_destroy(place->place_layer);
}

static void prv_unobstructed_will_change(GRect final_unobstructed_screen_area,
void *context) {
  // Get the total available screen real-estate
  GRect bounds = layer_get_unobstructed_bounds(window_get_root_layer(s_window));
  if (bounds.size.h > final_unobstructed_screen_area.size.h){
    condensing = true;
  }else{
    condensing = false;
  }
  
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
  GRect map_bounds = layer_get_frame(bitmap_layer_get_layer( map_layer));
  int32_t directed_progress;
  if (condensing){
    directed_progress = progress-ANIMATION_NORMALIZED_MIN;
  }else{
    directed_progress = ANIMATION_NORMALIZED_MAX - progress;
  }
  GRect floating_bounds = layer_get_frame(current.place_layer);
  map_bounds.origin.y = bottom_bounds.size.h + 
    (- floating_bounds.origin.y + 2)
    *(directed_progress)/(ANIMATION_NORMALIZED_MAX-ANIMATION_NORMALIZED_MIN);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Animation progress: %d, directed: %d", (int) progress, (int)directed_progress);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "New y: %d", (int) map_bounds.origin.y);
  layer_set_frame(bitmap_layer_get_layer(map_layer), map_bounds);
  
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  //GRect bounds = layer_get_bounds(window_layer);
  //TODO make ALL constants variable

  if (persist_exists(SETTINGS_KEY)){
    if (persist_exists(SETTINGS_VERSION_KEY)){
      int32_t set_ver = persist_read_int(SETTINGS_VERSION_KEY);
      if (set_ver == current_settings_version){
        persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded settings successfully"); 
        //window_set_background_color(s_window, settings.BackgroundColor);
      }
    }
  }

  // Load the image
  three_worlds = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_3_WORLDS);
  image = gbitmap_create_as_sub_bitmap(three_worlds, GRect(0, 0, WIDTH, HEIGHT));
  
  GRect map_bounds = GRect(0, 48, WIDTH, HEIGHT);
  map_layer = bitmap_layer_create(map_bounds);
  bitmap_layer_set_bitmap(map_layer, image);
  //(map_layer, draw_map);

  layer_add_child(window_layer, bitmap_layer_get_layer(map_layer));
  //draw_earth();
  
  tmp_layer = bitmap_layer_create(GRect(0,0, map_bounds.size.w, map_bounds.size.h));
  bitmap_layer_set_bitmap(tmp_layer, three_worlds);
  //layer_add_child(map_layer, bitmap_layer_get_layer(tmp_layer));
  
  pointer_layer = layer_create(GRect(0,0, map_bounds.size.w, map_bounds.size.h));
  layer_set_update_proc(pointer_layer, draw_arrows);
  layer_add_child(bitmap_layer_get_layer( map_layer), pointer_layer);  
  
  create_place_layer_default(&place1, &settings.place1, 0, window_layer);
  create_place_layer_default(&place2, &settings.place2, 120, window_layer);
  bottom_place_layer = place2.place_layer;
  
  create_place_layer_floating(&current, &settings.place_cur, bitmap_layer_get_layer( map_layer));
  layer_set_hidden(current.place_layer, true);
  

  UnobstructedAreaHandlers handlers = {
    .will_change = prv_unobstructed_will_change,
    .change = prv_unobstructed_change
  };
  unobstructed_area_service_subscribe(handlers, NULL);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Going to swap pannels");
  switch_panels_if_required();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Going to update the floating pannel");
  update_floating_place(&current);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Loading complete");
}

static void window_unload(Window *window) {
  destroy_place_layer(&place1);
  destroy_place_layer(&place2);
  destroy_place_layer(&current);
  bitmap_layer_destroy(tmp_layer);
  
  
  layer_destroy(pointer_layer);
  bitmap_layer_destroy(map_layer);
  
  gbitmap_destroy(three_worlds);
  gbitmap_destroy(image);
  
  //unobstructed_area_service_unsubscribe();
  
}

static void update_time(place_descr *place, time_t *time){
  struct tm *tick_time = gmtime(time);
  strftime(place->watch_str, sizeof(place->watch_str), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Time in %s is updated to %s", place->place->place_name, place->watch_str); 
  layer_mark_dirty(text_layer_get_layer(place->place_time_layer));
  

}



static void handle_zone_change(){
  
  layer_set_hidden(current.place_layer, !settings.show_local_time);
  if (clock_is_timezone_set()){
    clock_get_timezone(tmp_time_zone, TIMEZONE_NAME_LENGTH);
    if (strcmp(tmp_time_zone, settings.place_cur.place_name) != 0){
      request_locaion();
      strcpy(settings.place_cur.place_name, tmp_time_zone);
    }
  }
}

static void handle_connection_change(bool connected){
  if (connected){
    //TODO improve logic
    request_locaion();
  }
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed){
  time_t now = time(NULL);
  time_t time1 = now + place1.place->offset;
  update_time(&place1, &time1);
  
  time_t time2 = now + place2.place->offset;
  update_time(&place2, &time2);
 
  strftime(current.watch_str, sizeof(current.watch_str), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Time in current location (%s) is updated to: %s", 
          settings.place_cur.place_name, current.watch_str); 
  handle_zone_change();
  
  redraw_counter++;
  if (redraw_counter >= REDRAW_INTERVAL) {
    draw_earth(three_worlds, bitmap_layer_get_layer(map_layer));
    redraw_counter = 0;
  }
}

static void init(void) {
  s_window = window_create();  
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Pushing window");  
  window_stack_push(s_window, true);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Window pushed");  
  
  
  // Register AppMessage handlers
  app_message_register_inbox_received(in_received_handler); 
  app_message_register_inbox_dropped(in_dropped_handler); 
  app_message_register_outbox_failed(out_failed_handler);

  // Initialize AppMessage inbox and outbox buffers with a suitable size
  const int inbox_size = 512;
  const int outbox_size = 512;
  app_message_open(inbox_size, outbox_size);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  bluetooth_connection_service_subscribe(handle_connection_change);
}

static void deinit(void) {
  prv_save_settings();
  app_message_deregister_callbacks();
  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  window_destroy(s_window);
}

int main( void ) {
  init();
  app_event_loop();
  deinit();
}