#include <EEPROM.h>
#include <Event.h>
#include <Timer.h>
#include <Pushbutton.h>
#include <ResponsiveAnalogRead.h>
#include <bitswap.h>
#include <chipsets.h>
#include <color.h>
#include <colorpalettes.h>
#include <colorutils.h>
#include <controller.h>
#include <cpp_compat.h>
#include <dmx.h>
#include <FastLED.h>
#include <fastled_config.h>
#include <fastled_delay.h>
#include <fastled_progmem.h>
#include <fastpin.h>
#include <fastspi.h>
#include <fastspi_bitbang.h>
#include <fastspi_dma.h>
#include <fastspi_nop.h>
#include <fastspi_ref.h>
#include <fastspi_types.h>
#include <hsv2rgb.h>
#include <led_sysdefs.h>
#include <lib8tion.h>
#include <noise.h>
#include <pixelset.h>
#include <pixeltypes.h>
#include <platforms.h>
#include <power_mgt.h>
//AmbienceNG.ino

#define AMB_COLOR_ENABLE_BUTTON_PIN 	2
#define AMB_SELECT_STRIP_BUTTON_PIN 	3

#define AMB_STRIP0_PIN					4
#define AMB_STRIP0_LEDS					9

#define AMB_STRIP1_PIN					5
#define AMB_STRIP1_LEDS					16

#define AMB_STRIP2_PIN					6
#define AMB_STRIP2_LEDS					0

#define AMB_STRIP3_PIN					7
#define AMB_STRIP3_LEDS					0

#define AMB_ANALOG_POT_RED				A0
#define AMB_ANALOG_POT_GREEN			A1
#define AMB_ANALOG_POT_BLUE				A2

#define AMB_SELECTED_STRIP_BLINKS		4
#define AMB_ACTIVE_STRIPS				2

struct __defs
{
	int pin;
	int leds;
};

struct __state{
	CRGB strip0[AMB_STRIP0_LEDS];
	CRGB strip1[AMB_STRIP1_LEDS];
	CRGB strip2[AMB_STRIP2_LEDS];
	CRGB strip3[AMB_STRIP3_LEDS];
} ambienceState;

CRGBSet strip0Set(ambienceState.strip0, AMB_STRIP0_LEDS);
CRGBSet strip1Set(ambienceState.strip1, AMB_STRIP1_LEDS);
CRGBSet strip2Set(ambienceState.strip2, AMB_STRIP2_LEDS);
CRGBSet strip3Set(ambienceState.strip3, AMB_STRIP3_LEDS);

void _SerialPrintf(const char *fmt, ...);
#define _p(fmt, ...) _SerialPrintf(PSTR(fmt), ##__VA_ARGS__)
#define _d(fmt, ... ) _SerialPrintf(PSTR("%s(): " fmt "\n"), __FUNCTION__, ##__VA_ARGS__)

Pushbutton colorEnableBtn(AMB_COLOR_ENABLE_BUTTON_PIN);
Pushbutton selectStripBtn(AMB_SELECT_STRIP_BUTTON_PIN);

ResponsiveAnalogRead analogPotRed(AMB_ANALOG_POT_RED, false);
ResponsiveAnalogRead analogPotGreen(AMB_ANALOG_POT_GREEN, false);
ResponsiveAnalogRead analogPotBlue(AMB_ANALOG_POT_BLUE, false);
Timer timer;
static byte red,green,blue;
static unsigned long storedChecksum = -1;
static int currentStrip = -1;
static int blinkTimerEvent;
static byte selectedStripBlink;
static byte r,g,b;
static void updateStripColor();
static void selectNextStrip();
static void blinkSelectedStrip();
static CRGBSet &getStrip(byte strip = currentStrip);
//static int getNumLeds(byte strip = currentStrip);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  _d ("start");
  _d ("add strip 0");
  FastLED.addLeds<NEOPIXEL, AMB_STRIP0_PIN>(ambienceState.strip0, AMB_STRIP0_LEDS);
  _d ("add strip 1");
  FastLED.addLeds<NEOPIXEL, AMB_STRIP1_PIN>(ambienceState.strip1, AMB_STRIP1_LEDS);

  if (AMB_STRIP2_LEDS > 0)
  {
  	_d ("add strip 2");
  	FastLED.addLeds<NEOPIXEL, AMB_STRIP2_PIN>(ambienceState.strip2, AMB_STRIP2_LEDS);
  }

  if (AMB_STRIP2_LEDS > 0)
  {
  	_d ("add strip 3");
  	FastLED.addLeds<NEOPIXEL, AMB_STRIP3_PIN>(ambienceState.strip3, AMB_STRIP3_LEDS);
  }

  EEPROM.get(0,ambienceState);
  // strip0Set.fill_solid(0x1f1f1f);
  // strip1Set.fill_solid(0x1f1f1f);

  FastLED.show();

  _d ("done");
}

void loop() 
{
	updatePots();
	timer.update();

	if (colorEnableBtn.isPressed())
	{
		updateStripColor();
	}

	if (selectStripBtn.getSingleDebouncedPress())
	{
		selectNextStrip();
	}

}

void updatePots()
{
	analogPotRed.update();
	analogPotGreen.update();
	analogPotBlue.update();
}

void updateStripColor()
{
	_d("enter");
	if(analogPotRed.hasChanged()
		|| analogPotGreen.hasChanged()
		|| analogPotBlue.hasChanged()) 
	{
		r = map(analogPotRed.getValue(), 0, 1024, 0, 255);
		g = map(analogPotGreen.getValue(), 0, 1024, 0, 255);
		b = map(analogPotBlue.getValue(), 0, 1024, 0, 255);

		_d("r = %d, g = %d, b = %d", r,g,b);

		getStrip() = CRGB(r,g,b);

		EEPROM.put (0, ambienceState);
		FastLED.show();
	}
}

void selectNextStrip()
{
	_d("enter");
	
	if (++currentStrip == AMB_ACTIVE_STRIPS)
			currentStrip = -1;

	timer.stop(blinkTimerEvent);
	blinkTimerEvent = timer.every(250, blinkSelectedStrip);
	_d("end");
}

void blinkSelectedStrip()
{
	_d("execute, strip: %d", currentStrip);

	(selectedStripBlink % 2) ? (getStrip() *= 4) : (getStrip() /= 4);

	FastLED.show();
	
	if (++selectedStripBlink == AMB_SELECTED_STRIP_BLINKS)
	{
		_d("end blinking");
		selectedStripBlink = 0;
		timer.stop(blinkTimerEvent);
	}
}

CRGBSet &getStrip(byte strip)
{
	switch (strip)
	{
		case 0:
			return (strip0Set);
		case 1:
			return (strip1Set);
		case 2:
			return (strip2Set);
		case 3:
			return (strip3Set);
		default:
			break;
	}

	return (strip0Set);
}

int getNumLeds(byte strip)
{
	switch (strip)
	{
		case 0:
			return (AMB_STRIP0_LEDS);
		case 1:
			return (AMB_STRIP1_LEDS);
		case 2:
			return (AMB_STRIP2_LEDS);
		case 3:
			return (AMB_STRIP3_LEDS);
		default:
			break;
	}
	return (AMB_STRIP0_LEDS);
}