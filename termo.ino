
#include <TFT_eSPI.h>
#include "Button2.h"
#include "esp_adc_cal.h"
#include "DHT.h"
#include "WiFi.h"
#include "ESPAsyncWebServer.h"

#define DHTTYPE DHT11 // could be DHT22 as well

#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_PIN             34

#define BUTTON_1            35
#define BUTTON_2            0
#define DEBOUNCE_MS         20
#define DHTPIN              13 // pin for temp sensor
#define NUM_RELAYS          2  // Set number of relays
#define HEAT_RELAY          26
#define FAN_RELAY           27


// Replace with your network credentials
const char* ssid = SECRET_WIFI;
const char* password = "reallyfastwifi";


const char* PARAM_INPUT_1 = "relay";  
const char* PARAM_INPUT_2 = "state";

AsyncWebServer server(80); // should be in setup??

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library for display
DHT dht(DHTPIN, DHTTYPE);
Button2 btn1 = Button2(BUTTON_1);
Button2 btn2 = Button2(BUTTON_2);

// Set number of relays
#define NUM_RELAYS  2

// Assign each GPIO to a relay
int relayGPIOs[NUM_RELAYS] = {26, 27};

long interval = 2000; // stupid hack for timing 
long previousMillis = 0;  // 

long heatInterval = 3 * 60 * 1000; // use to prevent short cycling
long previousHeatMillis = 0 - heatInterval; // allow turning on after reset

float currentTemp; // temp in f saved here
float setHeatTemp = 50; // temp to set heat
boolean canHeat = true; 
boolean isFanOn = false; 


void click(Button2& btn) {
  if (btn == btn1) {
    Serial.println("btn1 press");
    canHeat = true;
    setHeatTemp++;
  } else {
    Serial.println("btn2 press");
    canHeat = true;
    setHeatTemp--;
  }
}

String relayState(int numRelay){
  if(digitalRead(relayGPIOs[numRelay-1])){
    return "checked";
  }
  else {
    return "";
  }
}


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2,p,.set-heat,#set-heat {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px; color: #ECF0F1; background-color: #1C2833}
    .temp {font-family: monospace;}
    .red {color: #D02D25;}
    .blue {color: #253FD0;}
    #set-heat {font-family:monospace;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #566573; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked +.slider {background-color: #2196F3}
    input:checked +.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>

  <h2>Thermo</h2>
  <h2 class="temp">%TEMP% F</h2>
  <span class="set-heat">
   <label for="set-heat">Set:</label>
   <input type="number" min="50" max="90" step="1" value="%SETHEATTEMP%" id="set-heat">
  </span>
  <h4>Can Heat</h4>
  <label class="switch">
    <input type="checkbox" %CANHEATCHECKED% id="enable-heat">
    <span class="slider"></span>
  </label>
  <h4>Turn On Fan</h4>
  <label class="switch">
    <input type="checkbox" %FANSTATUSCHECKED% id="fan">
    <span class="slider"></span>
  </label>


<script>
  const currentTemp = %TEMP%
  let canHeat = %CANHEAT%;
  let setHeatTemp = %SETHEATTEMP%;
  
  function toggleCheckbox(e) {
    if (e.currentTarget.id === "enable-heat") {
      canHeat = !canHeat;
      setColors(setHeatTemp, canHeat);
    }
    var xhr = new XMLHttpRequest();
    xhr.open("PUT", "/"+ e.currentTarget.id +"?state=" + canHeat); 
    xhr.send();
  }
  function handleSetHeat(e) {
    setColors(e.target.value, canHeat);
    var xhr = new XMLHttpRequest();
    xhr.open("PUT", "/heat?set="+ e.target.value);
    xhr.send();
  }
  
  function setColors(set, canHeat) {
    const temp = document.querySelector(".temp");
    const input = document.querySelector("#set-heat");
    if (!canHeat) {
      temp.classList.remove("blue");
      temp.classList.remove("red");
      input.disabled = true;
    } else if (currentTemp < set) {
      temp.classList.remove("blue");
      temp.classList.add("red");
      input.disabled = false;
    } else {
      temp.classList.remove("red");
      temp.classList.add("blue");
      input.disabled = false;
    }
  }
  
  setColors(setHeatTemp, canHeat)
  
  var setHeat = document.getElementById('set-heat');
  setHeat.oninput = handleSetHeat;
  
  var enableHeat = document.getElementById('enable-heat');
  enableHeat.addEventListener('change', toggleCheckbox)

  var enableFan = document.getElementById('fan');
  enableFan.addEventListener('change', toggleCheckbox)
  </script>
</body>
</html>
)rawliteral";


String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="";
    for(int i=1; i<=NUM_RELAYS; i++){
      String relayStateValue = relayState(i);
      buttons+= "<h4>Relay #" + String(i) + " - GPIO " + relayGPIOs[i-1] + "</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + String(i) + "\" "+ relayStateValue +"><span class=\"slider\"></span></label>";
    }
    return buttons;
  }
  if(var == "TEMP") {
    return String(currentTemp);
  }
  if (var == "CANHEAT") {
    return String(canHeat);
  }
  if(var == "SETHEATTEMP") {
    return String(int(setHeatTemp));
  }
  if(var == "CANHEATCHECKED") {
    return canHeat ? "checked" : "";
  }
  if(var == "FANSTATUSCHECKED") {
    return isFanOn ? "checked" : "";
  }
  return String();
}


void setup() {
  Serial.begin(115200);
  Serial.println("Start");
  
  /*
  ADC_EN is the ADC detection enable port
  If the USB port is used for power supply, it is turned on by default.
  If it is powered by battery, it needs to be set to high level
  */
  pinMode(ADC_EN, OUTPUT);
  digitalWrite(ADC_EN, HIGH);
  dht.begin();

  btn1.setClickHandler(click);
  btn2.setClickHandler(click);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(3);
  tft.drawString("Sup", tft.width() / 2, tft.height() / 2);

  for(int i=1; i<=NUM_RELAYS; i++){
    pinMode(relayGPIOs[i-1], OUTPUT);
    digitalWrite(relayGPIOs[i-1], LOW);
  }

 // wifi setup
//  IPAddress local_IP(10, 0, 0, 38);
//  IPAddress gateway(10, 0, 0, 1);
//  IPAddress subnet(255, 255, 255, 0);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

    // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());


  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/heat", HTTP_PUT, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // PUT input1 value on <ESP_IP>/heat?set=<value>
    if (request->hasParam("set")) {
      inputMessage = request->getParam("set")->value();
      setHeatTemp = inputMessage.toFloat();
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println("PUT /heat " + inputMessage);
    request->send(200, "text/plain", "OK");
  });
  
  server.on("/enable-heat", HTTP_PUT, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // PUT input1 value on <ESP_IP>/heat?set=<value>
    if (request->hasParam("state")) {
      inputMessage = request->getParam("state")->value();
      if (inputMessage == "true") {
        canHeat = true;
      } else if (inputMessage = "false") {
        canHeat = false;
      }
  
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println("PUT /enable-heat " + inputMessage);
    request->send(200, "text/plain", "OK");
  });

  server.on("/fan", HTTP_PUT, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // PUT input1 value on <ESP_IP>/heat?set=<value>
    if (request->hasParam("state")) {
      inputMessage = request->getParam("state")->value();
      if (inputMessage == "true") {
        isFanOn = true;
      } else if (inputMessage = "false") {
        isFanOn = false;
      }
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println("PUT /fan " + inputMessage);
    request->send(200, "text/plain", "OK");
  });
  // Start server
  server.begin();
}

void temp_loop() {
   unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis < interval) {
    return;
  }
  
  previousMillis = currentMillis; 

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  currentTemp = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(currentTemp)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  //float hif = dht.computeHeatIndex(currentTemp, h);
  
  boolean shouldHeat = canHeat && (currentTemp < setHeatTemp);
  boolean heatOn = !!digitalRead(HEAT_RELAY);
  
  // exit early if heatinterval not met
  if(currentMillis - previousHeatMillis < heatInterval) {
    return;
  }
  if (heatOn != shouldHeat) {
    previousHeatMillis = currentMillis;
    digitalWrite(HEAT_RELAY, shouldHeat);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  // Wait a few seconds between measurements.
  btn1.loop();
  btn2.loop();
  temp_loop();

  if (canHeat) {
    String set = "Set: " + String(setHeatTemp) + "F";
    tft.drawString(set, tft.width() / 2, tft.height() / 4);
  } else {
    tft.drawString("   Heat Off   ", tft.width() / 2, tft.height() / 4);
  }
  
  if (currentTemp) {
    if (currentTemp < setHeatTemp && canHeat) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
    } else {
      tft.setTextColor(TFT_BLUE, TFT_BLACK);
    }
    String temp = String(currentTemp)+ "F";
    tft.drawString(temp, tft.width() / 2, tft.height() / 2);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  }
  
  if (isFanOn) {
    tft.drawString("Fan On", tft.width() / 2, tft.height() * 0.75);
  } else {
    tft.drawString("      ", tft.width() / 2, tft.height() * 0.75);

  }
}
