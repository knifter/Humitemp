#include <Arduino.h>
#include <SHT3X.h>

#include "config.h"
#include "globals.h"
#include "settings.h"
#include "pid.h"
#include "screens.h"

#include "tools-log.h"

void halt(const char*);
void loop_measure();

void setup()
{
	Serial.begin(115200);

    // Init IO
    digitalWrite(PIN_HB1_A, LOW);
    digitalWrite(PIN_HB1_B, LOW);
    digitalWrite(PIN_HB2_A, LOW);
    digitalWrite(PIN_HB2_B, LOW);
  	pinMode(PIN_HB1_A, OUTPUT);
  	pinMode(PIN_HB1_B, OUTPUT);
  	pinMode(PIN_HB2_A, OUTPUT);
  	pinMode(PIN_HB2_B, OUTPUT);
	digitalWrite(PIN_SPEAKER, LOW);
	pinMode(PIN_SPEAKER, OUTPUT);

	pinMode(PIN_BTN_A, INPUT);
	pinMode(PIN_BTN_B, INPUT);
	pinMode(PIN_BTN_C, INPUT);

    // while(1)
    // {
    //     static bool tmp;
    //     digitalWrite(PIN_HB2_B, tmp=!tmp);
    //     delay(500);
    // };

	gui.begin();
    ScreenPtr scr = std::make_shared<BootScreen>(gui);
    gui.pushScreen(scr);

	Wire.begin(PIN_SDA, PIN_SCL);
	if(!sht_sensor.begin())
	{
		halt("SHT3X sensor error");
	};

	setman.begin();
	pid1.begin();
	pid1.set_tuning(setman.settings.pid1);
	pid2.begin();
	pid2.set_tuning(setman.settings.pid2);

	// gui.showMessage("Test", "a very long text this is not.\nBut long enough?");
};

event_t scan_keys()
{
	// Read current states
	uint32_t pressed = KEY_NONE;
	if(digitalRead(PIN_BTN_A) == LOW)
		pressed |= KEY_A;
	if(digitalRead(PIN_BTN_B) == LOW)
		pressed |= KEY_B;
	if(digitalRead(PIN_BTN_C) == LOW)
		pressed |= KEY_C;
	// if(digitalRead(PIN_POWERINT) == LOW)
	// 	pressed |= KEY_P;
	return (event_t) keytool_get_event(pressed);
};

void loop()
{
	event_t e = scan_keys();
	if(e)
		gui.handle(e);

	setman.loop();
	pid1.loop();
	pid2.loop();
	gui.loop();
};

extern LGFX _lgfx;
void halt(const char* error)
{
	DBG("HALT: %s", error);
	_lgfx.fillScreen(TFT_RED);
	_lgfx.setTextSize(3);
	_lgfx.setTextColor(TFT_WHITE, TFT_RED);
	_lgfx.setCursor(5, 5);
	_lgfx.print(error);
	while(1);
};
