#ifndef SETTINGS_H
#define SETTINGS_H

#include <pebble.h>
#include "place_description.h"


#define SETTINGS_VERSION 97

// Define our settings struct
typedef struct ClaySettings {
  GColor TextColor;
  
  GColor BackgroundColor;
  GColor ForegroundColor;

  GColor HighlightColor;
  GColor ShadowColor;

  struct place_descrition place1;
  struct place_descrition place2;
  bool places_were_swapped;

  //struct place_descrition place_cur;
  //time_t last_update;
  //int8_t show_dow;
  int8_t show_local_time;
  int8_t allways_show_local_time;
  int8_t show_date;

  //int tmp;
} Settings;

typedef struct SettingsHandler{
  Settings *settings;
  void (*callback)();
}SettingsHandler;

// Save the settings to persistent storage
void prv_save_settings(Settings* settings);

void prv_load_settings(Settings* settings);

// Called when an incoming message from PebbleKitJS is dropped
void in_dropped_handler(AppMessageResult reason, void *context);

// Called when PebbleKitJS does not acknowledge receipt of a message
void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context);


// Called when a message is received from PebbleKitJS
void in_received_handler(DictionaryIterator *received, void *context);

#endif
