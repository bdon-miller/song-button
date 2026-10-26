#include "hal_power.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_random.h>
#include <esp_sleep.h>

#include "config.h"

// Digital pads parked through deep sleep, with their sleep level. The amp pin is handled
// separately by ampEnable(), which sets its level before releasing the hold.
struct ParkedPin {
  uint8_t pin;
  uint8_t level;
};
static const ParkedPin kParked[] = {
    {PIN_NEOPIXEL_POWER, LOW},  // pixel unpowered
    {PIN_SD_CS, HIGH},          // card deselected
    {PIN_SD_MOSI, HIGH},
    {PIN_SD_SCK, LOW},
    {PIN_I2S_BCLK, LOW},
    {PIN_I2S_LRC, LOW},
    {PIN_I2S_DOUT, LOW},
};

void powerBegin() {
  // Release last sleep's holds before SPI.begin / setPinout reconfigure these pads.
  for (const ParkedPin& p : kParked) gpio_hold_dis((gpio_num_t)p.pin);
  pinMode(PIN_NEOPIXEL_POWER, OUTPUT);
  digitalWrite(PIN_NEOPIXEL_POWER, LOW);
  // Cold power-up has no sleep hold on the amp pin, so drive it off until ampEnable(true).
  // After a deep-sleep wake the pad is still held LOW and this write is a no-op.
  pinMode(PIN_AMP_ENABLE, OUTPUT);
  digitalWrite(PIN_AMP_ENABLE, LOW);
}

void ampEnable(bool on) {
  pinMode(PIN_AMP_ENABLE, OUTPUT);
  digitalWrite(PIN_AMP_ENABLE, on ? HIGH : LOW);
  gpio_hold_dis((gpio_num_t)PIN_AMP_ENABLE);  // release last sleep's hold only after the level is set
}

bool buttonPressedEdge() {
  static bool inited = false;
  static bool stable = HIGH;       // debounced level
  static bool lastRaw = HIGH;
  static uint32_t changedAt = 0;
  if (!inited) {
    // ext0 wake left the pad routed to the RTC mux; hand it back to the digital GPIO matrix.
    rtc_gpio_deinit((gpio_num_t)PIN_BUTTON);
    gpio_hold_dis((gpio_num_t)PIN_BUTTON);
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    stable = lastRaw = digitalRead(PIN_BUTTON);  // a button already held at boot is not an edge
    inited = true;
    return false;
  }
  bool raw = digitalRead(PIN_BUTTON);
  uint32_t now = millis();
  if (raw != lastRaw) {
    lastRaw = raw;
    changedAt = now;
    return false;
  }
  if (raw != stable && now - changedAt >= DEBOUNCE_MS) {
    stable = raw;
    return stable == LOW;
  }
  return false;
}

[[noreturn]] void deepSleep() {
  pinMode(PIN_AMP_ENABLE, OUTPUT);
  digitalWrite(PIN_AMP_ENABLE, LOW);
  gpio_hold_en((gpio_num_t)PIN_AMP_ENABLE);
  for (const ParkedPin& p : kParked) {
    pinMode(p.pin, OUTPUT);
    digitalWrite(p.pin, p.level);
    gpio_hold_en((gpio_num_t)p.pin);
  }
  gpio_deep_sleep_hold_en();
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BUTTON, 0);
  rtc_gpio_pullup_en((gpio_num_t)PIN_BUTTON);
  rtc_gpio_pulldown_dis((gpio_num_t)PIN_BUTTON);
  esp_deep_sleep_start();
}

uint32_t rngU32() { return esp_random(); }
