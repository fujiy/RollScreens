#include <EEPROM.h>
#include <HomeSpan.h>
#include <WebServer.h>

#include "RollScreen.h"
#include "DEV_Identify.h"

#define LED_PIN    2
#define BUTTON_PIN 25
#define SENSOR_ACTIVE_PIN 12
#define SCREEN_NUM 4

WebServer webServer(80);

DEV_RollScreen *screens[SCREEN_NUM];

void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:

  EEPROM.begin(sizeof(Position) * SCREEN_NUM);

  homeSpan.enableOTA();
  homeSpan.setSketchVersion("0.0.9");
  homeSpan.setControlPin(BUTTON_PIN);
  homeSpan.setStatusPin(LED_PIN);
  homeSpan.setHostNameSuffix("");
  homeSpan.setPortNum(1201);
  /* homeSpan.enableOTA(); */
  homeSpan.setMaxConnections(5);
  homeSpan.setWifiCallback(setupWeb);
  homeSpan.begin(Category::Bridges, "Roll Screen Bridge", "rollscreens");

  new SpanAccessory();
  new DEV_Identify("Roll Screen Bridge","HomeSpan","123-ABD","Roll Screen Bridge","0.9", 3);
  new Service::HAPProtocolInformation();
  new Characteristic::Version("1.1.0");

  new SpanAccessory();
  new DEV_Identify("Roll Screen #1","HomeSpan","123-ABD","Roll Screen","0.9",0);
  screens[0] = new DEV_RollScreen(0, 4,  22, 35, SENSOR_ACTIVE_PIN);
  new SpanAccessory();
  new DEV_Identify("Roll Screen #2","HomeSpan","123-ABD","Roll Screen","0.9",0);
  screens[1] = new DEV_RollScreen(1, 18, 19, 34, SENSOR_ACTIVE_PIN);
  new SpanAccessory();
  new DEV_Identify("Roll Screen #3","HomeSpan","123-ABD","Roll Screen","0.9",0);
  screens[2] = new DEV_RollScreen(2, 33, 32, 39, SENSOR_ACTIVE_PIN);
  new SpanAccessory();
  new DEV_Identify("Roll Screen #4","HomeSpan","123-ABD","Roll Screen","0.9",0);
  screens[3] = new DEV_RollScreen(3, 27, 26, 36, SENSOR_ACTIVE_PIN);
}

void loop() {
  homeSpan.poll();
  webServer.handleClient();
  delay(10);
}

void setupWeb(){
  log_d("Starting web server");
  webServer.begin();
  webServer.on("/", []() {
      sendContent();
    });
  webServer.on("/0", []() {
      receive(0);
      sendContent();
    });
  webServer.on("/1", []() {
      receive(1);
      sendContent();
    });
  webServer.on("/2", []() {
      receive(2);
      sendContent();
    });
  webServer.on("/3", []() {
      receive(3);
      sendContent();
    });
}

void receive(unsigned n) {
  Position p = screens[n]->position;
  for(int i = 0; i < webServer.args(); i++) {
    switch(webServer.argName(i).charAt(0)) {
    case 'c':
      p.current = webServer.arg(i).toInt();
      break;
    case 't':
      p.target = webServer.arg(i).toInt();
      break;
    case 'n':
      p.min = webServer.arg(i).toInt();
      break;
    case 'x':
      p.max = webServer.arg(i).toInt();
      break;
    }
  }
  screens[n]->set_position(p);
}

void form(String &content, unsigned n) {
  content += "<form action='/" + String(n) + "' method='POST'><h2>Screen #";
  content += String(n + 1) + "</h2>";
  content +=
    "<div><label>Current Position:"
    "<input type='number' name='c' value='";
  content += String(screens[n]->position.current);
  content += "'></label></div>";
  content +=
    "<div><label>Target Position:"
    "<input type='number' name='t' value='";
  content += String(screens[n]->position.target);
  content += "'></label></div>";
  content +=
    "<div><label>Minimum:"
    "<input type='number' name='n' value='";
  content += String(screens[n]->position.min);
  content += "'></label></div>";
  content +=
    "<div><label>Maximum:"
    "<input type='number' name='x' value='";
  content += String(screens[n]->position.max);
  content += "'></label></div>";
  content += "<div><button type='submit'>Update</button></div></form>";
}

void sendContent() {
  String content = "<html><body><h1>Roll Screen Configuration</h1>";
  content += "<button onclick='location.assign(\"/\")'>Refresh</button>";
  for (unsigned n = 0; n < SCREEN_NUM; n++) form(content, n);
  content += "</body></html>";
  webServer.send(200, "text/html", content);
}
