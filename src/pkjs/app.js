// Import the Clay package
var Clay = require('pebble-clay');
//message_keys
var messageKeys = require('message_keys');
// Load our Clay configuration file
var clayConfig = require('./config');
// Initialize Clay
var clay = new Clay(clayConfig);

var maxAngle = 65536; //360
var dict = {};

Pebble.addEventListener('webviewclosed', function(e) {
  if (e && !e.response) {
    return;
  }

  // Get the keys and values from each config item
  dict = clay.getSettings(e.response);
  dict[messageKeys.UpdateReason] = 0; //places not to be updated
  
  
  //   dict[messageKeys.P1X] = 10;
  //   dict[messageKeys.P1Y] = 11;
  //   dict[messageKeys.P2X] = 20;
  //   dict[messageKeys.P2Y] = 21;
  //dict[messageKeys.Place1] = dict[messageKeys.Place1].trim();
  //dict[messageKeys.Place2] = dict[messageKeys.Place2].trim();
  
  console.log("From UI: " + JSON.stringify(dict));
  fetchLocation(dict[messageKeys.Place1], messageKeys.Place1, messageKeys.P1X, messageKeys.P1Y, messageKeys.ZoneOffset1, 1);
  fetchLocation(dict[messageKeys.Place2], messageKeys.Place2, messageKeys.P2X, messageKeys.P2Y, messageKeys.ZoneOffset2, 2);
//   fetchLocation(dict[messageKeys.CurPlace], messageKeys.CurPlace, messageKeys.P_CUR_X, messageKeys.P_CUR_Y, 
//                  messageKeys.ZoneOffsetCur, 3);
});

function doneTimeZone(timeZoneDataString, keyZone, reason, tryNum)
{
    console.log("Recieved time zone: " + timeZoneDataString);
    var timeZoneData = JSON.parse(timeZoneDataString);
    console.log("Status time zone: " + JSON.stringify(timeZoneData));
    if (timeZoneData.status == "OK"){
      //register values
      var offset = timeZoneData.rawOffset + timeZoneData.dstOffset;
      dict[keyZone] = offset;
      console.log("Time zone found: " + timeZoneData.timeZoneId + " ofset: " + offset);
      
      //send info back to watch
      dict[messageKeys.UpdateReason] = reason;
      // Send settings values to watch side
      Pebble.sendAppMessage(dict, function(e) {
        console.log('Sent update for location, id: '+reason);
      }, function(e) {
        if (tryNum<3){
          console.log('Retrying sending location data!');
          console.log(JSON.stringify(e));
          doneTimeZone(timeZoneData, keyZone, reason, tryNum+1);
        }
        else{
          console.log('Failed to send config data!');
          console.log(JSON.stringify(e));
        }
      });
    }  
}

var urlTimeZonePrefix = "https://maps.googleapis.com/maps/api/timezone/json?location=";

function fetchTimeZone(lat, lng, keyZone, reason)
{
  console.log("Requesting time zone: " + lat + "," + lng);
  var nowTime = Math.floor(Date.now()/1000);
  httpGetAsync(urlTimeZonePrefix + lat + "," + lng + "&timestamp=" + nowTime, function(e){
    doneTimeZone(e, keyZone, reason, 0);
  });
}

function doneLocation(locationDataString, keyPlace, keyX, keyY, keyZone, reason)
{
    //register values
    //console.log("Status location: " + JSON.stringify(locationData));
    //console.log("Status location: " + locationData);
    var locationData = JSON.parse(locationDataString);
    if (locationData.status == "OK"){
      var niceAddress = locationData.results[0].formatted_address;
      var tmp_str = niceAddress;
      var c_address = niceAddress.substr(0, niceAddress.indexOf(','));
      if (!isNaN(c_address)){
        console.log("Number, not address: " + c_address); 
        tmp_str = tmp_str.substr(tmp_str.indexOf(',')+1) + ",";
        c_address = tmp_str.substr(0, tmp_str.indexOf(','));
      }
      c_address = c_address.replace("Islands", "Isl.");
      c_address = c_address.replace("Island", "Isl.");

      dict[keyPlace] = c_address;
      dict[keyX] = Math.round((locationData.results[0].geometry.location.lng+180)*maxAngle/360);
      dict[keyY] = Math.round((-locationData.results[0].geometry.location.lat)*maxAngle/180);
        //Math.round((locationData.results[0].geometry.location.lat)*maxAngle/360);
      //dict[keyY] = Math.floor((-locationData.results[0].geometry.location.lat+90)*maxAngle/360);
      console.log("Location found: " + niceAddress  + 
                  JSON.stringify(locationData.results[0].geometry.location));
      fetchTimeZone(locationData.results[0].geometry.location.lat, locationData.results[0].geometry.location.lng, 
                   keyZone, reason);
    }
}

var urlLocationPrefix = "https://maps.googleapis.com/maps/api/geocode/json?address=";
    
function fetchLocation(locationName, keyPlace, keyX, keyY, keyZone, reason)
{
  console.log("Requesting location: " + locationName);
  //console.log(urlLocationPrefix + locationName);
  httpGetAsync(urlLocationPrefix + locationName, function(e){
    doneLocation(e, keyPlace, keyX, keyY, keyZone, reason);
  });
}

function httpGetAsync(theUrl, callback)
{
    var xmlHttp = new XMLHttpRequest();
    xmlHttp.onreadystatechange = function() { 
        if (xmlHttp.readyState == 4)
          {
            //console.log("Recieved; status: "+xmlHttp.status+"; from: " + theUrl);
            if(xmlHttp.status == 200){
                //console.log("Callback!");
                callback(xmlHttp.responseText);
             }
          }
    };
    theUrl = theUrl.replace(/ /g,"+");
    console.log("Request: " + theUrl);
    xmlHttp.open("GET", theUrl, true); // true for asynchronous 
    xmlHttp.send(null);
}

///////location
// function locationSuccess(pos) {
//   var coordinates = pos.coords;
//   console.log(coordinates);
//   //messageKeys.CurPlace, 
//   var keyX = messageKeys.P_CUR_X;
//   var keyY = messageKeys.P_CUR_Y;
//   dict[keyX] = Math.floor((coordinates.longitude+180)*maxAngle/360);
//   //readonly attribute double latitude;
//   //readonly attribute double longitude;
//   dict[keyY] = Math.floor((-coordinates.latitude+90)*maxAngle/360);
//   dict[messageKeys.UpdateReason] = 42; //GPS
//   Pebble.sendAppMessage(dict, function(e) {
//       console.log('Sent update for current location');
//     }, function(e) {
//       console.log('Failed to send config data!');
//       console.log(JSON.stringify(e));
//     });
// }

// function locationError(err) {
//   console.warn('location error (' + err.code + '): ' + err.message);
//   dict[messageKeys.UpdateReason] = 43; //no GPS
//   Pebble.sendAppMessage(dict, function(e) {
//       console.log('Sent update for current location');
//     }, function(e) {
//       console.log('Failed to send config data!');
//       console.log(JSON.stringify(e));
//     });  
// }

// var locationOptions = {
//   'enableHighAccuracy': false,
//   'timeout': 5000, //do not update if not available
//   'maximumAge': 1200000
// };

// Pebble.addEventListener('appmessage', function (e) {
//   console.log('message: ');
//   console.log(e.payload.Request);
//   if (e.payload.Request == 1){
//    navigator.geolocation.getCurrentPosition(locationSuccess, locationError,
//      locationOptions);
//    console.log('Position requested!');
//   }
// });

////// other
// Pebble.addEventListener('ready', function (e) {
//   console.log('connect!' + e.ready);
//   navigator.geolocation.getCurrentPosition(locationSuccess, locationError,
//     locationOptions);
//   console.log(e.type);
// });


// // Fetch stock data for a given stock symbol (NYSE or NASDAQ only) from markitondemand.com
// // & send the stock price back to the watch via app message
// // API documentation at http://dev.markitondemand.com/#doc
// function fetchStockQuote(symbol, isInitMsg) {
//   var response;
//   var req = new XMLHttpRequest();
//   // build the GET request
//   req.open('GET', "http://dev.markitondemand.com/Api/Quote/json?" +
//     "symbol=" + symbol, true);
//   req.onload = function(e) {
//     if (req.readyState == 4) {
//       // 200 - HTTP OK
//       if(req.status == 200) {
//         console.log(req.responseText);
//         response = JSON.parse(req.responseText);
//         var price;
//         if (response.Message) {
//           // the markitondemand API sends a response with a Message
//           // field when the symbol is not found
//           Pebble.sendAppMessage({
//             "price": "Not Found"});
//         }
//         if (response.Data) {
//           // data found, look for LastPrice
//           price = response.Data.LastPrice;
//           console.log(price);

//           var msg = {};
//           if (isInitMsg) {
//             msg.init = true;
//             msg.symbol = symbol;
//           }
//           msg.price = "$" + price.toString();
//           Pebble.sendAppMessage(msg);
//         }
//       } else {
//         console.log("Request returned error code " + req.status.toString());
//       }
//     }
//   };
//   req.send(null);
// }




