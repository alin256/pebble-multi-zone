#pragma once
#include <pebble.h>
#include "my_types.h"

// Persistent storage key
#define SETTINGS_KEY 1
#define SETTINGS_VERSION_KEY 2

const int32_t current_settings_version = 5;

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
} Settings;

// Save the settings to persistent storage
void prv_save_settings(Settings* settings) {
  persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
  persist_write_int(SETTINGS_VERSION_KEY, current_settings_version);
}

void prv_load_settings(Settings* settings){
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
}