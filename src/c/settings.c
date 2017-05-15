#include <pebble.h>
#include "settings.h"

#define current_settings_version 2


// Save the settings to persistent storage
void prv_save_settings(Settings* settings)
{
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
  persist_write_int(SETTINGS_VERSION_KEY, current_settings_version);
}

// // Define our settings struct
// typedef struct ClaySettings {
//   GColor BackgroundColor;
//   GColor ForegroundColor;
//   GColor TextColor;
//   struct place_descrition place1;
//   struct place_descrition place2;
//   struct place_descrition place_cur;
//   int8_t show_local_time;
//   time_t last_update;
// } Settings;

void init_settings(Settings *settings){
  APP_LOG(APP_LOG_LEVEL_DEBUG, "No settings found. Applying default setting.");
  settings->BackgroundColor = GColorBlack;
  settings->ForegroundColor = GColorRed;
  settings->TextColor = GColorWhite;
  settings->show_local_time = false;
}

void prv_load_settings(Settings* settings)
{
  if (persist_exists(SETTINGS_KEY)){
    if (persist_exists(SETTINGS_VERSION_KEY)){
      int32_t set_ver = persist_read_int(SETTINGS_VERSION_KEY);
      if (set_ver == current_settings_version){
        persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Loaded settings successfully"); 
        return;
        //window_set_background_color(s_window, settings.BackgroundColor);
      }
    }
  }
  init_settings(settings);
}

// Called when an incoming message from PebbleKitJS is dropped
void in_dropped_handler(AppMessageResult reason, void *context)
{  
}

// Called when PebbleKitJS does not acknowledge receipt of a message
void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context)
{
}

void reorder_places_if_needed(struct place_descrition *place1, struct place_descrition *place2){
  if (place1->y > place2->y){
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Performing place swap");
    struct place_descrition tmp;
    memcpy(&tmp, place1, sizeof(struct place_descrition));
    memcpy(place1, place2, sizeof(struct place_descrition));
    memcpy(place2, &tmp, sizeof(struct place_descrition));    
  }
}

// Called when a message is received from PebbleKitJS
void in_received_handler(DictionaryIterator *received, void *context)
{
  //would be reserved if we update based on GPS in the future
  //Tuple *reason_t = dict_find(received, MESSAGE_KEY_UpdateReason);
  SettingsHandler *handler = (SettingsHandler*) context;
  Settings *settings = handler->settings;
    
  Tuple *show_local_t = dict_find(received, MESSAGE_KEY_ShowLocalTime);
  if (show_local_t){
    settings->show_local_time = show_local_t->value->int16;
  }
  
  Tuple *back_color_t = dict_find(received, MESSAGE_KEY_BackgroundColor);
  if (back_color_t){
    settings->BackgroundColor = GColorFromHEX(back_color_t->value->int32);
  }
  
  Tuple *front_color_t = dict_find(received, MESSAGE_KEY_ForegroundColor);
  if (front_color_t){
    settings->ForegroundColor = GColorFromHEX(front_color_t->value->int32);
  }

  Tuple *text_color_t = dict_find(received, MESSAGE_KEY_TextColor);
  if (text_color_t){
    settings->TextColor = GColorFromHEX(text_color_t->value->int32);
  }
  
  //if (reason_t)
  //update of locations
  {
    //uint32_t reason_id = reason_t->value->uint32;
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Status: %d", (int) reason_id);
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
      update_place(&settings->place1, city_t, offset_t, x_t, y_t);

      //updated place 2
      city_t = dict_find(received, MESSAGE_KEY_Place2);
      offset_t = dict_find(received, MESSAGE_KEY_ZoneOffset2);
      x_t = dict_find(received, MESSAGE_KEY_P2X);
      y_t = dict_find(received, MESSAGE_KEY_P2Y);
      update_place(&settings->place2, city_t, offset_t, x_t, y_t);
      
      //reorder
      reorder_places_if_needed(&settings->place1, &settings->place2);

      
      //GPS
      //       if (reason_id == 42){//GPS
      //         //updated place Curent
      //         //city_t = dict_find(received, MESSAGE_KEY_CurPlace);
      //         //offset_t = dict_find(received, MESSAGE_KEY_ZoneOffsetCur);
      //         x_t = dict_find(received, MESSAGE_KEY_P_CUR_X);
      //         y_t = dict_find(received, MESSAGE_KEY_P_CUR_Y);
      //         //update_place(&settings.place_cur, &settings.place_cur.place_name, offset_t, x_t, y_t);
      //         update_place_partial(&settings.place_cur, x_t, y_t);
      //         update_floating_place(&current);
      //         settings.last_update = time(NULL);
      //       }
      //       else if  (reason_id == 43){
      //         //position_known = false;         
      //       }
    }
    prv_save_settings(settings);
    handler->callback();
  }
  
  //send_message();
}