#include <EEPROM.h>
#include <ResponsiveAnalogRead.h>
#include <Event.h>
#include <Timer.h>
#include <Adafruit_NeoPixel.h>
//Ambience.ino
#define SET_BUTTON_PIN 6

ResponsiveAnalogRead anal0(A0, false);
ResponsiveAnalogRead anal1(A1, false);
ResponsiveAnalogRead anal2(A2, false);
Timer t;
byte r,g,b;
unsigned long storedCRC = -1;

bool updateStrip[4] = { false, false, false, false };

struct stripState {
	byte r = 0;
	byte g = 0;
	byte b = 0;
};

struct stripsState {
	stripState strip[4];
} allStrips;

byte pixelsInStrip[4] = 
{ 	
	9,  /* STRIP 0 pixels on pin 2*/
	0, 
	0, 
	0 
};

Adafruit_NeoPixel neoStrip[4] = {
	Adafruit_NeoPixel(pixelsInStrip[0], 2, NEO_GRB + NEO_KHZ800),
	Adafruit_NeoPixel(pixelsInStrip[1], 3, NEO_GRB + NEO_KHZ800),
	Adafruit_NeoPixel(pixelsInStrip[2], 4, NEO_GRB + NEO_KHZ800),
	Adafruit_NeoPixel(pixelsInStrip[3], 5, NEO_GRB + NEO_KHZ800),
};

void setup() {
  Serial.begin(9600);
  Serial.println("Ambience start");
  
  pinMode(SET_BUTTON_PIN, INPUT); 
  digitalWrite(SET_BUTTON_PIN, HIGH);

  for (int strip=0; strip<4; strip++)
  {
  	neoStrip[strip].begin();
  	neoStrip[strip].setPixelColor(0, neoStrip[strip].Color(32,0,0));
  	neoStrip[strip].show();
  }

  EEPROM.get(0, allStrips);
  EEPROM.get(sizeof(stripsState), storedCRC);

  if (eeprom_crc(0, sizeof(stripsState)) == storedCRC)
  {
  	Serial.println ("Got valid EEPROM crc, use it");
  	for (int strip=0; strip<4; strip++)
	{
		for (int pixel=0; pixel<pixelsInStrip[strip]; pixel++)
			neoStrip[strip].setPixelColor(pixel, neoStrip[strip].Color(allStrips.strip[strip].r, allStrips.strip[strip].g, allStrips.strip[strip].b));

		neoStrip[strip].show();
	}
  }

  t.every(25, timerCallback);
}

void loop() {
  // update the ResponsiveAnalogRead object every loop
  t.update();
  	anal0.update();
  	anal1.update();
  	anal2.update();
}

unsigned long eeprom_crc(int start, int end) {

  const unsigned long crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };

  unsigned long crc = ~0L;

  for (int index = start ; index < end  ; ++index) {
    crc = crc_table[(crc ^ EEPROM[index]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (EEPROM[index] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  return crc;
}

void timerCallback()
{
	if (digitalRead(SET_BUTTON_PIN) == HIGH)
		return;

	if(anal0.hasChanged()) {
		r = anal0.getValue() / 4;
		updateStrip[0] = true;
	}

	if(anal1.hasChanged()) {
		g = anal1.getValue() / 4;
		updateStrip[0] = true;
	}

	if(anal2.hasChanged()) {
		b = anal2.getValue() / 4;
		updateStrip[0] = true;
	}

	for (int strip=0; strip<4; strip++)
	{
		if (updateStrip[strip] == true) {
			for (int pixel=0; pixel<pixelsInStrip[strip]; pixel++)
				neoStrip[strip].setPixelColor(pixel, neoStrip[strip].Color(r,g,b));

			Serial.print(r); Serial.print(",");
			Serial.print(g); Serial.print(",");
			Serial.print(b); Serial.print(",");
			Serial.println("");
			Serial.println("-----");
			Serial.println("");

			neoStrip[strip].show();

			allStrips.strip[strip].r = r;
			allStrips.strip[strip].g = g;
			allStrips.strip[strip].b = b;

			EEPROM.put(0, allStrips);
			EEPROM.put(sizeof(stripsState), eeprom_crc(0, sizeof(stripsState)));
		}

		updateStrip[strip] = false;
	}
}