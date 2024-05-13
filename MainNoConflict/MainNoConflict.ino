// CPE FINAL PROJECT
//Sofia Ahlstedt
//Joe Antrim
//Helene Henry-Greard
#include <LiquidCrystal.h>
#include <DHT11.h>
#include <Stepper.h>
#include <RTClib.h>


 #define RDA 0x80
 #define TBE 0x20


DHT11 DHT(10); // connect to pin 10;
RTC_DS3231 rtc;

int stepsPerRevolution = 2048;
Stepper Vent = Stepper(stepsPerRevolution, 22, 24, 26 , 28); // in1 - 26, in2 - 22, in 28, in4 - 24

const int WATERSENSORPIN = A0;
const byte StartButton = 2; 
volatile bool startCooler = false;

 volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
 volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
 volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
 volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
 volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;

volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

volatile unsigned char* LIGHT_DDR = (unsigned char*) 0x24; //pin 11 12 13 /-- PORT B
volatile unsigned char* LIGHT_PORT = (unsigned char*) 0x25; //pin 11 12 13 /-- PORT B
volatile unsigned int RED = 5; // pin 11
volatile unsigned int GREEN = 6; // pin 12 
volatile unsigned int BLUE = 7; // pin 13

volatile unsigned char* MOTOR_DDR = (unsigned char*) 0x10A; // PORT L PINS 42 - 49 inclusive
volatile unsigned char* MOTOR_PORT = (unsigned char*) 0x10B; // PORT L PINS 42 - 49 inclusive

volatile unsigned char* START_DDR = (unsigned char*) 0x2D; //pin 2 /-- PORT E



// LCD pins <--> Arduino pins
const int RS = 9, EN = 8, D4 = 4, D5 = 5, D6 = 6, D7 = 7; // connect RS(9) to Blue, en(8) to black
//const int RS = 53, EN = 52, D4 = 44, D5 = 45, D6 = 46, D7 = 47;
float ventPosition = 45; // limited from 0 - 90;
int waterLevel = 0;
int threshold = 500;
int state = 1; // 0 = idle, 1 = running ; 2 = DISABLED ; 3 = error -- Starts Disabled 

LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

void setup() {
  *LIGHT_DDR |= 0xFF;
  *LIGHT_DDR &= 0b11101111; // set pin 10 to read for humidity sensor.
  *MOTOR_DDR |= 0b10000000; // set pin 42 to write for fan control
  *MOTOR_DDR &= 0b10000000; // set pin 43-49 for read

  *START_DDR &= 0b11101111; // set pin 2 to read for interupt start

  attachInterrupt(digitalPinToInterrupt(StartButton), blink, RISING);
  Vent.setSpeed(5);
  UARTStart(9600);
  lcd.begin(16, 2);
  adc_init();
     // SETUP RTC MODULE
  if (! rtc.begin()) {
    lcd.clear();
    lcd.print("Couldn't find RTC");
    while (1);
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  
}

void loop() {
  if (!startCooler){
    //Disabled
    moveVent(getVentMovement());
    setStateLED('y');
    lcd.clear();
  }
  if (startCooler){
  char* temp;
  char* hum;
  
  unsigned char waterlevel;
  intToCharArray(getTemp(),&temp);
  intToCharArray(getHumidity(),&hum);

  switch (state){
    case 0:
      // IDLE
      LCDMonitor(temp,hum);
      setStateLED('g');
      moveVent(getVentMovement());
      controlFan(1);
      break;
    case 1:
      // RUNNING
      moveVent(getVentMovement());
      LCDMonitor(temp,hum);
      setStateLED('b');
      controlFan(0);
      break;
    case 2:
      // DISABLED
      moveVent(getVentMovement());
      lcd.clear();
      setStateLED('y');
      controlFan(1);
      break;
    case 3:
      // ERROR 
      LCDDisplay(0,"Water Level is");
      LCDDisplay(1,"too low!");
      setStateLED('r');
      controlFan(1);
      break;
  }
    // Change between states 
    if(checkWaterLevel() == 3){ 
      state = 3;
    } else if (getTemp() < 74){
      state = 0; // good temp, no fan - idle
    } else {
      state = 1; // too high, need fan - running
    }
    // Memory freeing
      free(temp);
      free(hum); 
    }
}
// Functions
//

//return 0 if okay 3 if too low
int checkWaterLevel(){
  int waterlevel = adc_read(WATERSENSORPIN);
    if(waterlevel < 200){
      return 3;
    }
  return 0;
}

// the movement up or down
void moveVent(int direction){
  if (direction == -1 && ventPosition > 0){
    Vent.step(-stepsPerRevolution/24); //7.5* shift
    ventPosition -= 7.5;
  }
  if (ventPosition < 90 && direction == 1){
    Vent.step(stepsPerRevolution/24);
    ventPosition += 7.5;
  }
}

void controlFan(int onOff){
  if (onOff == 1){
    *MOTOR_PORT |= 0b10000000; // turn off fan
  } else if (onOff == 0){
    *MOTOR_PORT &= 0b01111111; // turn on fan
  }
  reportTime();
}

void reportTime(){
  DateTime now = rtc.now();
  UARTStart(9600);
 
  UARTOut(now.month());
  UARTOut('/');
  UARTOut(now.day());
  UARTOut('/');
  UARTOut(now.year());
  UARTOut(' ');
  UARTOut(now.hour());
  UARTOut(':');
  UARTOut(now.minute());
  UARTOut(':');
  UARTOut(now.second());
  UARTOut('\n'); // Transmit newline

}


int getVentMovement(){
  if (*MOTOR_PORT & 0b01000000){
    return -1;
  } 
  else if (*MOTOR_PORT & 0b00100000){
    return 1;
  }
}

// display message via UART/Serial monitor
void UARTDisplay(unsigned char message[],int length){
  for (int i = 0; i < length;i++){
    UARTOut(message[i]);
  }
  UARTOut('\n');
}

// check temp
int getTemp(){
  int temperature = DHT.readTemperature();
  return temperature * 1.8 + 32;
}

//check humidity
int getHumidity(){
  int humidity = DHT.readHumidity();
  return humidity;
}

//initialize the UART
void UARTStart(unsigned long baud){
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / baud - 1);
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}

//output via uart
void UARTOut(unsigned char out){
  while(!(*myUCSR0A & 0b00100000)){}
  *myUDR0 = out;
}
//ADC functions:
void adc_init()
{
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}

unsigned int adc_read(unsigned char adc_channel_num)
{
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if(adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register
  return *my_ADC_DATA;
}

void setStateLED(char c){
  if (c == 'b'){
    *LIGHT_PORT &= 0x00;
    *LIGHT_PORT |= 1<<BLUE;
  } else if (c == 'g'){
    *LIGHT_PORT &= 0x00;
    *LIGHT_PORT |= 1<<GREEN;
  } else if (c == 'r'){
    *LIGHT_PORT &= 0x00;
    *LIGHT_PORT |= 1<<RED;
  } else if (c == 'y'){
    *LIGHT_PORT &= 0x00;
    *LIGHT_PORT |= 1<<RED;
    *LIGHT_PORT |= 1<<GREEN;
  }
}

void LCDMonitor(char* t, char* h){
  LCDDisplay(0,"Tempurature: ");
  LCDAppend(0,13,t,2);
  LCDAppend(0,15,"*",1);
  LCDDisplay(1,"Humidity: ");
  LCDAppend(1,13,h,2);
  LCDAppend(1,15,"%",1);
}

// display message via LCD
void LCDDisplay(int line, char c[]){
  if (line == 0) { lcd.clear(); } // Clear the LCD display
  lcd.setCursor(0, line); // Set the cursor to the left position
  lcd.write(c); // Print the number to the LCD
}

void LCDAppend(int line, int x, char c[],int l){
  lcd.setCursor(x, line); // Set the cursor to the position
  for (int i = 0; i < l; i++){
      lcd.write(c[i]);
  }
}

void intToCharArray(int in, char **mem){
  char *c = malloc(2);
  c[1] = '0' + in%10;
  c[0] = '0' + (in - in%10)/10;
  *mem = c;
}

void blink() {
  if (startCooler == false){
    startCooler = true;
  } else if (startCooler == true){
    startCooler = false;
  }
}
