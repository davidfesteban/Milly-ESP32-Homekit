#include <HomeSpan.h>
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#define INFO_LED 2              // Status LED Pin
#define CANDLE_PIN 13           // Pin for the candle light
#define NEOPIXEL_PIN 15         // GPIO pin connected to NeoPixel data
#define NUMPIXELS 15            // Number of NeoPixels
#define BRIGHTNESS_MAX 255      // Maximum brightness for NeoPixel

Adafruit_NeoPixel strip(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// State variables
bool candle_light_on = false;
float candle_brightness = BRIGHTNESS_MAX;
float candle_saturation = 100;
uint8_t candle_hue = 30;

bool pixel_light_on = false;
float pixel_brightness = BRIGHTNESS_MAX;
float pixel_saturation = 100;
uint8_t pixel_hue = 30;

// Helper Function: Convert HSB to RGB
void hsbToRgb(float h, float s, float v, uint8_t &r, uint8_t &g, uint8_t &b) {
  float c = v * s;
  float x = c * (1 - abs(fmod(h / 60.0, 2) - 1));
  float m = v - c;

  float rgb[3] = {0, 0, 0};  // Temporary array for RGB values

  int sector = int(h / 60.0) % 6;  // Determine hue sector
  switch (sector) {
    case 0: rgb[0] = c; rgb[1] = x; break;
    case 1: rgb[0] = x; rgb[1] = c; break;
    case 2: rgb[1] = c; rgb[2] = x; break;
    case 3: rgb[1] = x; rgb[2] = c; break;
    case 4: rgb[0] = x; rgb[2] = c; break;
    case 5: rgb[0] = c; rgb[2] = x; break;
  }

  // Scale and adjust for brightness
  r = (rgb[0] + m) * 255;
  g = (rgb[1] + m) * 255;
  b = (rgb[2] + m) * 255;
}

// Light Accessory Class
struct NeoLight : Service::LightBulb {
  SpanCharacteristic *power;
  SpanCharacteristic *bright;
  SpanCharacteristic *hueChar;
  SpanCharacteristic *saturationChar;

  NeoLight()
    : Service::LightBulb() {
    power = new Characteristic::On();
    bright = new Characteristic::Brightness(100);
    hueChar = new Characteristic::Hue(0);
    saturationChar = new Characteristic::Saturation(100);
  }

  bool update() override {
    pixel_light_on = power->getNewVal();
    pixel_brightness = map(bright->getNewVal(), 0, 100, 0, 255);
    pixel_hue = hueChar->getNewVal();
    pixel_saturation = saturationChar->getNewVal() / 100.0;
    return true;  // No immediate NeoPixel update; handled in loop
  }
};

// Candle Light Accessory Class
struct CandleLight : Service::LightBulb {
  SpanCharacteristic *power;
  SpanCharacteristic *bright;
  SpanCharacteristic *hueChar;
  SpanCharacteristic *saturationChar;

  CandleLight()
    : Service::LightBulb() {
    power = new Characteristic::On();
    bright = new Characteristic::Brightness(100);
    hueChar = new Characteristic::Hue(30);  // Default warm hue
    saturationChar = new Characteristic::Saturation(100);
  }

  bool update() override {
    candle_light_on = power->getNewVal();
    candle_brightness = map(bright->getNewVal(), 0, 100, 0, 255);
    candle_hue = hueChar->getNewVal();
    candle_saturation = saturationChar->getNewVal() / 100.0;
    return true;  // No immediate candle animation update; handled in loop
  }
};

// Identify Accessory Class
struct IdentifyAccessory : Service::AccessoryInformation {
  IdentifyAccessory()
    : Service::AccessoryInformation() {
    new Characteristic::Identify();
    new Characteristic::Name("Milly Candle");
    new Characteristic::SerialNumber("01798295933");
    new Characteristic::Model("NeoPixel-Candle-Taline");
    new Characteristic::Manufacturer("Misei.dev");
    new Characteristic::FirmwareRevision("2.0.0");
  }

  bool update() override {
    Serial.println("Accessory identified!");
    for (int i = 0; i < 3; i++) {
      digitalWrite(INFO_LED, HIGH);
      delay(200);
      digitalWrite(INFO_LED, LOW);
      delay(200);
    }
    return true;
  }
};

// Fire Animation for Candle
void updateCandleLight() {
  digitalWrite(CANDLE_PIN, HIGH);

  // Flicker effect for each pixel
  for (int i = 0; i < NUMPIXELS; i++) {
    // Adjust flicker intensity by changing the brightness range
    uint8_t flicker = random(candle_brightness / 3, candle_brightness);  // More intense flicker range
    // Add larger hue variation for more dynamic color changes
    uint8_t r, g, b;
    hsbToRgb(candle_hue + random(-15, 15), candle_saturation, flicker / 255.0, r, g, b);
    strip.setPixelColor(i, strip.Color(r, g, b));
  }

  strip.show();
  
  // Adjust flicker speed with delay
  delay(50);  // Increase delay for slower flicker; decrease for faster flicker
}

// Static Color Update for NeoPixel
void updateNeoPixel() {
  uint8_t r, g, b;
  hsbToRgb(pixel_hue, pixel_saturation, pixel_brightness / 255.0, r, g, b);
  strip.fill(strip.Color(r, g, b));
  strip.show();
}

// Setup and Loop
void setup() {
  Serial.begin(115200);
  pinMode(INFO_LED, OUTPUT);
  pinMode(CANDLE_PIN, OUTPUT);

  // Initialize NeoPixel
  strip.begin();
  strip.clear();
  strip.show();

  // Initialize HomeSpan
  homeSpan.setStatusPin(INFO_LED);
  homeSpan.setPairingCode("11119999");
  homeSpan.setWifiCredentials("Misei", "NaiveTardis");
  homeSpan.begin(Category::Lighting, "Milly Candle");

  // Add Accessories
  new SpanAccessory();
  new IdentifyAccessory();
  new NeoLight();
  new CandleLight();
}

void loop() {
  homeSpan.poll();

  // Continuously update animations
  if (candle_light_on) {
    updateCandleLight();
  } else if (pixel_light_on) {
    digitalWrite(CANDLE_PIN, LOW);
    updateNeoPixel();
  } else {
    digitalWrite(CANDLE_PIN, LOW);
    strip.clear();
    strip.show();
    return;
  }
}