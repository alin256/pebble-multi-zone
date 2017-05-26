module.exports = [
  {
    "type": "heading",
    "defaultValue": "App Configuration"
  },
  {
    "type": "text",
    "defaultValue": "Configuration of map locations displayed on the watch. A search for location is performed, so you can put anything and even make typos."
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Places"
      },
      {
      "type": "input",
      "messageKey": "Place1",
      "defaultValue": "London",
      "label": "Place 1",
      "attributes": {
        "placeholder": "eg: New York",
        "limit": 500,
        }
      },
      {
      "type": "input",
      "messageKey": "Place2",
      "defaultValue": "Bergen",
      "label": "Place 2",
      "attributes": {
        "placeholder": "eg: New York",
        "limit": 500,
        }
      }
    ]
  },
  {
    "type": "text",
    "defaultValue": "Customization of the features."
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Featurs"
      }      ,
      {
        "type": "toggle",
        "messageKey": "ShowDate",
        "label": "Show date on the map",
        "defaultValue": true
     },
     {
        "type": "toggle",
        "messageKey": "ShowDOW",
        "label": "Also show day of the week on the map",
        "defaultValue": false
      },      
      {
        "type": "toggle",
        "messageKey": "ShowLocalTime",
        "label": "Show local time on the map when it is different from selected places",
        "defaultValue": false
      },      
      {
        "type": "toggle",
        "messageKey": "ForceShowLocalTime",
        "label": "Always show local time on the map",
        "defaultValue": false
      }
//       ,
//       {
//         "type": "toggle",
//         "messageKey": "Animations",
//         "label": "Enable Animations",
//         "defaultValue": false
//       }
    ]
  },
  {
    "type": "text",
    "defaultValue": "Customization of colors. For those who want full controll."
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Place colors"
      },
      {
        "type": "toggle",
        "messageKey": "CustomColorBubbles",
        "defaultValue": true,
        "label": "Use custom colors for places"        
      },
      {
        "type": "color",
        "messageKey": "BackgroundColor",
        "defaultValue": "0x000000",
        "label": "Background Color"
      },
      {
        "type": "color",
        "messageKey": "TextColor",
        "defaultValue": "0xFFFFFF",
        "label": "Text Color"
      },
      {
        "type": "color",
        "messageKey": "ForegroundColor",
        "defaultValue": "0xFF5500",
        "label": "Foreground Color"
      }
    ]
  },
  {
      "type": "section",
      "items": [
      {
        "type": "heading",
        "defaultValue": "Overlay colors"
      },
      {
        "type": "toggle",
        "messageKey": "CustomColorsMap",
        "defaultValue": false,
        "label": "Use custom colors for places"        
      },
      {
        "type": "color",
        "messageKey": "HighlightMapColor",
        "defaultValue": "0xFFFFFF",
        "label": "Active color on map"
      },
      {
        "type": "color",
        "messageKey": "GrayMapColor",
        "defaultValue": "0xAAAAAA",
        "label": "Inactive color on map"
      }
    ]
  },
//   {
//     "type": "section",
//     "items": [
//       {
//         "type": "heading",
//         "defaultValue": "More Settings"
//       },
//       {
//         "type": "toggle",
//         "messageKey": "ShowLocalTime",
//         "label": "Show local time buble",
//         "defaultValue": false
//       }
// //       ,
// //       {
// //         "type": "toggle",
// //         "messageKey": "Animations",
// //         "label": "Enable Animations",
// //         "defaultValue": false
// //       }
//     ]
//   },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];