#include "mbed.h"
#include "hcsr04.h"
#include "Adafruit_SSD1306.h"
#include "motordriver.h"

Motor mt(D11,PC_8);

#define COOLER 1
#define HEAT 2

void delay(uint32_t time);
void config();
void RCC_Config();
void GPIO_Config();
void TIM_Config();
void ultrasonic();
void bluetooth();
void left_led_on();
void left_led_off();
void toggle_mode();
void center_led_on();
void center_led_off();
void toggle_center_led();
void right_led_on();
void right_led_off();
void toggle_right_led();
void print_led(int result);
void display();
void rgb_red_on();//RGB RED 
void rgb_red_off();
void rgb_blue_on(); //RGB BLUE
void rgb_blue_off();
void rgb_green_on();//RGB GREEN
void rgb_green_off();

void RGB_OFF();
void SYSTEM_OFF();
void SYSTEM_ON();
void WIND_ON(int wind_power);
void WIND_OFF();

bool is_pressed_SW2(){
   // PA14 SW2
   if((GPIOA->IDR & (1<<14)) == 0) return true;
   else return false;
}

bool is_pressed_SW9(){
   // PB7 SW9
   if((GPIOB->IDR & (1<<7)) == 0) return true;
   else return false;
}

bool is_pressed_SW10(){
   // PC4 SW10
   if((GPIOC->IDR & (1<<4)) == 0) return true;
   else return false;
}



class I2CPreInit : public I2C{
   public:
   I2CPreInit(PinName sda, PinName scl) : I2C(sda, scl){
      frequency(400000);
      start();
   };
};
I2C myI2C(I2C_SDA,I2C_SCL);
Adafruit_SSD1306_I2c myGUI(myI2C, D13, 0x78, 64, 128);

Serial pc(USBTX,USBRX);

DigitalOut trig_pin(D10);
DigitalIn echo_pin(D7);

HCSR04 hcsr(D10, D7);


bool sys_on=false;
int mode=COOLER; //COOLER, HEAT => default COOLER
float current_temp, desired_temp=18.0; //default = 18.0; 
int wind_power=2; // 1~5
bool automatic=true;
Ticker display_ticker;

int main(){
	myGUI.clearDisplay();
  config();
	
	display_ticker.attach(&display,1.0);	
	pc.printf("HAVC SYSTEM\r\n");
  while(1)
  { 
			current_temp=26.8;
		// click SW2 : left button = cooler / heater
      if((GPIOA->IDR & (1<<14)) == 0){
				if(sys_on){
					toggle_mode();
					rgb_red_off();
					rgb_green_off();
					rgb_blue_on();
				}
			}
			//click SW9 : center button = auto / manual
			if((GPIOB->IDR & (1<<7)) == 0){
				if(sys_on){
					pc.printf("center button\r\n");
					//toggle_center_led();
					if((GPIOB->ODR & (1<<10))==1){
						pc.printf("center led off\r\n");
						center_led_off();
					}else if((GPIOB->ODR & (1<<10))==0){
						pc.printf("center led on\r\n");
						center_led_on();
					}
					rgb_red_off();
					rgb_blue_off();
					rgb_green_on();
				}
			}
			//click SW 10 : right button = power on/off
			if((GPIOC->IDR & (1<<4)) == 0){
				if(sys_on){
					pc.printf("air conditioner power off\r\n");
					SYSTEM_OFF();
				}
				else{
					pc.printf("air conditioner power on\r\n");
					rgb_blue_off();
					rgb_green_off();
					rgb_red_on();
					SYSTEM_ON();
					WIND_ON(wind_power);
				}
			}   
			wait(0.5);
  }
}

void config(){
  pc.baud(9600);
  echo_pin.mode(PullDown); //ultrasonic
	//RCC_Config();
	GPIO_Config();
	//TIM_Config();
	//RGB_OFF();
}

void ultrasonic(){
      hcsr.start();
      wait_ms(300);
      printf("Distance(cm): %d\r\n", hcsr.get_dist_cm());
}

   
void delay(uint32_t time){
   //depend on hardware clock cycle, caculate second time
   while(time--);
}


void GPIO_Config(){
  //LED : PA13(PA13), D6(PB10), A2(PA4)   => bit print
  //Button :PA14(+1) , PB7(print) : , PC4
   //RGB: Red A1(PA1), Green PC6(PC6), Blue A3(PB0)
   
   
   RCC->AHB1ENR |= (1<<0); // GPIOA clock enable
   RCC->AHB1ENR |= (1<<1); // GPIOB clock enable
   RCC->AHB1ENR |= (1<<2); // GPIOC clock enable
   
		//PA13 LED : [27th:26th] => [0:1] => output mode
		GPIOA->MODER &= ~(1<<27);
		GPIOA->MODER |= (1<<26);
		
		//D6 LED => PB10 
		GPIOB->MODER &= ~(1<<21);
		GPIOB->MODER |= (1<<20);
		
		//A2 LED =>PA4
		GPIOA->MODER &= ~(1<<9);
		GPIOA->MODER |= (1<<8);
		
		//PA14 SWITCH
		GPIOA->MODER &= ~(1<<29);
		GPIOA->MODER &= ~(1<<28);
		
		//PB7 SWITCH
		GPIOB->MODER  &= ~(1<<15);
		GPIOB->MODER  &= ~(1<<14);
		
		//PC4 
		GPIOC->MODER &= ~(1<<9);
		GPIOC->MODER &= ~(1<<8);	
					 
		 //RGB RED PA1 Output mode
		 GPIOA->MODER &=~(1<<3);
		 GPIOA->MODER |=~(1<<2);

		 //RGB GREEN PB0 Output mode
		 GPIOB->MODER &= ~(1<<1);
		 GPIOB->MODER |= (1<<0); 

		 //RGB GREEN PC6 Output mode
		 GPIOC->MODER &= ~(1<<13);
		 GPIOC->MODER |= (1<<12); 
	
		GPIOA->OTYPER=0;
		GPIOA->OSPEEDR=0;
		GPIOB->OTYPER=0;
		GPIOB->OSPEEDR=0;
		GPIOC->OTYPER=0;
		GPIOC->OSPEEDR=0;
   
}

void left_led_on(){
   GPIOA->ODR |= (1<<13);         
}

void left_led_off(){
   GPIOA->ODR &= ~(1<<13);
}

void toggle_mode(){
	if((GPIOA->ODR & (1<<13)) == 1){
		pc.printf("Heater on\r\n");
		left_led_off();
		mode= HEAT;
	}else if((GPIOA->ODR & (1<<13))==0){
		left_led_on();
		pc.printf("Cooler on\r\n");
		mode = COOLER;
	}	
}

void center_led_on(){
   GPIOB->ODR |= (1<<10);
}
void center_led_off(){
   GPIOB->ODR &= ~(1<<10);
}

void right_led_on(){
   GPIOA->ODR |= (1<<4);         
}
void right_led_off(){
   GPIOA->ODR &= ~(1<<4);
}


void display(){
	//pc.printf("displayed\r\n");
   myGUI.clearDisplay();
   myGUI.setTextCursor(0,0);
		wait(0.1);
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
   
   wait(0.1);
   //myGUI.printf ("%ux%u OLED Display\r\n",myGUI.width(), myGUI.height());
   myGUI.display();
}

void rgb_red_on(){
    GPIOA->ODR |=(1<<1);
}
void rgb_red_off(){
    GPIOA->ODR &=~(1<<1);
}
void rgb_blue_on(){
    GPIOB->ODR |=(1<<0);
}
void rgb_blue_off(){
    GPIOB->ODR &=~(1<<0);
}
void rgb_green_on(){
    GPIOC->ODR |=(1<<6);
}
void rgb_green_off(){
    GPIOC->ODR &=~(1<<6);
}

void SYSTEM_OFF(){
	right_led_off();
	sys_on = !sys_on;
	
	left_led_off();
	mode=HEAT;
	
	center_led_off();
	automatic=false;
}
void SYSTEM_ON(){
	right_led_on();
	sys_on = !sys_on;
	
	left_led_on();
	mode=COOLER;
	
	center_led_on();
	automatic=true;
}

void WIND_ON(int wind_power){
	if(mode==COOLER){
		mt.forward(float(wind_power/5));
	}else{
		mt.backward(float(wind_power/5));
	}
}
void WIND_OFF(){
	mt.stop();
}