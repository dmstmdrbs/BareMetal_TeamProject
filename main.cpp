#include "mbed.h"
#include "hcsr04.h"
#include "Adafruit_SSD1306.h"
#include "motordriver.h"
#include "DHT22.h"

#define COOLER 1
#define HEAT 2
#define MAX_RANGE 0.00125
#define MIN_RANGE 0.00333

Motor mt(D11,PC_8); // Motor
PwmOut sound(PC_9); //Buzzer
DHT22 sensor (PB_2); //Temp & Humid
//Joystick
AnalogIn x_axis (PC_2);
AnalogIn y_axis (PC_3);

//Button
DigitalIn left_button(PA_14); //left button
DigitalIn center_button(PB_7); // center button
DigitalIn right_button(PC_4);// right button

//LED
DigitalOut left_led(PA_13); //left led
DigitalOut center_led(D6); //center led
DigitalOut right_led(PA_4); //right led

//RGB
DigitalOut Red(A1); //RGB Red
DigitalOut Green(PC_6); //RGB Green
DigitalOut Blue(A3); //RGB Blue

void config();
void ultrasonic();
void display();
void SYSTEM_OFF();
void SYSTEM_ON();
void WIND_ON(int wind_power);
void WIND_OFF();
void timeout_SYSTEM_OFF();
void BUZZWER_WARNING();
void check_temp_and_humid();
void joystick_handler();
void WARNING_MAX();
void WARNING_MIN();

class I2CPreInit : public I2C{
   public:
   I2CPreInit(PinName sda, PinName scl) : I2C(sda, scl){
      frequency(400000);
      start();
   };
};

Serial pc(USBTX,USBRX);
//display
I2C myI2C(I2C_SDA,I2C_SCL);
Adafruit_SSD1306_I2c myGUI(myI2C, D13, 0x78, 64, 128);
// ultrasonic pin
DigitalOut trig_pin(D10);
DigitalIn echo_pin(D7);
HCSR04 hcsr(D10, D7); 

//Ticker display_ticker;
int ticker_count=0;
Timeout myTimeout;
Ticker display_ticker;
int x, y, debug_print;

bool ultrasonic_timeout_flag = false;
bool sys_on=false;
int mode=COOLER; //COOLER, HEAT => default COOLER
float current_temp, desired_temp=18.0; //default = 18.0; 
float humid;
int wind_power=2; // 1~5
bool automatic=true;

int main(){
   config();

   while(1)
   {
		 	if(!right_button){
				SYSTEM_ON();
			}
      while(sys_on){
        ultrasonic();
				while(1){
					x = x_axis.read() * 1000;
					y = y_axis.read() * 1000;
					//debug_print = 1;
					if(y>765){
						break;
					}
					else if(x<5&&y<5){

						//temp up
						
						if(desired_temp>=32){
							WARNING_MAX();
						}else{
							desired_temp+=0.5;
						}
						automatic=false;
						center_led=0;
						break;
					}
					else if(x>700&&y<5){
						//wind power up
						if(wind_power==5){
							WARNING_MAX();
						}else{
							wind_power+=1;
						}
						automatic=false;
						center_led=0;
						break;
						
					}
					else if(x<5&&y>700){
						
						//temp down
						if(desired_temp<=16.06){
							WARNING_MIN();
						}else{
							desired_temp-=0.5;
						}
						automatic=false;
						center_led=0;
						break;
						
					}
					else if(x>750&&y>750){
						//wind power down
						
						if(wind_power==1){
							WARNING_MIN();
						}else{
							wind_power-=1;
						}
						automatic=false;
						center_led=0;
						break;
					}		
				}
			 // click SW2 : left button = cooler / heater
			 if(!left_button){
					if(mode==COOLER){
						mode=HEAT;
						left_led=0;
						Red=1;
						Blue=0;
						Green=0;
					}
					else{
						mode=COOLER;
						left_led=1;
						Red=0;
						Blue=1;
						Green=0;
					}
					wait(0.01);
					WIND_ON(wind_power);
			 }
		//click SW9 : center button = auto / manual
			if(!center_button){
					if(automatic)
					{
						 automatic=false;
						 center_led=0;
					}
					else{
						 automatic=true;
						 center_led=1;
					}
			}
			pc.printf("current_temp:%f desired_temp:%f wind_power:%d mode:%d\r\n",current_temp,desired_temp,wind_power,mode);
			
			if(automatic){
			// AUTO MODE
			pc.printf("AUTO MODE\r\n");
				if(mode==COOLER){
					pc.printf("COOLER MODE\r\n");
					//COOLER MODE AREA
					if(current_temp-desired_temp>0){
						if(wind_power<5){
							wind_power++;
							WIND_ON(wind_power);
						}
						else{
							wind_power=5;
							WIND_ON(wind_power);
						}
					} else {
						wind_power=1;
						WIND_ON(wind_power);
					}
				}
				else{
					pc.printf("HEATER MODE\r\n");
					//HEATER MODE AREA
					if(desired_temp-current_temp>0){
						if(wind_power<5){
							wind_power++;
							WIND_ON(wind_power);
						}
						else{
							wind_power=5;
							WIND_ON(wind_power);
						}
					} else {
						wind_power=1;
						WIND_ON(wind_power);
					}
				}
			}else{
			//MANUAL MODE
				pc.printf("MANUAL MODE\r\n");
				WIND_ON(wind_power);
			}

		//click SW 10 : right button = power on/off
			if(!right_button){
					if(sys_on){
						 SYSTEM_OFF();
							break;
					}
			}
			wait(0.1);
		}
		WIND_OFF();
		wait(0.1);
  }
}

void config(){
	pc.baud(9600);
	SYSTEM_OFF();
	myGUI.clearDisplay();
	echo_pin.mode(PullDown); //ultrasonic
	display_ticker.attach(&display,1.0);	
	pc.printf("HAVC SYSTEM\r\n");		
}

void timeout_SYSTEM_OFF(){
    pc.printf("Human not detected!! System off\r\n");
    SYSTEM_OFF();
		ultrasonic_timeout_flag = false;
}

void joystick_handler(){
	x = x_axis.read() * 1000;
	y = y_axis.read() * 1000;
	//debug_print = 1;
	if(x<100){ // left side
		 pc.printf("X=%d, Y=%d \r\n", x, y);
		 desired_temp = float(float(y+1)/15 + 16); // 16.06 to 32.20
		 pc.printf("desired temp = %0.2f\r\n", desired_temp);
		 //display();
		 wait(0.1);
	}
	else if(x>140){ // right side
		 pc.printf("X=%d, Y=%d \r\n", x, y);
		 wind_power = int((y+1)/50)+1; // 1 to 5
		 if(wind_power>=5){
			 WARNING_MAX();
			 wind_power=5;
		 }
		 if(wind_power<=0){
			 WARNING_MIN();
			 wind_power=2;
		 }
		 
		 pc.printf("wind power = %d\r\n", wind_power);
		 //display();
		 wait(0.1);
	}
}


void ultrasonic(){
   // pc.printf("ultrasonic() started...\r\n");
   hcsr.start();
   wait_ms(500);
   int distance = hcsr.get_dist_cm();
   pc.printf("Distance(cm): %d\r\n", distance);
   
   if (distance >= 50 && !ultrasonic_timeout_flag){
      //pc.printf("[1] distance >= 50 && timeout is NOT activated\r\n");
      myTimeout.attach(&timeout_SYSTEM_OFF, 5.0);
      ultrasonic_timeout_flag = true;
      //pc.printf("timeout attach\r\n");
   }
   else if (distance >= 50 && ultrasonic_timeout_flag){
      //pc.printf("[2] distance >= 50 && timeout is activated\r\n");
      // pc.printf("timeout already attached\r\n");
   }
   else if (distance < 50 && !ultrasonic_timeout_flag){
      //pc.printf("[3] distance < 50 && timeout is NOT activated\r\n");
   }
   else if (distance < 50 && ultrasonic_timeout_flag){
      //pc.printf("[4] distance < 50 && timeout is activated\r\n");
      myTimeout.detach();
      ultrasonic_timeout_flag = false;
      //pc.printf("timeout detach\r\n");
   }
   wait(0.1);
}

void display(){
	check_temp_and_humid();
	myGUI.clearDisplay();
	myGUI.setTextCursor(0,0);
   //system on/off
	if(sys_on){
		myGUI.printf ("HVAC power is on\r\n");
	 //conditioner mode COOLER/HEATER
		if(mode == COOLER){
			myGUI.printf ("MODE : Cooler\r\n");
		}
		else{
			myGUI.printf ("MODE : Heater\r\n");
		}      

		//Temperature Cur/Desired
		myGUI.printf ("Current Temp : %.1f\r\nDesired Temp : %.1f\r\n", current_temp, desired_temp);
		//Humid
		myGUI.printf("Current Humid : %.lf\r\n", humid);
		//Wind power
		myGUI.printf("Wind : ");
		for(int i=0;i<wind_power;i++){
			myGUI.printf (">");
		}
		// Air vent
		myGUI.printf ("\r\n");

	}
	else{
		myGUI.printf ("HVAC power is off\r\n");
	}

   //myGUI.printf ("%ux%u OLED Display\r\n",myGUI.width(), myGUI.height());
	myGUI.display();
}

void SYSTEM_OFF(){
	sys_on = false;
	sound.period(1.0);
	sound=0.5;
	WIND_OFF();
	right_led=0;
	center_led=0;
	left_led=0;
	Red=0;
	Blue=0;
	Green=1;
}

void SYSTEM_ON(){
	sys_on = true;
	mode=COOLER;
	automatic=true;
	right_led=1;
	center_led=1;
	left_led=1;
	Red=0;
	Blue=1;
	Green=0;
	WIND_ON(wind_power);
}

void WIND_ON(int wind_power){
	if(mode==COOLER){
		mt.forward(float(wind_power)/5);
	}else{
		mt.backward(float(wind_power)/5);
	}
}
void WIND_OFF(){
	mt.stop();
}

void WARNING_MAX(){
	float max=MAX_RANGE;
	
	sound.period(max);
	sound=0.5;
	wait(0.5);
	sound.period(1);
	sound=0.5;
}
void WARNING_MIN(){
	float min=MIN_RANGE;
	sound.period(min);
	sound=0.5;
	wait(0.5);
	sound.period(1);
	sound=0.5;
}

void check_temp_and_humid(){
		float h=0.0f, c=0.0f;
		sensor.sample();
		c=sensor.getTemperature()/10;
		h=sensor.getHumidity()/10;
		current_temp=c;
		humid=h;
		//pc.printf("check temp and humid : %.1f %.1f\r\n",current_temp,humid);
}
