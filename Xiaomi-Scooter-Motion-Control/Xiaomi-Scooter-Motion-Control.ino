#include <SPI.h>
#include <Arduino.h>
#include <arduino-timer.h>
#include <mcp4xxx.h>


using namespace icecave::arduino;

MCP4XXX* pot;

auto timer_m = timer_create_default();



//int ThrottlePin = 13;
int BrakeHandle;
int Speed; //current speed
int oldspeed; //speed during last loop
int trend = 0; //speed trend


// Boost timer, how long the motor will be powered after a kick. time in miliseconds.
int boosttimer = 6000; 
int kickdelay = 500; //time before you can do an new kick

//motionmodes
uint8_t motionstate = 0;
#define motionready 0
#define motionbusy 1
#define motionbreaking 2




void logByteInHex(uint8_t val)
{
//	if(val < 16)
//		Serial.print('0');
//
//	Serial.print(val, 16);
//	Serial.print(' ');
}

uint8_t readBlocking()
{
	while(!Serial2.available())
		delay(1);

	return Serial2.read();
}

void setup()
{

    
	Serial.begin(115200);
	Serial2.begin(115200);

	Serial.println("Starting Logging data...");

  pot = new MCP4XXX(49); //CS PIN
  
  ThrottleWrite(45);
}

uint8_t buff[256];
void loop()
{

  int w = 0;

  
  
	while(readBlocking() != 0x55);
	if(readBlocking() != 0xAA)
		return;

	uint8_t len = readBlocking();
	buff[0] = len;
	if(len > 254)
		return;

	uint8_t addr = readBlocking();
	buff[1] = addr;

	uint16_t sum = len + addr;
	for(int i = 0; i < len; i++)
	{
		uint8_t curr = readBlocking();
		buff[i + 2] = curr;
		sum += curr;
	}


	uint16_t checksum = (uint16_t)readBlocking() | ((uint16_t)readBlocking() << 8);
	if(checksum != (sum ^ 0xFFFF))
		return;

	for(int i = 0; i < len + 2; i++)
		logByteInHex(buff[i]);

//	Serial.print("check ");
//	Serial.print(checksum, 16);
//
//	Serial.println();
switch (buff[1]) {
  case 0x20:
    switch (buff[2]) {
      case 0x65:
        BrakeHandle = buff[6];
    }
   case 0x21:
    switch (buff[2]) {
      case 0x64:
        Speed = buff[8];
    } 
}

Serial.print("Speed: ");
Serial.print(Speed);
Serial.print(" Brake: ");
Serial.print(BrakeHandle);
Serial.print(" State: ");
Serial.print(motionstate);
Serial.println(" ");

  motion_control();
  timer_m.tick();


}


bool release_throttle(void *) {
  Serial.println("Timer expired, stopping...");
  ThrottleWrite(45); //Keep throttle open for 10% (100) to disable KERS. 
  timer_m.in(kickdelay, motion_wait);
  return false; // false to stop
}

bool motion_wait(void *) {
  Serial.println("Ready for new kick!");
  motionstate = motionready;
  return false; // false to stop
}

void motion_control() {



      if ((Speed != 0) && (Speed < 5)) {
        // If speed is under 5 km/h, stop throttle
        ThrottleWrite(45); //  0% throttle
        
      }

  if (BrakeHandle > 46) {

    ThrottleWrite(45); //close throttle directly when break is touched. 0% throttle
    Serial.println("BRAKE detected!!!");
    motionstate = motionready;
    timer_m.cancel();

  }


if (Speed != 0){
          // Check if auto throttle is off and speed is increasing
  if (motionstate == motionready) {
    if (Speed != oldspeed)
    {
      trend = Speed - oldspeed;
    }

    if (trend > 0)
    {
      // speed is increasing
      // Check if speed is at least 5 km/h
      if (Speed > 5) {
        // Open throttle for 5 seconds
        ThrottleWrite(214); //  100% throttle
        Serial.println("Kick detected!");
        timer_m.in(boosttimer, release_throttle); //Set timer to release throttle
        motionstate = motionbusy;

      }

    }
    else if (trend < 0)
    {
      // speed is decreasing

    }
    else
    {     
      // no change in speed
    }

  }

  oldspeed = Speed;
      }

}

int ThrottleWrite(int value)
{
  if (value != 0){
      pot->set(value);
      Serial.print(value);
  }

}
