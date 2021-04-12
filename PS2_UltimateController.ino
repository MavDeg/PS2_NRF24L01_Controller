#include <PS2X_lib.h>
#include <RF24_config.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <SPI.h>
#include "BatteryCheck.h"

/*-------------------Preprocessors-----------------------*/
//RF24
#define  PIN_07_RF_CE	7
#define  PIN_08_RF_CSN	8
#define  RF_PAYLOAD_SIZE_10	10

//PS2X
#define PIN_02_PS2X_ATT  2
#define PIN_03_PS2X_COM  3
#define PIN_04_PS2X_DTA  4
#define PIN_05_PS2X_CLK  5

/*--------Variables/Class Declarations-------------------*/

//check battery for undervoltage
BatteryCheck battCheck;

//NRF24
RF24 radio(PIN_07_RF_CE, PIN_08_RF_CSN);
const uint64_t pipe = 0xE8E8F0F0E1LL; // Define the transmit pipe, NOTE: the "LL" at the end of the constant is "LongLong" type
unsigned int rx_status[2] = { 0, 0};

//PS2X
PS2X ps2x;	
int ps2x_ErrorCheck = 0;	
char vibrate = 0;

//analogs
typedef struct analogs {
	unsigned int leftX;
	unsigned int leftY;
	unsigned int rghtX;
	unsigned int rghtY;
};

//digitals
typedef union digitals{
	struct {
		unsigned int l1 : 1;
		unsigned int l2 : 1;
		unsigned int l3 : 1;
		unsigned int r1 : 1;
		unsigned int r2 : 1;
		unsigned int r3 : 1;
		unsigned int up : 1;
		unsigned int dn : 1;
		unsigned int lf : 1;
		unsigned int rt : 1;
		unsigned int tr : 1;
		unsigned int cr : 1;
		unsigned int sq : 1;
		unsigned int cl : 1;
		unsigned int sl : 1;
		unsigned int st : 1;
	}bits;
	unsigned int onoff;
} ;

struct keys {
	analogs analogKeys;
	digitals digitalKeys;
};

keys keyValues;

void setup()
{
	// enable serial monitor for debugging
	Serial.begin(115200);

	//RF24
	radio.begin();
	radio.setAutoAck(true);		
	radio.enableAckPayload();
	radio.enableDynamicPayloads();
	//radio.setPayloadSize(sizeof(keyValues)+1);
	radio.setPALevel(RF24_PA_LOW); //try to set to RF24_PA_LOW if there are some issues, RF24_PA_MAX is the default
	radio.stopListening();
	radio.openWritingPipe(pipe);
	radio.setRetries(5, 5);

	//PS2X setup
	ps2x_ErrorCheck = ps2x.config_gamepad(PIN_05_PS2X_CLK, PIN_03_PS2X_COM,
		PIN_02_PS2X_ATT, PIN_04_PS2X_DTA, true, true);   //setup pins and settings:  GamePad(clock, command, attention, data, Pressures?, Rumble?) check for error
	switch(ps2x_ErrorCheck){
	case 0:
		Serial.println("Found Controller, configured successful");
		Serial.println("Try out all the buttons, X will vibrate the controller, faster as you press harder;");
		Serial.println("holding L1 or R1 will print out the analog stick values.");
		Serial.println("Go to www.billporter.info for updates and to report bugs.");
		break;
	case 1:
		Serial.println("No controller found, check wiring, see readme.txt to enable debug. visit www.billporter.info for troubleshooting tips");
		break;
	case 2:
		Serial.println("Controller found but not accepting commands. see readme.txt to enable debug. Visit www.billporter.info for troubleshooting tips");
		break;
	case 3:
	default:
		Serial.println("Controller refusing to enter Pressures mode, may not support it. ");
		break;
	}

	memset(&keyValues, 0, sizeof(keyValues));  // intialize struct to 0
}

void loop()
{
	// PS2X read gamepad values:
	gameController_Reading();
	//RF24
	if (radio.write(&keyValues, sizeof(keyValues)) )
	{
		Serial.println("...tx success");
		if (radio.isAckPayloadAvailable())
		{
			radio.read(rx_status, sizeof(rx_status));
			Serial.print("received ack payload is : ");
			Serial.println(rx_status[0]);
		}
		else
		{
			Serial.println("status has become false so stop here....");
		}
	}
	//delay(50);
	//ps2x.read_gamepad(200, 200);          //read controller and set large motor to spin at 'vibrate' speed
	Serial.println(battCheck.readVcc(), DEC);

}

//Read the controllers inputs
void gameController_Reading()
{
	ps2x.read_gamepad(false, false);  //read controller but set rumble motor to off

	//analog readings
	keyValues.analogKeys.leftX = ps2x.Analog(PSS_LX);	// left analog stick
	keyValues.analogKeys.leftY = ps2x.Analog(PSS_LY);
	keyValues.analogKeys.rghtX = ps2x.Analog(PSS_RX);	// right analog stick
	keyValues.analogKeys.rghtY = ps2x.Analog(PSS_RY);

	// digital readings
	if (ps2x.NewButtonState()) {
		// use masking to toggle the value
		if (ps2x.ButtonPressed(PSB_L1))			keyValues.digitalKeys.bits.l1 = toggleButton(0x0001);
		if (ps2x.ButtonPressed(PSB_L2))			keyValues.digitalKeys.bits.l2 = toggleButton(0x0002);
		if (ps2x.ButtonPressed(PSB_L3))			keyValues.digitalKeys.bits.l3 = toggleButton(0x0004);

		if (ps2x.ButtonPressed(PSB_R1))			keyValues.digitalKeys.bits.r1 = toggleButton(0x0008);
		if (ps2x.ButtonPressed(PSB_R2))			keyValues.digitalKeys.bits.r2 = toggleButton(0x0010);
		if (ps2x.ButtonPressed(PSB_R3))			keyValues.digitalKeys.bits.r3 = toggleButton(0x0020);

		if (ps2x.ButtonPressed(PSB_PAD_UP))		keyValues.digitalKeys.bits.up = toggleButton(0x0040);
		if (ps2x.ButtonPressed(PSB_PAD_DOWN))	keyValues.digitalKeys.bits.dn = toggleButton(0x0080);
		if (ps2x.ButtonPressed(PSB_PAD_LEFT))	keyValues.digitalKeys.bits.lf = toggleButton(0x0100);
		if (ps2x.ButtonPressed(PSB_PAD_RIGHT))	keyValues.digitalKeys.bits.rt = toggleButton(0x0200);

		if (ps2x.ButtonPressed(PSB_TRIANGLE))	keyValues.digitalKeys.bits.tr = toggleButton(0x0400);
		
		if (ps2x.ButtonPressed(PSB_CROSS))		keyValues.digitalKeys.bits.cr = toggleButton(0x0800);
		if (ps2x.ButtonReleased(PSB_CROSS))		keyValues.digitalKeys.bits.cr = toggleButton(0x0800);

		if (ps2x.ButtonPressed(PSB_SQUARE))		keyValues.digitalKeys.bits.sq = toggleButton(0x1000);
		if (ps2x.ButtonPressed(PSB_CIRCLE))		keyValues.digitalKeys.bits.cl = toggleButton(0x2000);

		if (ps2x.ButtonPressed(PSB_SELECT))		keyValues.digitalKeys.bits.sl = toggleButton(0x4000);
		if (ps2x.ButtonPressed(PSB_START))		keyValues.digitalKeys.bits.st = toggleButton(0x8000);
	}
}

int toggleButton(unsigned int mask) {
	if ((keyValues.digitalKeys.onoff & mask) == 0) {
		return 1;
	}
	else {
		return 0;
	}
}