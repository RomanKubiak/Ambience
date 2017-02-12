#include <stdio.h>
#include <Arduino.h>

extern "C"
{
	int serialputc(char c, FILE *fp)
 	{
 		if(c == '\n')
       		Serial.write('\r'); 
   		Serial.write(c);
   		return (0); 
 	}
}

void _SerialPrintf(const char *fmt, ...)
{
	FILE stdiostr;
	va_list ap;

	fdev_setup_stream(&stdiostr, serialputc, NULL, _FDEV_SETUP_WRITE);

	va_start(ap, fmt);
	vfprintf_P(&stdiostr, fmt, ap);
	va_end(ap);
}