
#include <TFT_eSPI.h>
#include "Button2.h"
#include "esp_adc_cal.h"
#include "DHT.h"

#define DHTTYPE DHT11 // could be DHT22 as well

#define ADC_EN              14  //ADC_EN is the ADC detection enable port
#define ADC_PIN             34

#define BUTTON_1            35
#define BUTTON_2            0
#define DHTPIN              13 // pin for temp sensor
#define RELAY_1             27

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library for display
DHT dht(DHTPIN, DHTTYPE);
Button2 btn1 = Button2(BUTTON_1);
Button2 btn2 = Button2(BUTTON_2);

long interval = 2000; // stupid hack for timing 
long previousMillis = 0;  // 

void click(Button2& btn) {
  if (btn == btn1) {
    Serial.println("btn1 press");
    digitalWrite(RELAY_1, LOW);
  } else {
    Serial.println("btn2 press");
    digitalWrite(RELAY_1, HIGH);
  }
}

void setup() {
  // put your setup code here, to run once:
  
    Serial.begin(115200);
    Serial.println("Start");
    pinMode(RELAY_1, OUTPUT);

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
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(0, 0);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(3);
    tft.drawString("Sup", tft.width() / 2, tft.height() / 2);

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
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));
  
 

  tft.fillScreen(TFT_BLACK);
  String temp = "F: " + String(f);
  tft.drawString(temp, tft.width() / 2, tft.height() / 2);
}

void loop() {
  // put your main code here, to run repeatedly:
  // Wait a few seconds between measurements.
  btn1.loop();
  btn2.loop();
  temp_loop();
 
}
