// BatteryCheck.h

#ifndef _BATTERYCHECK_h
#define _BATTERYCHECK_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class BatteryCheck {
private:

public:
	long readVcc();
};

#endif

