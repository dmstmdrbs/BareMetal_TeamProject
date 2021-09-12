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
void ADC_Init();
void ADC_Enable();
void ADC_Start(int channel);
void ADC_WaitForConv();
void ADC_Disable();
void delay(uint32_t time);

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
bool vent;
uint16_t ADC_GetVal();
uint16_t ADC_VAL[2] = {0, 0};

int main(){
   config();

   while(1)
   {
		 	if((GPIOC->IDR & (1<<4))==0){
				wait(0.3);
				SYSTEM_ON();
			}
      while(sys_on){
        ultrasonic();
				while(1){
					ADC_Start(12);
					ADC_WaitForConv();
					ADC_VAL[0] = ADC_GetVal();
					x = ADC_VAL[0];
					
					ADC_Start(13);
					ADC_WaitForConv();
					ADC_VAL[1] = ADC_GetVal();
					y = ADC_VAL[1];
					pc.printf("x: %d, y: %d\r\n", x, y);
					//debug_print = 1;
					if(y>765*4){
						break;
					}
					else if(x<5*4&&y<5*4){

						//temp up
						
						if(desired_temp>=32){
							WARNING_MAX();
						}else{
							desired_temp+=0.5;
						}
						automatic=false;
						GPIOB->ODR &=~(1<<10);
						break;
					}
					else if(x>700*4&&y<5*4){
						//wind power up
						if(wind_power==5){
							WARNING_MAX();
						}else{
							wind_power+=1;
						}
						automatic=false;
						GPIOB->ODR &=~(1<<10);
						break;
						
					}
					else if(x<5*4&&y>700*4){
						
						//temp down
						if(desired_temp<=16.06){
							WARNING_MIN();
						}else{
							desired_temp-=0.5;
						}
						automatic=false;
						GPIOB->ODR &=~(1<<10);
						break;
						
					}
					else if(x>730*4&&y>720*4){
						//wind power down
						
						if(wind_power==1){
							WARNING_MIN();
						}else{
							wind_power-=1;
						}
						automatic=false;
						GPIOB->ODR &=~(1<<10);
						break;
					}		
				}
			 // click SW2 : left button = cooler / heater
			 if((GPIOA->IDR & (1<<14))==0){
					if(mode==COOLER){
						mode=HEAT;
						GPIOA->ODR &= ~(1<<13);
						GPIOA->ODR |= (1<<1);
						GPIOB->ODR &= ~(1<<0);
						GPIOC->ODR &= ~(1<<6);
					}
					else{
						mode=COOLER;
						GPIOA->ODR |= (1<<13);
						GPIOA->ODR &= ~(1<<1);
						GPIOB->ODR |= (1<<0);
						GPIOC->ODR &= ~(1<<6);
					}
					wait(0.01);
					WIND_ON(wind_power);
			 }
		//click SW9 : center button = auto / manual
			if((GPIOB->IDR & (1<<7))==0){
					if(automatic)
					{
						 automatic=false;
						 vent = false;
						 GPIOB->ODR &=~(1<<10);
					}
					else{
						 automatic=true;
						 vent = true;
						 GPIOB->ODR |=(1<<10);
					}
			}
			pc.printf("current_temp:%f desired_temp:%f wind_power:%d mode:%d\r\n",current_temp,desired_temp,wind_power,mode);
			
			if(automatic){
			// AUTO MODE
			pc.printf("AUTO MODE\r\n");
				if(mode==COOLER){
					pc.printf("COOLER MODE\r\n");
					if(current_temp-desired_temp>=10){
						wind_power = 5;
						WIND_ON(wind_power);
					} else if(current_temp-desired_temp>=6){
						wind_power = 4;
						WIND_ON(wind_power);
					}else if(current_temp-desired_temp>=3){
						wind_power = 3;
						WIND_ON(wind_power);
					}else if(current_temp-desired_temp>=1){
						wind_power = 2;
						WIND_ON(wind_power);
					}else {
						wind_power=1;
						WIND_ON(wind_power);
					}
				}
				else{
					pc.printf("HEATER MODE\r\n");
					if(desired_temp-current_temp>=10){
						wind_power = 5;
						WIND_ON(wind_power);
					} else if(desired_temp-current_temp>=6){
						wind_power = 4;
						WIND_ON(wind_power);
					}else if(desired_temp-current_temp>=3){
						wind_power = 3;
						WIND_ON(wind_power);
					}else if(desired_temp-current_temp>=1){
						wind_power = 2;
						WIND_ON(wind_power);
					}else {
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
			if((GPIOC->IDR & (1<<4))==0){
					if(sys_on){
							SYSTEM_OFF();
							wait(0.3);
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
	//ADC init, enable
	ADC_Init();
	ADC_Enable();
	
	myGUI.clearDisplay();
	echo_pin.mode(PullDown); //ultrasonic
	display_ticker.attach(&display,1.0);	
	pc.printf("HAVC SYSTEM\r\n");
	
	//Set Register
	RCC->AHB1ENR |= (1<<0); //Assign Clock GPIOA
	RCC->AHB1ENR |= (1<<1); //Assign Clock GPIOB
	RCC->AHB1ENR |= (1<<2); //Assign Clock GPIOC
	//SET MODE
	//BUTTON
	GPIOA->MODER &=~(3<<28); //PA14 Input Mode
	GPIOB->MODER &=~(3<<14); //PB7 Input Mode
	GPIOC->MODER &=~(3<<8); //PC4 Input Mode
	//LED
	GPIOA->MODER &=~(1<<27); //PA13 left_led
	GPIOA->MODER |=(1<<26);
	GPIOB->MODER &=~(1<<21);//PB10 center_led
	GPIOB->MODER |=(1<<20);
	GPIOA->MODER &=~(1<<9); //PA4 right_led
	GPIOA->MODER |=(1<<8);
	//RGB
	GPIOA->MODER &=~(1<<3); //PA1 Red 
	GPIOA->MODER |=(1<<2);
	GPIOC->MODER &=~(1<<13);//PC6 Green
	GPIOC->MODER |=(1<<12);
	GPIOB->MODER &=~(1<<1);//PB0 Blue
	GPIOB->MODER |=(1<<0);
	//Set Output speed
	GPIOA->OTYPER=0;
	GPIOA->OSPEEDR=0;
	GPIOB->OTYPER=0;
	GPIOB->OSPEEDR=0;
	GPIOC->OTYPER=0;
	GPIOC->OSPEEDR=0;
	
}

void timeout_SYSTEM_OFF(){
    pc.printf("Human not detected!! System off\r\n");
    SYSTEM_OFF();
		ultrasonic_timeout_flag = false;
}

void joystick_handler(){
	ADC_Start(12);
	ADC_WaitForConv();
	ADC_VAL[0] = ADC_GetVal();
	x = ADC_VAL[0] * 1000;
					
	ADC_Start(13);
	ADC_WaitForConv();
	ADC_VAL[1] = ADC_GetVal();
	y = ADC_VAL[1] * 1000;
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
		myGUI.printf ("\r\n");
		// Air vent
		if(vent){
			myGUI.printf ("vent open\r\n");

		}else{
			myGUI.printf ("vent closed\r\n");

		}
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
	GPIOA->ODR &= ~(1<<4);
	GPIOB->ODR &=~(1<<10);
	GPIOA->ODR &= ~(1<<13);
	GPIOA->ODR &= ~(1<<1);
	GPIOB->ODR &= ~(1<<0);
	GPIOC->ODR |= (1<<6);
}

void SYSTEM_ON(){
	vent = true;
	sys_on = true;
	mode=COOLER;
	automatic=true;
	GPIOA->ODR |= (1<<4);
	GPIOB->ODR |=(1<<10);
	GPIOA->ODR |= (1<<13);
	GPIOA->ODR &= ~(1<<1);
	GPIOB->ODR |= (1<<0);
	GPIOC->ODR &= ~(1<<6);
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

void ADC_Init(){
	RCC->APB2ENR |= (1<<8);
	RCC->AHB1ENR |= (1<<2);
	
	ADC->CCR |= 1<<16;
	
	ADC1->CR1 = (1<<8);
	ADC1->CR1 &= ~(1<<24);
	
	ADC1->CR2 |= (1<<1);
	ADC1->CR2 |= (1<<10);
	ADC1->CR2 &= ~(1<<11);
	
	ADC1->SMPR1 &= ~((1<<6)|(1<<9));
	
	ADC1->SQR1 |= (1<<20);
	
	GPIOC->MODER |= (3<<4);
	GPIOC->MODER |= (3<<6);
	
}

void ADC_Enable(){
	ADC1->CR2 |= 1<<0;
	uint32_t delay = 10000;
	while(delay--);
}

void ADC_Start(int channel){
	ADC1->SQR3 = 0;
	ADC1->SQR3 |= (channel<<0);
	
	ADC1->SR = 0;
	
	ADC1->CR2 |= (1<<30);
}

void ADC_WaitForConv(){
	while(!(ADC1->SR & (1<<1)));
}

uint16_t ADC_GetVal(){
	return ADC1->DR;
}

void ADC_Disable(){
	ADC1->CR2 &= ~(1<<0);
}

void delay(uint32_t time){
	while(time--);
}


