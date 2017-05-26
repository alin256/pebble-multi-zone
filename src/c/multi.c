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
#include "utils.h"
#include "place_description.h"
#include "settings.h"
#include "place_layer.h"
#include "map_layer.h"
#include "arrows_layer.h"
#include "today_layer.h"


//window
static Window *s_window;  

//Layers
static struct MapLayer map_layer_struct;
static Layer *arrow_layer1, *arrow_layer2;

  
//static bool position_known = false;
const time_t OUTDATE_TIME = 1200;

// An instance of the struct
static Settings settings;
static SettingsHandler settings_handler;

static ConnectionHandlers connection_handlers;
  
static struct date_layer date_l;

static place_layer place1, place2;

//this is a reference to botom place
static Layer *bottom_place_layer;


static void handle_update_settings(){
  place_handle_update_settings(&place1);
  place_handle_update_settings(&place2);
  date_layer_handle_update_settings(&date_l);
  
  layer_mark_dirty(window_get_root_layer(s_window));
}

static void handle_companion_connection_change(bool connected){
}
  
static void handle_connection_change(bool connected){
  update_place_layer(&place1);
  update_place_layer(&place2);
  date_layer_handle_connection_change(&date_l, connected);
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed){
  time_t now = time(NULL);
  
  /////////////////////////////////////////////////
  time_t time1 = now + place1.place->offset;
  place_layer_update_time(&place1, &time1);
  
  /////////////////////////////////////////////////
  time_t time2 = now + place2.place->offset;
  place_layer_update_time(&place2, &time2);

  /////////////////////////////////////////////////
  date_layer_handle_minute_tick(&date_l, tick_time, units_changed);

  /////////////////////////////////////////////////
  if (map_layer_redraw_required_minute(&map_layer_struct)){
      map_layer_handle_night_pos_update(now, &map_layer_struct);
      date_layer_handle_night_pos_update(&date_l, tick_time, units_changed);
  }
}

static void move_layers_simple(AnimationProgress progress, void *context){
  // Get the total available screen real-estate
  GRect bounds = layer_get_unobstructed_bounds(window_get_root_layer(s_window));
  //Get frame of bubbles
  GRect place_rect = layer_get_frame(place2.place_layer);
  layer_set_frame(place2.place_layer, 
                  GRect(0, bounds.size.h-place_rect.size.h, 
                        place_rect.size.w, place_rect.size.h));
//   //get map rect
//   Layer* map_layer = bitmap_layer_get_layer(map_layer_struct.map_layer);
//   GSize map_size = layer_get_frame(map_layer).size;
//   layer_set_frame(map_layer, 
//                   GRect(0, bounds.size.h-place_rect.size.h/2, 
//                         map_size.w, map_size.h));
}

static void window_load(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Performing window load");
  Layer *window_layer = window_get_root_layer(window);
  //GRect bounds = layer_get_bounds(window_layer);
  //TODO make ALL constants variable
  
  /////////////////////////////////////////////////////////////////
  //load settings
  prv_load_settings(&settings);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done loading settings");

  /////////////////////////////////////////////////////////////////
  //map layer
  Layer *map_layer = map_leyer_create(GPoint(0, 48), &map_layer_struct);
  layer_add_child(window_layer, map_layer);
  GRect map_frame = layer_get_bounds(map_layer);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done loading map"); 

  ///////////////////////////////////////////////////////////////
  //arrow layers
  arrow_layer1 = arrows_layer_create(map_frame,
                                     &place1, 
                                     &settings, 
                                     get_point_on_map,
                                     false);
  layer_add_child(map_layer, arrow_layer1);
  arrow_layer2 = arrows_layer_create(map_frame,
                                     &place2, 
                                     &settings, 
                                     get_point_on_map,
                                     true);
  layer_add_child(map_layer, arrow_layer2);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done loading arrows");

  /////////////////////////////////////////////////////////////////
  //date
  date_l.settings = &settings;
  Layer *date_root = date_layer_create(map_frame, &date_l);
  layer_add_child(map_layer, date_root);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done loading overlay");

  
  /////////////////////////////////////////////////////////////////
  //place layers
  create_place_layer_default(&place1, &settings.place1, 0,
                             &settings, window_layer);
  create_place_layer_default(&place2, &settings.place2, 120,                                 
                             &settings, window_layer);
  bottom_place_layer = place2.place_layer;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done loading place layers");  
  
  ///////////////////////////////////////////////////////////////
  //loading settings
  handle_update_settings();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done setting update handle");   
  
  ///////////////////////////////////////////////////////////////
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Making refresh");   
  layer_mark_dirty(window_get_root_layer(window));
    
  //TODO this can be avoided perhaps
  map_layer_struct.redraw_counter = 32000;

  //TODO move logic to outcide
  //event animation API 4.0
  UnobstructedAreaHandlers handlers = {
  //     .will_change = prv_unobstructed_will_change,
    .change = move_layers_simple
  };
  unobstructed_area_service_subscribe(handlers, NULL);
  move_layers_simple(0, NULL);
}

static void window_unload(Window *window) {
  destroy_place_layer(&place1);
  destroy_place_layer(&place2);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Destroyed places");
  
  destroy_date_layer(&date_l);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Destroyed date layer");
  
  layer_destroy(arrow_layer1);
  layer_destroy(arrow_layer2);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Destroyed arrows");
    
  map_layer_destroy(&map_layer_struct);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Destroyed map");  
  unobstructed_area_service_unsubscribe();
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
  //settings handler set up
  settings_handler.settings = &settings;
  settings_handler.callback = handle_update_settings;
  app_message_set_context(&settings_handler);

  // Initialize AppMessage inbox and outbox buffers with a suitable size
  const int inbox_size = 512;
  const int outbox_size = 512;
  app_message_open(inbox_size, outbox_size);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  //bluetooth_connection_service_subscribe(handle_connection_change);
  connection_handlers.pebble_app_connection_handler = handle_connection_change;
  connection_handlers.pebblekit_connection_handler = handle_companion_connection_change;
  connection_service_subscribe(connection_handlers);
}

static void deinit(void) {
  prv_save_settings(&settings);
  app_message_deregister_callbacks();
  tick_timer_service_unsubscribe();
  //bluetooth_connection_service_unsubscribe();
  connection_service_unsubscribe();
  window_destroy(s_window);
}

int main( void ) {
  init();
  app_event_loop();
  deinit();
}