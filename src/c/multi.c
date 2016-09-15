#include <pebble.h>


#define REDRAW_INTERVAL 15
const uint16_t WIDTH = 144;
const uint16_t HEIGHT = 72;
const uint16_t radius = 8;

static int redraw_counter = 500;

static Window *s_window;	
static Layer *map_layer;
static Layer *pointer_layer;
static GBitmap *three_worlds;
static GBitmap *image;
static BitmapLayer *tmp_layer;

  



struct place_descrition{
  Layer *place_layer;
  TextLayer *place_name_layer;
  TextLayer *place_time_layer;
  int32_t offset, x, y;
  char place_name[80];
  char watch_str[8];
  //GColor color;
};



typedef struct place_descrition place_descr;

static place_descr place1, place2, current;

	
//layers

// Write message to buffer & send
static void send_message(void){
	DictionaryIterator *iter;
	
	app_message_outbox_begin(&iter);
	
	dict_write_end(iter);
  app_message_outbox_send();
}

//piece of code from:
//davidfg4/pebble-day-night
static void draw_earth() {
  // ##### calculate the time
#ifdef PBL_SDK_2
  int now = (int)time(NULL) + time_offset;
#else
  int now = (int)time(NULL);
#endif
  float day_of_year; // value from 0 to 1 of progress through a year
  float time_of_day; // value from 0 to 1 of progress through a day
  // approx number of leap years since epoch
  // = now / SECONDS_IN_YEAR * .24; (.24 = average rate of leap years)
  int leap_years = (int)((float)now / 131487192.0);
  // day_of_year is an estimate, but should be correct to within one day
  day_of_year = now - (((int)((float)now / 31556926.0) * 365 + leap_years) * 86400);
  day_of_year = day_of_year / 86400.0;
  time_of_day = day_of_year - (int)day_of_year;
  day_of_year = day_of_year / 365.0;
  // ##### calculate the position of the sun
  // left to right of world goes from 0 to 65536
  int sun_x = (int)((float)TRIG_MAX_ANGLE * (1.0 - time_of_day));
  // bottom to top of world goes from -32768 to 32768
  // 0.2164 is march 20, the 79th day of the year, the march equinox
  // Earth's inclination is 23.4 degrees, so sun should vary 23.4/90=.26 up and down
  int sun_y = -sin_lookup((day_of_year - 0.2164) * TRIG_MAX_ANGLE) * .26 * .25;
  // ##### draw the bitmap
  int x, y;
  for(x = 0; x < WIDTH; x++) {
    int x_angle = (int)((float)TRIG_MAX_ANGLE * (float)x / (float)(WIDTH));
    for(y = 0; y < HEIGHT; y++) {
      int y_angle = (int)((float)TRIG_MAX_ANGLE * (float)y / (float)(HEIGHT * 2)) - TRIG_MAX_ANGLE/4;
      // spherical law of cosines
      float angle = ((float)sin_lookup(sun_y)/(float)TRIG_MAX_RATIO) * ((float)sin_lookup(y_angle)/(float)TRIG_MAX_RATIO);
      angle = angle + ((float)cos_lookup(sun_y)/(float)TRIG_MAX_RATIO) * ((float)cos_lookup(y_angle)/(float)TRIG_MAX_RATIO) * ((float)cos_lookup(sun_x - x_angle)/(float)TRIG_MAX_RATIO);
#ifdef PBL_BW
      int byte = y * gbitmap_get_bytes_per_row(image) + (int)(x / 8);
      if ((angle < 0) ^ (0x1 & (((char *)gbitmap_get_data(world_bitmap))[byte] >> (x % 8)))) {
        // white pixel
        ((char *)gbitmap_get_data(image))[byte] = ((char *)gbitmap_get_data(image))[byte] | (0x1 << (x % 8));
      } else {
        // black pixel
        ((char *)gbitmap_get_data(image))[byte] = ((char *)gbitmap_get_data(image))[byte] & ~(0x1 << (x % 8));
      }
#else
      int byte = y * gbitmap_get_bytes_per_row(three_worlds) + x;
      if (angle < 0) { // dark pixel
        ((char *)gbitmap_get_data(three_worlds))[byte] = ((char *)gbitmap_get_data(three_worlds))[WIDTH*HEIGHT + byte];
      } else { // light pixel
        ((char *)gbitmap_get_data(three_worlds))[byte] = ((char *)gbitmap_get_data(three_worlds))[WIDTH*HEIGHT*2 + byte];
      }
#endif
    }
  }
  layer_mark_dirty(map_layer);
}

static void draw_map(struct Layer *layer, GContext *ctx) {
  graphics_draw_bitmap_in_rect(ctx, image, gbitmap_get_bounds(image));
}
//end of borrowed code

static void switch_panels_if_required(){
  GRect rect1 = layer_get_frame(place1.place_layer);
  GRect rect2 = layer_get_frame(place2.place_layer);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "rect1y: %d, rect2y: %d, y1: %d, y2 %d", 
          (int)rect1.origin.y, (int)rect2.origin.y, (int)place1.y,(int)place2.y); 
  
  if ((place1.y > place2.y) != (rect1.origin.y > rect2.origin.y)){
    layer_set_frame(place1.place_layer, rect2);
    layer_set_frame(place2.place_layer, rect1);
    layer_mark_dirty(window_get_root_layer(s_window));
  }
}

static void update_place(place_descr *place, Tuple *city_t, Tuple *offset_t, Tuple* x_t, Tuple* y_t){
  if (!(city_t && offset_t && x_t && y_t))
    return;
  //TODO update only on suibstansial cahnges; make ifs
  place->x = x_t->value->int32;
  place->y = y_t->value->int32;
  place->offset = offset_t->value->int32;
  strncpy(place->place_name, city_t->value->cstring, sizeof(place->place_name));

  snprintf(place->watch_str, sizeof(place->watch_str), "%d", (int)offset_t->value->int32);
  text_layer_set_text(place->place_time_layer, place->watch_str);
  text_layer_set_text(place->place_name_layer, place->place_name);
}





// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
}



static void draw_place_bubble(struct Layer *layer, GContext *ctx){
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, GColorOrange);
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

static void draw_arrows(struct Layer *layer, GContext *ctx){
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, GColorOrange);
  
  //set up coords
  GPoint p1_m = get_point_on_map(place1.x, place1.y, bounds.size);
  GPoint p2_m = get_point_on_map(place2.x, place2.y, bounds.size);

  int16_t y1_con = 0;
  int16_t y2_con = bounds.size.h;
  if (place1.y>place2.y){
    y2_con = 0;
    y1_con = bounds.size.h;
  }
  
  int16_t x1_con = get_x_within_bounds(p1_m.x, layer_get_frame(place1.place_layer));
  int16_t x2_con = get_x_within_bounds(p2_m.x, layer_get_frame(place2.place_layer));

  //draw lines 1
  graphics_draw_lines(ctx, p1_m, GPoint(x1_con, y1_con), radius);
  //draw lines 2
  graphics_draw_lines(ctx, p2_m, GPoint(x2_con, y2_con), radius);
  
}

static void create_place_layer_default(place_descr *place, int16_t top, Layer *parent){
  GRect bounds = layer_get_bounds(parent);
  place->place_layer = layer_create(GRect(0, top, bounds.size.w, 48));
  //place->place_layer = layer_create(GRect(0, top, 96, 48));
  layer_set_update_proc(place->place_layer, draw_place_bubble);
  layer_add_child(parent, place->place_layer);
  
  //place->color = GColorOrange;
  
  strncpy(place->watch_str, "00:00", 6);
  strncpy(place->place_name, "Test", 5);
  
  bounds = layer_get_bounds(place->place_layer);
 
  place->place_name_layer = text_layer_create(GRect(0, 0, bounds.size.w, 16));
  text_layer_set_text_color(place->place_name_layer, GColorWhite);
  text_layer_set_background_color(place->place_name_layer, GColorClear);
  //text_layer_set_background_color(place->place_name_layer, GColorBlue);
  text_layer_set_font(place->place_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(place->place_name_layer, GTextAlignmentCenter);

  text_layer_set_text(place->place_name_layer, place->place_name);
  layer_add_child(place->place_layer, text_layer_get_layer(place->place_name_layer));

  place->place_time_layer = text_layer_create(GRect(0, 12, bounds.size.w, 34));
  text_layer_set_text_color(place->place_time_layer, GColorWhite);
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
  //return place.place_layer;
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
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, get_offset_rect(bounds.size.h, bounds.size.w-radius*2, 0), 
                     radius, GCornersLeft);
  for (int16_t i = 0; i<=(bounds.size.h-1)/2; ++i){
      graphics_draw_round_rect(ctx, get_offset_rect_right(bounds.size.h, bounds.size.h+i, 1, bounds.size.w), radius-1);    
  }
    
  //draw counters
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, GColorOrange);
  graphics_draw_round_rect(ctx, bounds, radius);
  graphics_draw_round_rect(ctx, get_offset_rect_right(bounds.size.h, bounds.size.h, 2, bounds.size.w), radius-2);
}

static void draw_floating_layer_to_right(struct Layer *layer, GContext *ctx){
  GRect bounds = layer_get_bounds(layer);

  //fill insides
  graphics_context_set_antialiased(ctx, false);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, get_offset_rect_right(bounds.size.h, bounds.size.w-radius*2, 0, bounds.size.w), 
                     radius, GCornersRight);
  for (int16_t i = 0; i<=(bounds.size.h-1)/2; ++i){
      graphics_draw_round_rect(ctx, get_offset_rect(bounds.size.h, bounds.size.h+i, 1), radius-1);    
  }
    
  //draw counters
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, GColorOrange);
  graphics_draw_round_rect(ctx, bounds, radius);
  graphics_draw_round_rect(ctx, get_offset_rect(bounds.size.h, bounds.size.h, 2), radius-2);
}

static void update_floating_place(place_descr *place, Tuple *city_t, Tuple *offset_t, Tuple* x_t, Tuple* y_t){
  if (!(x_t && y_t))
    return;
  //TODO update only on suibstansial cahnges; make ifs
  place->x = x_t->value->int32;
  place->y = y_t->value->int32;
  place->offset = offset_t->value->int32;
  
  //compare to place 1 and place 2
  if ((abs(place->x-place1.x)<radius/2 && abs(place->x-place1.x)<radius/2) ||
    (abs(place->x-place2.x)<radius/2 && abs(place->x-place2.x)<radius/2)){
    //too close
    layer_set_hidden(place->place_layer, true);
    return;
  }

  layer_set_hidden(place->place_layer, false);
  text_layer_set_text(place->place_time_layer, place->watch_str);
  
  GSize my_size = layer_get_bounds(place->place_layer).size;
  int16_t new_top = place->y - my_size.h/2;
  if (new_top<1){
    new_top = 1;
  }
  if (new_top>HEIGHT-1-my_size.h){
    new_top = HEIGHT-1-my_size.h;
  }

  //deduce orientation:
  if (place->x < WIDTH - my_size.w){
    //normal oriention to the right
    int16_t new_left = place->x - my_size.h/2;
    if (new_left<1){
      new_left = 1;
    }
    
    layer_set_frame(place->place_layer, GRect(new_left, new_top, my_size.w, my_size.h));
    layer_set_update_proc(place->place_layer, draw_floating_layer_to_right);
    layer_set_frame(text_layer_get_layer(place->place_time_layer), GRect(radius*2, -1, my_size.w, my_size.h));
    text_layer_set_text_alignment(place->place_time_layer, GTextAlignmentLeft);
  }else{
    //reverse oriention to the left
    int16_t new_left = place->x - (my_size.w - my_size.h/2);
    if (new_left>WIDTH-1-my_size.w){
      new_left = WIDTH-1-my_size.w;
    }
    
    layer_set_frame(place->place_layer, GRect(new_left, new_top, my_size.w, my_size.h));
    layer_set_update_proc(place->place_layer, draw_floating_layer_to_left);
    layer_set_frame(text_layer_get_layer(place->place_time_layer), GRect(0, -1, my_size.w-radius*2, my_size.h));
    text_layer_set_text_alignment(place->place_time_layer, GTextAlignmentRight);
  }
    
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
  Tuple *reason_t = dict_find(received, MESSAGE_KEY_UpdateReason);
	
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
      update_place(&place1, city_t, offset_t, x_t, y_t);

      //updated place 2
      city_t = dict_find(received, MESSAGE_KEY_Place2);
      offset_t = dict_find(received, MESSAGE_KEY_ZoneOffset2);
      x_t = dict_find(received, MESSAGE_KEY_P2X);
      y_t = dict_find(received, MESSAGE_KEY_P2Y);
      update_place(&place2, city_t, offset_t, x_t, y_t);

      switch_panels_if_required();
      
      //updated place Curent
//       city_t = dict_find(received, MESSAGE_KEY_CurPlace);
//       offset_t = dict_find(received, MESSAGE_KEY_ZoneOffsetCur);
//       x_t = dict_find(received, MESSAGE_KEY_P_CUR_X);
//       y_t = dict_find(received, MESSAGE_KEY_P_CUR_Y);
//       update_floating_place(&current, city_t, offset_t, x_t, y_t);

    }
  }
  
  //send_message();
}

static void create_place_layer_floating(place_descr *place, Layer* parent){
  GRect bounds = layer_get_bounds(parent);
  place->place_layer = layer_create(GRect(0, 0, 52, radius*2));
  //place->place_layer = layer_create(GRect(0, top, 96, 48));
  layer_set_update_proc(place->place_layer, draw_floating_layer_to_left);
  //layer_set_hidden(place->place_layer, true);
  layer_add_child(parent, place->place_layer);
  
  //place->color = GColorOrange;
  
  strncpy(place->watch_str, "00:00", 6);
  strncpy(place->place_name, "Test", 5);
  
  bounds = layer_get_bounds(place->place_layer);
 
  //The layer below is not in use
  place->place_name_layer = text_layer_create(GRect(radius*2+1, -2+1, bounds.size.w-radius*2, bounds.size.h));
  //text_layer_set_text_color(place->place_name_layer, GColorWhite);
  //text_layer_set_background_color(place->place_name_layer, GColorClear);
  //text_layer_set_background_color(place->place_name_layer, GColorBlue);
  //text_layer_set_font(place->place_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  //text_layer_set_text_alignment(place->place_name_layer, GTextAlignmentLeft);
  //text_layer_set_text(place->place_name_layer, place->watch_str);
  //layer_add_child(place->place_layer, text_layer_get_layer(place->place_name_layer));

  place->place_time_layer = text_layer_create(GRect(radius*2, -1, bounds.size.w-radius*2, bounds.size.h));
  text_layer_set_text_color(place->place_time_layer, GColorWhite);
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

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  //TODO make ALL constants variable

  GRect map_bounds = GRect(0, 48, WIDTH, HEIGHT);
  map_layer = layer_create(map_bounds);
  layer_set_update_proc(map_layer, draw_map);

  layer_add_child(window_layer, map_layer);
  //draw_earth();
  
  tmp_layer = bitmap_layer_create(GRect(0,0, map_bounds.size.w, map_bounds.size.h));
  // Load the image
  three_worlds = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_3_WORLDS);
  bitmap_layer_set_bitmap(tmp_layer, three_worlds);
  //layer_add_child(map_layer, bitmap_layer_get_layer(tmp_layer));
  
  pointer_layer = layer_create(GRect(0,0, map_bounds.size.w, map_bounds.size.h));
  layer_set_update_proc(pointer_layer, draw_arrows);
  layer_add_child(map_layer, pointer_layer);  
  
  create_place_layer_default(&place1, 0, window_layer);
  create_place_layer_default(&place2, 120, window_layer);
  
  create_place_layer_floating(&current, map_layer);
  layer_set_hidden(current.place_layer, true);
  
  image = gbitmap_create_as_sub_bitmap(three_worlds, GRect(0, 0, WIDTH, HEIGHT));

}

static void window_unload(Window *window) {
  destroy_place_layer(&place1);
  destroy_place_layer(&place2);
  destroy_place_layer(&current);
  bitmap_layer_destroy(tmp_layer);
  
  
  layer_destroy(pointer_layer);
  layer_destroy(map_layer);
  
  gbitmap_destroy(three_worlds);
  gbitmap_destroy(image);
}

static void update_time(place_descr *place, time_t *time){
  struct tm *tick_time = gmtime(time);
  strftime(place->watch_str, sizeof(place->watch_str), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Time in %s is updated to %s", place->place_name, place->watch_str); 
  layer_mark_dirty(text_layer_get_layer(place->place_time_layer));
  

}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed){
  time_t now = time(NULL);
  time_t time1 = now + place1.offset;
  update_time(&place1, &time1);
  
  time_t time2 = now + place2.offset;
  update_time(&place2, &time2);
 
  strftime(current.watch_str, sizeof(current.watch_str), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  
  redraw_counter++;
  if (redraw_counter >= REDRAW_INTERVAL) {
    draw_earth();
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
  window_stack_push(s_window, true);
	
	
	// Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler); 
	app_message_register_inbox_dropped(in_dropped_handler); 
	app_message_register_outbox_failed(out_failed_handler);

  // Initialize AppMessage inbox and outbox buffers with a suitable size
  const int inbox_size = 512;
  const int outbox_size = 512;
	app_message_open(inbox_size, outbox_size);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

static void deinit(void) {
	app_message_deregister_callbacks();
  tick_timer_service_unsubscribe();
	window_destroy(s_window);
}

int main( void ) {
	init();
	app_event_loop();
	deinit();
}