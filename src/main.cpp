#include <Arduino.h>
#include <M5Stack.h>
#include <WiFi.h>
#include <WebSocketClient.h>
#include <ArduinoJson.h>
#include <M5ez.h>
#include <ezTime.h>
#include "UbuntuMono_Regular16pt7b.h"
#include "UbuntuMono_B18pt7b.h"
#include "UbuntuMono_R18pt7b.h"
#include "LittleLordFontleroyNF60pt7b.h"

#define DEG2RAD 0.0174532925

const char* ssid     = "openplotter";
const char* password = "margaritaville";
char path[] = "/signalk/v1/stream?subcribe=none";
char host[] = "10.10.10.1";

//Declare functions
void client_connect();
void subscribe_datastream ();
void server_handshake ();
const char* handleReceivedMessage(String message);
String DegreesToDegMin(float x);
void location_display();
void tank_display();
void electrical_display();
void wind_display();
void displayTank(const char * updates_path);
void displayInfo(const char * updates_path);
void displayElectrical(const char * updates_path);
uint16_t SetTimeFromGPS();
int parseDateTime (int start, int end);

WebSocketClient webSocketClient;

// Use WiFiClient class to create TCP connections
WiFiClient client;

//Declare globals for all the info coming in
float navigation_position_latitude;
float navigation_position_longitude;
float environment_wind_speedTrue;
float environment_wind_angleTrueWater;
float environment_water_temperature;
float environment_wind_angleApparent;
float environment_wind_speedApparent;
float environment_depth_belowTransducer;
float navigation_speedOverGround;
float navigation_magneticVariation;
float navigation_courseOverGroundTrue;
float navigation_headingMagnetic;
float navigation_speedThroughWater;
float navigation_headingTrue;
const char* navigation_datetime_value;
float electrical_batteries_house_voltage;
float electrical_batteries_house_current;
float electrical_batteries_engine_voltage;
float electrical_batteries_engine_current;
float tanks_freshWater_forwardTank_currentLevel;
float tanks_freshWater_starboardTank_currentLevel;

//Use this to set time
unsigned long resetTimeInterval = 300000; //reset time from GPS every 5 minutes

String northSouth = "N ";
String eastWest = "E ";
String latiTude = "";
String longiTude = "";
String old_latiTude = "";
String old_longiTude = "";
int circleCenterX;
int circleCenterY;
float windAngleOld;

const char* handleReceivedMessage(String message){

  //Serial.println(message);
  StaticJsonDocument<4096> doc;
  DeserializationError err = deserializeJson(doc, message);

  // Parse succeeded?
  if (err) {
    Serial.print(F("deserializeJson() returned "));
    Serial.println(err.c_str());
    return "INVALID";
  }
  JsonObject obj = doc.as<JsonObject>();

  const char* updates_path = obj["updates"][0]["values"][0]["path"];

  if (updates_path != nullptr) {

    //load up the global variables
    if (strcmp(updates_path, "navigation.position")==0) {
      navigation_position_longitude = obj["updates"][0]["values"][0]["value"]["longitude"];
      navigation_position_latitude = obj["updates"][0]["values"][0]["value"]["latitude"];
    }
    if (strcmp(updates_path, "environment.wind.speedTrue")==0) {
      environment_wind_speedTrue = obj["updates"][0]["values"][0]["value"];
    }
    if (strcmp(updates_path, "environment.wind.angleTrueWater")==0) {
      environment_wind_angleTrueWater = obj["updates"][0]["values"][0]["value"];
    }
    if (strcmp(updates_path, "environment.water.temperature")==0) {
      environment_water_temperature = obj["updates"][0]["values"][0]["value"];
    }
    if (strcmp(updates_path, "environment.wind.angleApparent")==0) {
      environment_wind_angleApparent = obj["updates"][0]["values"][0]["value"];
    }
    if (strcmp(updates_path, "environment.wind.speedApparent")==0) {
      environment_wind_speedApparent = obj["updates"][0]["values"][0]["value"];
    }
    if (strcmp(updates_path, "environment.depth.belowTransducer")==0) {
      environment_depth_belowTransducer = obj["updates"][0]["values"][0]["value"];
    }
    if (strcmp(updates_path, "navigation.speedOverGround")==0) {
      navigation_speedOverGround = obj["updates"][0]["values"][0]["value"];
    }
    if (strcmp(updates_path, "navigation.magneticVariation")==0) {
      navigation_magneticVariation = obj["updates"][0]["values"][0]["value"];
    }
    if (strcmp(updates_path, "navigation.courseOverGroundTrue")==0) {
      navigation_courseOverGroundTrue = obj["updates"][0]["values"][0]["value"];
    }
    if (strcmp(updates_path, "navigation.headingMagnetic")==0) {
      navigation_headingMagnetic = obj["updates"][0]["values"][0]["value"];
    }
    if (strcmp(updates_path, "navigation.speedThroughWater")==0) {
      navigation_speedThroughWater = obj["updates"][0]["values"][0]["value"];
    }
    if (strcmp(updates_path, "navigation.headingTrue")==0) {
      navigation_headingTrue = obj["updates"][0]["values"][0]["value"];
    }
    if (strcmp(updates_path, "navigation.datetime")==0) {
      navigation_datetime_value = obj["updates"][0]["values"][0]["value"];
    }
    if (strcmp(updates_path, "electrical.batteries.house.voltage")==0) {
      electrical_batteries_house_voltage = obj["updates"][0]["values"][0]["value"];
    }
    if (strcmp(updates_path, "electrical.batteries.house.current")==0) {
      electrical_batteries_house_current = obj["updates"][0]["values"][0]["value"];
    }
    if (strcmp(updates_path, "electrical.batteries.engine.voltage")==0) {
      electrical_batteries_engine_voltage = obj["updates"][0]["values"][0]["value"];
    }
    if (strcmp(updates_path, "electrical.batteries.engine.current")==0) {
      electrical_batteries_engine_current = obj["updates"][0]["values"][0]["value"];
    }
    if (strcmp(updates_path, "tanks.freshWater.forwardTank.currentLevel")==0) {
      tanks_freshWater_forwardTank_currentLevel = obj["updates"][0]["values"][0]["value"];
    }
    if (strcmp(updates_path, "tanks.freshWater.starboardTank.currentLevel")==0) {
      tanks_freshWater_starboardTank_currentLevel = obj["updates"][0]["values"][0]["value"];
    }
    return updates_path;
  }
  return "";
}

void client_connect () {
  // Connect to the websocket server
  if (client.connect(host, 3000)) {
    Serial.println("Connected");
  } else {
    Serial.println("Connection failed.");
  }
}

void server_handshake () {
  // Handshake with the server
  webSocketClient.path = path;
  webSocketClient.host = host;
  if (webSocketClient.handshake(client)) {
    Serial.println("Handshake successful");
  } else {
    Serial.println("Handshake failed.");
    while(1) {
      // Hang on failure
    }
  }
}

void subscribe_datastream () {
  const size_t capacity = JSON_ARRAY_SIZE(20) + 20*JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2);
  DynamicJsonDocument doc(capacity);

  String data;

  data = "{\"context\":\"*\",\"unsubscribe\":[{\"path\": \"*\"}]}";
  webSocketClient.sendData(data);
  data = "";

  delay(1000);
  doc["context"] = "vessels.self";

  JsonArray subscribe = doc.createNestedArray("subscribe");
  JsonObject subscribe_0 = subscribe.createNestedObject();
  subscribe_0["path"] = "navigation.position";
  JsonObject subscribe_1 = subscribe.createNestedObject();
  subscribe_1["path"] = "navigation.speedOverGround";
  //JsonObject subscribe_2 = subscribe.createNestedObject();
  //subscribe_2["path"] = "navigation.magneticVariation";
  //JsonObject subscribe_3 = subscribe.createNestedObject();
  //subscribe_3["path"] = "navigation.magneticVariationAgeOfService";
  JsonObject subscribe_4 = subscribe.createNestedObject();
  subscribe_4["path"] = "navigation.courseOverGroundTrue";
  JsonObject subscribe_5 = subscribe.createNestedObject();
  subscribe_5["path"] = "navigation.headingMagnetic";
  JsonObject subscribe_6 = subscribe.createNestedObject();
  subscribe_6["path"] = "navigation.speedThroughWater";
  JsonObject subscribe_7 = subscribe.createNestedObject();
  subscribe_7["path"] = "navigation.headingTrue";
  JsonObject subscribe_8 = subscribe.createNestedObject();
  subscribe_8["path"] = "navigation.datetime";
  JsonObject subscribe_9 = subscribe.createNestedObject();
  subscribe_9["path"] = "environment.wind.speedTrue";
  JsonObject subscribe_11 = subscribe.createNestedObject();
  subscribe_11["path"] = "environment.wind.angleTrueWater";
  JsonObject subscribe_12 = subscribe.createNestedObject();
  subscribe_12["path"] = "environment.water.temperature";
  JsonObject subscribe_13 = subscribe.createNestedObject();
  subscribe_13["path"] = "environment.wind.angleApparent";
  JsonObject subscribe_14 = subscribe.createNestedObject();
  subscribe_14["path"] = "environment.wind.speedApparent";
  //JsonObject subscribe_15 = subscribe.createNestedObject();
  //subscribe_15["path"] = "environment.water.temperture";
  //JsonObject subscribe_16 = subscribe.createNestedObject();
  //subscribe_16["path"] = "environment.depth.surfaceToTransducer";
  JsonObject subscribe_17 = subscribe.createNestedObject();
  subscribe_17["path"] = "environment.depth.belowTransducer";
  //JsonObject subscribe_18 = subscribe.createNestedObject();
  //subscribe_18["path"] = "environment.depth.belowSurface";
  //JsonObject subscribe_19 = subscribe.createNestedObject();
  //subscribe_19["path"] = "environment.depth.belowKeel";
  JsonObject subscribe_20 = subscribe.createNestedObject();
  subscribe_20["path"] = "electrical.batteries.house.voltage";
  JsonObject subscribe_21 = subscribe.createNestedObject();
  subscribe_21["path"] = "electrical.batteries.house.current";
  JsonObject subscribe_22 = subscribe.createNestedObject();
  subscribe_22["path"] = "electrical.batteries.engine.voltage";
  JsonObject subscribe_23 = subscribe.createNestedObject();
  subscribe_23["path"] = "electrical.batteries.engine.current";
  JsonObject subscribe_24 = subscribe.createNestedObject();
  subscribe_24["path"] = "tanks.freshWater.forwardTank.currentLevel";
  JsonObject subscribe_25 = subscribe.createNestedObject();
  subscribe_25["path"] = "tanks.freshWater.starboardTank.currentLevel";
  serializeJson(doc, data);
  webSocketClient.sendData(data);

  Serial.println("Connection String Sent");
}

// This routine builds a LAT and LONG string
String DegreesToDegMin(float x) {
  // The abs function doesn't work on floats, so we do it manually
  Serial.print("Sent to DegreesToDegMin: ");
  Serial.println(x);

  if (x < 0) {
    x = x * -1.0;
  }
  int degRaw = x;
  float degFloat = float(degRaw);
  float minutesRemainder = (x - degFloat) * 60.0;
  String degMin = "";
  if (degRaw < 100) degMin = "0";
  if (degRaw < 10) degMin = degMin + "0";
  degMin = degMin + degRaw;
  degMin = degMin + " ";
  if (minutesRemainder < 10) degMin = degMin + "0";
  degMin = degMin + String(minutesRemainder, 4);
  degMin = degMin + "\'";

  return (degMin);
}

// this draws arcs
int fillArc(int x, int y, int start_angle, int seg_count, int rx, int ry, int w, unsigned int colour) {

  byte seg = 7; // Angle a single segment subtends
  byte inc = 6; // Draw segments every 6 degrees

  // Draw colour blocks every inc degrees
  for (int i = start_angle; i < start_angle + seg * seg_count; i += inc) {
    // Calculate pair of coordinates for segment start
    float sx = cos((i - 90) * DEG2RAD);
    float sy = sin((i - 90) * DEG2RAD);
    uint16_t x0 = sx * (rx - w) + x;
    uint16_t y0 = sy * (ry - w) + y;
    uint16_t x1 = sx * rx + x;
    uint16_t y1 = sy * ry + y;

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * DEG2RAD);
    float sy2 = sin((i + seg - 90) * DEG2RAD);
    int x2 = sx2 * (rx - w) + x;
    int y2 = sy2 * (ry - w) + y;
    int x3 = sx2 * rx + x;
    int y3 = sy2 * ry + y;

    M5.Lcd.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
    M5.Lcd.fillTriangle(x1, y1, x2, y2, x3, y3, colour);
  }
  return 0;
}

uint16_t SetTimeFromGPS(){
  uint8_t GPSMonth, GPSDay, GPSHour, GPSMinute, GPSSecond;
  uint16_t GPSYear;
  String data;
  const char* updates_path;

  Timezone UTC;
  if (client.connected()) {
  webSocketClient.getData(data);
    if (data.length() > 0) {
      updates_path = handleReceivedMessage(data);
      if (strcmp(updates_path, "navigation.datetime")==0) {
        // A typical date/time string looks like this: 2021-01-11T03:43:09.000Z
        GPSYear = parseDateTime(0,4);
        GPSMonth = parseDateTime(5,7);
        GPSDay = parseDateTime(8,10);
        GPSHour = parseDateTime(11,13);
        GPSMinute = parseDateTime(14,16);
        GPSSecond = parseDateTime(17,20);
        /*Serial.println(GPSYear);
        Serial.println(GPSMonth);
        Serial.println(GPSDay);
        Serial.println(GPSHour);
        Serial.println(GPSMinute);
        Serial.println(GPSSecond);*/
        setTime(GPSHour, GPSMinute, GPSSecond, GPSDay, GPSMonth, GPSYear);
        Serial.println("Setting Universal Standard Time: " + UTC.dateTime());
      }
    }
  } else {
    Serial.println("Client disconnected. Reconnecting");
    client_connect();
    server_handshake ();
    subscribe_datastream();
  }
  return 1;
}

int parseDateTime (int start, int end){
  String DTReturn = "";
  for (int i = start; i < end; i++){
    DTReturn = DTReturn + navigation_datetime_value[i];
  }
  return DTReturn.toInt();
}
/****************************************************************
 * SCREEN DISPLAY FUNCTIONS
 * *************************************************************/

void location_display() {
  String data;
  ez.addEvent(SetTimeFromGPS);
  ezMenu locationDisplay;
  ez.header.show("Location");
  ez.buttons.show("Electrical # Main Menu # Wind");
  String btnpressed = ez.buttons.poll();
  while (btnpressed == "") {
    if (client.connected()) {
      webSocketClient.getData(data);
      if (data.length() > 0) {
        displayInfo(handleReceivedMessage(data));
        M5.Lcd.setTextWrap(true, true);
        }
    } else {
      Serial.println("Client disconnected. Reconnecting");
      client_connect();
      server_handshake ();
      subscribe_datastream();
    }
    delay(10);  // <- fixes some issues with WiFi stability
    btnpressed = ez.buttons.poll();
  }
  if (btnpressed == "Electrical") {
    ez.canvas.reset();
    //electrical_display();
  }
  if (btnpressed == "Wind") {
    ez.canvas.reset();
    wind_display();
  }
}

void displayInfo(const char * updates_path) {
  ez.canvas.font(&FreeSans18pt7b);
  // Latitude
  if (strcmp(updates_path, "navigation.position")==0) {
    Serial.println(navigation_position_latitude);
    if (navigation_position_latitude < 0) {
      northSouth = "S ";
      }
      else
      {
        northSouth = "N ";
      }
    latiTude = northSouth;
    latiTude += DegreesToDegMin(navigation_position_latitude);
    Serial.print("Converted Latitude is: ");
    Serial.println(latiTude);
    ez.canvas.lmargin(10);
    ez.canvas.y(ez.canvas.top() + 10);
    ez.canvas.x(ez.canvas.lmargin());
    ez.canvas.font(&UbuntuMono_B18pt7b);
    ez.canvas.println("LAT/LON");
    ez.canvas.y(ez.canvas.top() + 40);
    ez.canvas.x(ez.canvas.lmargin());
    ez.canvas.font(&UbuntuMono_R18pt7b);
    if (latiTude != old_latiTude) {
      ez.canvas.color(ez.theme->background);
      ez.canvas.print(old_latiTude);
      ez.canvas.y(ez.canvas.top() + 40);
      ez.canvas.x(ez.canvas.lmargin());
      ez.canvas.color(ez.theme->foreground);
      ez.canvas.print(latiTude);
      old_latiTude = latiTude;
    }
    //Longitude
    if (navigation_position_longitude < 0) {
      eastWest = "W ";
    }
    else
    {
      eastWest = "E ";
    }
    longiTude = eastWest;
    longiTude += DegreesToDegMin(navigation_position_longitude);
    ez.canvas.y(ez.canvas.top() + 70);
    ez.canvas.x(ez.canvas.lmargin());
    if (longiTude != old_longiTude) {
      ez.canvas.color(ez.theme->background);
      ez.canvas.print(old_longiTude);
      ez.canvas.y(ez.canvas.top() + 70);
      ez.canvas.x(ez.canvas.lmargin());
      ez.canvas.color(ez.theme->foreground);
      ez.canvas.println(longiTude);
      old_longiTude = longiTude;
    }
    }
// We are going to print in knots, so all speeds need to be converted from meters/sec
// Speed over ground
  if (strcmp(updates_path, "navigation.speedOverGround")==0) {
    ez.canvas.y(ez.canvas.top() + 100);
    ez.canvas.x(ez.canvas.lmargin());
    M5.lcd.fillRect(0, ez.canvas.top() + 98, 160, ez.fontHeight(), ez.theme->background); //erase partial place for updating data
    ez.canvas.font(&UbuntuMono_B18pt7b);
    ez.canvas.print("SOG:");
    ez.canvas.font(&UbuntuMono_R18pt7b);
    if ((navigation_speedOverGround/0.514444) < 10) ez.canvas.print(" ");
    ez.canvas.print(navigation_speedOverGround/0.514444, 1);
    }
  // Course over ground
  if (strcmp(updates_path, "navigation.courseOverGroundTrue")==0) {
    ez.canvas.y(ez.canvas.top() + 100);
    ez.canvas.x(ez.canvas.lmargin() + 160);
    M5.lcd.fillRect(160, ez.canvas.top() + 98, 160, ez.fontHeight(), ez.theme->background); //erase partial place for updating data
    ez.canvas.font(&UbuntuMono_B18pt7b);
    ez.canvas.print("COG:");
    ez.canvas.font(&UbuntuMono_R18pt7b);
    ez.canvas.print(navigation_courseOverGroundTrue*57.2958, 0);
    }
  //Apparent Wind Speed
  if (strcmp(updates_path, "environment.wind.speedApparent")==0) {
    ez.canvas.y(ez.canvas.top() + 130);
    ez.canvas.x(ez.canvas.lmargin());
    M5.lcd.fillRect(0, ez.canvas.top() + 130, 170, ez.fontHeight(), ez.theme->background); //erase partial place for updating data
    ez.canvas.font(&UbuntuMono_B18pt7b);
    ez.canvas.print("AWS:");
    ez.canvas.font(&UbuntuMono_R18pt7b);
    if (environment_wind_speedApparent/0.514444 < 10)  ez.canvas.print(" ");
    ez.canvas.print(environment_wind_speedApparent/0.514444,1);
    }
  // Apparent Wind Angle
  if (strcmp(updates_path, "environment.wind.angleApparent")==0) {
    ez.canvas.y(ez.canvas.top() + 130);
    ez.canvas.x(ez.canvas.lmargin() + 160);
    M5.lcd.fillRect(170, ez.canvas.top() + 130, 160, ez.fontHeight(), ez.theme->background); //erase partial place for updating data
    ez.canvas.font(&UbuntuMono_B18pt7b);
    ez.canvas.print("AWA:");
    ez.canvas.font(&UbuntuMono_R18pt7b);
    if (environment_wind_angleApparent <0) {
      ez.canvas.print("P");
      ez.canvas.print(environment_wind_angleApparent*-57.2958,0);
      }
    else {
      ez.canvas.print("S");
      ez.canvas.print(environment_wind_angleApparent*57.2958,0);
      }
    }
  // True Wind Speed
  if (strcmp(updates_path, "environment.wind.speedTrue")==0) {
    ez.canvas.y(ez.canvas.top() + 160);
    ez.canvas.x(ez.canvas.lmargin());
    M5.lcd.fillRect(0, ez.canvas.top() + 158, 170, ez.fontHeight(), ez.theme->background); //erase partial place for updating data
    ez.canvas.font(&UbuntuMono_B18pt7b);
    ez.canvas.print("TWS:");
    ez.canvas.font(&UbuntuMono_R18pt7b);
    if (environment_wind_speedTrue/0.514444 < 10)  ez.canvas.print(" ");
    ez.canvas.print(environment_wind_speedTrue/0.514444,1);
    }
  // True Wind Angle
  if (strcmp(updates_path, "environment.wind.angleTrueWater")==0) {
    ez.canvas.y(ez.canvas.top() + 160);
    ez.canvas.x(ez.canvas.lmargin() + 160);
    M5.lcd.fillRect(170, ez.canvas.top() + 158, 160, ez.fontHeight(), ez.theme->background); //erase partial place for updating data
    ez.canvas.font(&UbuntuMono_B18pt7b);
    ez.canvas.print("TWA:");
    ez.canvas.font(&UbuntuMono_R18pt7b);
    int angledegrees;
    if (environment_wind_angleTrueWater < 0) {
      angledegrees = (environment_wind_angleTrueWater*-57.2958)+180;
    }
    else {
      angledegrees = environment_wind_angleTrueWater*57.2958;
    }
    ez.canvas.print(angledegrees);
  }
  Serial.println();
}

void wind_display() {
  int realTrueWindangle;
  String btnpressed = ez.buttons.poll();
  String data;
  ez.addEvent(SetTimeFromGPS);
  ezMenu windDisplay;
  ez.header.show("Wind");
  ez.buttons.show("Location # Main Menu # Tanks");
  ez.canvas.font(&UbuntuMono_Regular16pt7b);

  // Draw a compass rose
  M5.Lcd.fillEllipse(ez.canvas.lmargin() + 160, ez.canvas.top() + 100, 93, 93, ez.theme->foreground);
  M5.Lcd.fillEllipse(ez.canvas.lmargin() + 160, ez.canvas.top() + 100, 90, 90, ez.theme->background);
  circleCenterX = ez.canvas.lmargin() + 160;
  circleCenterY = ez.canvas.top() + 100;
  // do the small ticks every 15 degrees
  int roseAnglemark = 0;
  while (roseAnglemark < 360) {
    float roseAnglemarkradian = ((roseAnglemark - 90) * 71) / 4068.0;
    M5.Lcd.drawLine (int(circleCenterX + (80 * cos(roseAnglemarkradian))), int(circleCenterY + (80 * sin(roseAnglemarkradian))), int(circleCenterX + (90 * cos(roseAnglemarkradian))), int(circleCenterY + (90 * sin(roseAnglemarkradian))), ez.theme->foreground);
    roseAnglemark += 15;
  }
  // do the longer ticks every 45 degrees
  roseAnglemark = 0;
  while (roseAnglemark < 360) {
    float roseAnglemarkradian = ((roseAnglemark - 90) * 71) / 4068.0;
    M5.Lcd.drawLine (int(circleCenterX + (70 * cos(roseAnglemarkradian))), int(circleCenterY + (70 * sin(roseAnglemarkradian))), int(circleCenterX + (90 * cos(roseAnglemarkradian))), int(circleCenterY + (90 * sin(roseAnglemarkradian))), TFT_RED);
    roseAnglemark += 45;
  }
  // put red and green arcs on each side
  fillArc(circleCenterX, circleCenterY, 45, 14, 93, 93, 5, TFT_GREEN);
  fillArc(circleCenterX, circleCenterY, 210, 14, 93, 93, 5, TFT_RED);

  // put App and True on left and right
  ez.canvas.pos(ez.canvas.lmargin() + 10, ez.canvas.top() + 10);
  ez.canvas.println("App");
  ez.canvas.pos(ez.canvas.lmargin() + 250, ez.canvas.top() + 10);
  ez.canvas.print("True");

  // Run until the Main Menu button is pressed

  while (btnpressed == "") {
    if (client.connected()) {
      webSocketClient.getData(data);
      if (data.length() > 0) {
        const char * updates_path = handleReceivedMessage(data);
        if (strcmp(updates_path, "environment.wind.angleApparent")==0) {
          //adjust so 0 is at the top
          float apparentWindangleradian = environment_wind_angleApparent;
          if (apparentWindangleradian < 0){
            apparentWindangleradian = apparentWindangleradian - (PI/2);
            if (apparentWindangleradian < -PI){
              apparentWindangleradian = (2*PI) - abs(apparentWindangleradian);
            }
          }
          else {
            apparentWindangleradian = apparentWindangleradian - (PI/2);
          }

          // Clear the center of the rose
          M5.Lcd.fillEllipse(ez.canvas.lmargin() + 160, ez.canvas.top() + 100, 69, 69, ez.theme->background);

          // Draw the indicator on the rose
          float pointerWidth = 0.349; // 10 degrees in radians. This will make the pointer 20 degrees on the bottom

          // Get the coords of the pointer
          int xp = int(circleCenterX + (68 * cos(apparentWindangleradian)));
          int yp = int(circleCenterY + (68 * sin(apparentWindangleradian)));
          int xl = int(circleCenterX + (20 * cos(apparentWindangleradian - pointerWidth)));
          int yl = int(circleCenterY + (20 * sin(apparentWindangleradian - pointerWidth)));
          int xr = int(circleCenterX + (20 * cos(apparentWindangleradian + pointerWidth)));
          int yr = int(circleCenterY + (20 * sin(apparentWindangleradian + pointerWidth)));
          M5.Lcd.fillTriangle(xp, yp, xl, yl, xr, yr, ez.theme->foreground);

          // print apparent wind angle and speed on the left
          ez.canvas.pos(ez.canvas.lmargin()-3 + 10, ez.canvas.top() + 40);
          M5.lcd.fillRect(ez.canvas.lmargin()-3 + 10, ez.canvas.top() + 40, 64, 23, ez.theme->background); //erase partial place for updating data
          if (environment_wind_angleApparent <0) {
            ez.canvas.print("P");
            ez.canvas.print(environment_wind_angleApparent*(-180/PI),0);
            }
          else {
            ez.canvas.print("S");
            ez.canvas.print(environment_wind_angleApparent*(180/PI),0);
          }
          M5.Lcd.drawEllipse(ez.canvas.lmargin() + 73, ez.canvas.top() + 43, 3, 3, ez.theme->foreground);
        //}

          M5.lcd.fillRect(ez.canvas.lmargin() + 10, ez.canvas.top() + 160, 70, 23, ez.theme->background); //erase partial place for updating data
          ez.canvas.pos(ez.canvas.lmargin() + 10, ez.canvas.top() + 160);
          ez.canvas.print(environment_wind_speedApparent/0.514444,0);
          ez.canvas.println("KN");

      }

    // Print the true wind on the right
        if (strcmp(updates_path, "environment.wind.angleTrueWater")==0) {
          realTrueWindangle = environment_wind_angleTrueWater * (180/PI);
          if (realTrueWindangle < 0){
            realTrueWindangle = 360 + realTrueWindangle;
          }
          ez.canvas.pos(ez.canvas.lmargin() + 255, ez.canvas.top() + 40);
          M5.lcd.fillRect(ez.canvas.lmargin() + 255, ez.canvas.top() + 40, 50, 23, ez.theme->background); //erase partial place for updating data
          ez.canvas.print(realTrueWindangle);
          M5.Lcd.drawEllipse(ez.canvas.lmargin() + 308, ez.canvas.top() + 43, 3, 3, ez.theme->foreground);
        }
        // Print true wind speed on the lower right
        if (strcmp(updates_path, "environment.wind.speedTrue")==0) {
          M5.lcd.fillRect(ez.canvas.lmargin() + 245, ez.canvas.top() + 160, 70, 23, ez.theme->background); //erase partial place for updating data
          ez.canvas.pos(ez.canvas.lmargin() + 245, ez.canvas.top() + 160);
          ez.canvas.print(environment_wind_speedTrue/0.514444, 0);
          ez.canvas.println("KN");
        }
      }
    }

    btnpressed = ez.buttons.poll();
    }

  if (btnpressed == "Location") {
    ez.canvas.reset();
    location_display();
  }
  if (btnpressed == "Tanks") {
    ez.canvas.reset();
    tank_display();
  }
}

void tank_display() {
  String data;
  ez.addEvent(SetTimeFromGPS);
  ezMenu tankDisplay;
  ez.header.show("Water Tanks");
  ez.buttons.show("Wind # Main Menu # Electrical");
  String btnpressed = ez.buttons.poll();
  while (btnpressed == "") {
    if (client.connected()) {
      webSocketClient.getData(data);
      if (data.length() > 0) {
        displayTank(handleReceivedMessage(data));
        M5.Lcd.setTextWrap(true, true);
        }
    } else {
      Serial.println("Client disconnected. Reconnecting");
      client_connect();
      server_handshake ();
      subscribe_datastream();
    }
    delay(10);  // <- fixes some issues with WiFi stability
    btnpressed = ez.buttons.poll();
  }
  if (btnpressed == "Wind") {
    ez.canvas.reset();
    wind_display();
  }
  if (btnpressed == "Electrical") {
    ez.canvas.reset();
    electrical_display();
  }
}

void displayTank(const char * updates_path) {
  int tank1Percent;
  int tank2Percent;
  int percentStart = 220; //the x position where the dynamic percent text starts
  
  if (strcmp(updates_path, "tanks.freshWater.forwardTank.currentLevel")==0) {
    Serial.println(electrical_batteries_engine_voltage);
    ez.canvas.lmargin(10);
    ez.canvas.font(&FreeSansBold18pt7b);
    ez.canvas.y(ez.canvas.top() + 5);
    
    ez.canvas.y(ez.canvas.top() + 40);
    ez.canvas.x(ez.canvas.lmargin());
    ez.canvas.print("Forward:");
    ez.canvas.font(&FreeSans18pt7b);
    M5.lcd.fillRect(percentStart, ez.canvas.top() + 40, ez.canvas.width() - percentStart, ez.fontHeight(), ez.theme->background);
    ez.canvas.y(ez.canvas.top() + 40);
    ez.canvas.x(ez.canvas.lmargin() + percentStart);
    tank1Percent = tanks_freshWater_forwardTank_currentLevel;
    ez.canvas.print(tank1Percent);
    ez.canvas.println("%");
    ez.canvas.println();
  }
  if (strcmp(updates_path, "tanks.freshWater.starboardTank.currentLevel")==0) {
    Serial.println(tanks_freshWater_starboardTank_currentLevel);
    ez.canvas.lmargin(10);
    ez.canvas.y(ez.canvas.top() + 110);
    ez.canvas.x(ez.canvas.lmargin());
    ez.canvas.font(&FreeSansBold18pt7b);
    ez.canvas.print("Starboard:");
    ez.canvas.font(&FreeSans18pt7b);
    M5.lcd.fillRect(percentStart, ez.canvas.top() + 110, ez.canvas.width() - percentStart, ez.fontHeight(), ez.theme->background);
    ez.canvas.y(ez.canvas.top() + 110);
    ez.canvas.x(ez.canvas.lmargin() + percentStart);
    tank2Percent = tanks_freshWater_starboardTank_currentLevel;
    ez.canvas.print(tank2Percent);
    ez.canvas.println("%");
  }
}

void electrical_display() {
  String data;
  ez.addEvent(SetTimeFromGPS);
  ezMenu electricalDisplay;
  ez.header.show("Batteries");
  ez.buttons.show("Tanks # Main Menu # Location");
  String btnpressed = ez.buttons.poll();
  while (btnpressed == "") {
    if (client.connected()) {
      webSocketClient.getData(data);
      if (data.length() > 0) {
        displayElectrical(handleReceivedMessage(data));
        M5.Lcd.setTextWrap(true, true);
        }
    } else {
      Serial.println("Client disconnected. Reconnecting");
      client_connect();
      server_handshake ();
      subscribe_datastream();
    }
    delay(10);  // <- fixes some issues with WiFi stability
    btnpressed = ez.buttons.poll();
  }
  if (btnpressed == "Tanks") {
    ez.canvas.reset();
    tank_display();
  }
  if (btnpressed == "Location") {
    ez.canvas.reset();
    location_display();
  }
}
void displayElectrical(const char * updates_path) {
  
  if (strcmp(updates_path, "electrical.batteries.house.voltage")==0) {
    Serial.println(electrical_batteries_house_voltage);
    ez.canvas.lmargin(10);
    ez.canvas.y(ez.canvas.top() + 10);
    ez.canvas.x(ez.canvas.lmargin());
    ez.canvas.font(&FreeSansBold18pt7b);
    ez.canvas.print("House");
    ez.canvas.font(&FreeSans18pt7b);
    M5.lcd.fillRect(ez.canvas.lmargin(), ez.canvas.top() + 5 + ez.fontHeight(), ez.canvas.width(), ez.fontHeight(), ez.theme->background);
    ez.canvas.y(ez.canvas.top() + 45);
    ez.canvas.x(ez.canvas.lmargin());
    ez.canvas.print(electrical_batteries_house_voltage);
    ez.canvas.print(" V");
    ez.canvas.y(ez.canvas.top() + 45);
    ez.canvas.x(ez.canvas.width()/2);
    ez.canvas.print(electrical_batteries_house_current);
    ez.canvas.print(" A");
  }
  if (strcmp(updates_path, "electrical.batteries.engine.voltage")==0) {
    Serial.println(electrical_batteries_engine_voltage);
    ez.canvas.lmargin(10);
    ez.canvas.y(ez.canvas.top() + 95);
    ez.canvas.x(ez.canvas.lmargin());
    ez.canvas.font(&FreeSansBold18pt7b);
    ez.canvas.print("Engine");
    ez.canvas.font(&FreeSans18pt7b);
    M5.lcd.fillRect(ez.canvas.lmargin(), ez.canvas.top() + 105 + ez.fontHeight(), ez.canvas.width(), ez.fontHeight(), ez.theme->background);
    ez.canvas.y(ez.canvas.top() + 135);
    ez.canvas.x(ez.canvas.lmargin());
    ez.canvas.print(electrical_batteries_engine_voltage);
    ez.canvas.print(" V");
    ez.canvas.y(ez.canvas.top() + 135);
    ez.canvas.x(ez.canvas.width()/2);
    ez.canvas.print(electrical_batteries_engine_current);
    ez.canvas.print(" A");
  }
}

/****************************************************************
 * SETUP
 * *************************************************************/

void setup() {
  #include <themes/default.h>
  #include <themes/dark.h>
  #include "heyya.h"

  int status = WL_IDLE_STATUS;
  int connectionCount = 30; //try to connect to WiFi for 15 seconds

  M5.begin();
  ez.begin();

  //Turn off NTP time updates
  setInterval(0);
  M5.Lcd.setTextWrap(true, true);
  Serial.begin(115200);
  while (!Serial) continue;
  delay(500);
  ezt::setDebug(INFO);
  ez.canvas.clear();
  ez.canvas.font(&LittleLordFontleroyNF60pt7b);
  ez.canvas.x(40);
  ez.canvas.y(40);
  m5.lcd.setTextColor(BLUE);
  ez.canvas.println("Hey Ya");
  ez.canvas.font(&UbuntuMono_Regular16pt7b);
  ez.canvas.lmargin(10);
  ez.canvas.println("Connecting WiFi");
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to network, SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, password);
    // wait .5 seconds for connection:
    delay(500);
    ez.canvas.print(".");
    connectionCount --;
    if (connectionCount == 0) return;
  }
  ez.addEvent(SetTimeFromGPS);
}

/****************************************************************
 * LOOP
 * *************************************************************/

void loop() {
  ezMenu mainmenu("Hey Ya Info System");
  mainmenu.txtBig();
  mainmenu.addItem("Location", location_display);
  mainmenu.addItem("Wind", wind_display);
  mainmenu.addItem("Tanks", tank_display);
  mainmenu.addItem("Electrical", electrical_display);
  mainmenu.addItem("Settings", ez.settings.menu);
  mainmenu.upOnFirst("last|up");
  mainmenu.downOnLast("first|down");
  mainmenu.run();
}

