#include "CYD28_TouchscreenR.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include "hal/bright/bright.h"
#include "hal/device.h"
#include "hal/inputs/touch.h"
#include <interface.h>

#define BTN_ACT LOW
#define BTN_PIN 0

extern CYD28_TouchR touch; // defined by hal/inputs/touch.cpp

static DeviceTouch touchCfg() {
    DeviceTouch cfg;
    // XPT2046 orientation tables per rotation (0,1,2,3)
    const bool swapXY[4] = {true, false, true, false};
    const bool mirrorX[4] = {true, false, false, true};
    const bool mirrorY[4] = {false, false, true, true};
    for (int i = 0; i < 4; i++) {
        cfg.SwapXY[i] = swapXY[i];
        cfg.MirrorX[i] = mirrorX[i];
        cfg.MirrorY[i] = mirrorY[i];
    }
    return cfg;
}

void _setup_gpio() {
    bruceConfigPins.rotation = 0; // portrait (handheld); change in Settings if flipped

    // --- IR ---
    bruceConfigPins.irTx = 8;

    // --- I2C (no device wired; GPIO8/9 are IR-TX / battery here) ---
    bruceConfigPins.i2c_bus = {(gpio_num_t)-1, (gpio_num_t)-1}; // sda, scl = none

    // --- one-pin RF: unused, CC1101 handles SubGHz ---
    bruceConfigPins.rfTx = -1;
    bruceConfigPins.rfRx = -1;

    // --- UART / GPS (console is on USB-CDC, so 43/44 are free) ---
    bruceConfigPins.uart_bus = {(gpio_num_t)44, (gpio_num_t)43}; // rx, tx
    bruceConfigPins.gps_bus = {(gpio_num_t)44, (gpio_num_t)43};  // rx, tx

    // --- BadUSB over native USB-HID (no serial target pins) ---
    bruceConfigPins.badusb_bus = {(gpio_num_t)-1, (gpio_num_t)-1};

    // --- Generic SPI slot (no RC522/PN532 wired) : sck,miso,mosi,cs ---
    bruceConfigPins.outer_bus = {(gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)-1};
    bruceConfigPins.PN532_bus = {(gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)-1};

    // --- CC1101 SubGHz : sck,miso,mosi,cs,gdo0 ---
    bruceConfigPins.CC1101_bus = {
        (gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)18, (gpio_num_t)39
    };

    // --- nRF24 #1 : sck,miso,mosi,cs,ce  (stock Bruce = 1 module only) ---
    bruceConfigPins.NRF24_bus = {
        (gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)10, (gpio_num_t)14
    };

    // --- SD card : sck,miso,mosi,cs (shares HSPI) ---
    bruceConfigPins.SDCARD_bus = {
        (gpio_num_t)12, (gpio_num_t)13, (gpio_num_t)11, (gpio_num_t)17
    };

    // Keep XPT2046 CS high until needed
    pinMode(XPT2046_SPI_CONFIG_CS_GPIO_NUM, OUTPUT);
    digitalWrite(XPT2046_SPI_CONFIG_CS_GPIO_NUM, HIGH);

    bruceConfig.colorInverted = 1; // if colors look wrong, toggle in Settings
}

void _post_setup_gpio() {
    if (!hal_touch_init(touchCfg(), 0, TFT_MOSI == CYD28_TouchR_MOSI)) {
        Serial.println("[TOUCH] XPT2046 not started");
    } else {
        Serial.println("[TOUCH] XPT2046 started OK");
    }
    hal_bright_attach(TFT_BL);
    hal_bright_set(TFT_BL, 100);
}

void _setBrightness(uint8_t brightval) { hal_bright_set(TFT_BL, brightval); }

void InputHandler(void) {
    static long d_tmp = 0;
    if (millis() - d_tmp > 200 || LongPress) {
        BruceTouchPoint t;
        if (hal_touch_read(touchCfg(), t)) {
            d_tmp = millis();
            hal_touch_apply(t);
        }
    }
    checkPowerSaveTime();
    if (digitalRead(BTN_PIN) == BTN_ACT) {
        if (!wakeUpScreen()) AnyKeyPress = true;
        SelPress = true;
        long tmp = millis();
        while ((millis() - tmp) < 200 && digitalRead(BTN_PIN) == BTN_ACT);
    }
}

void powerOff() { esp_deep_sleep_start(); }
void goToDeepSleep() { esp_deep_sleep_start(); }
void checkReboot() {}
