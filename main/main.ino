// CPE FINAL PROJECT
//Sofia
//Joe
//Helene
#include <LiquidCrystal.h>
#include <DHT11.h>
#include <Stepper.h>


 #define RDA 0x80
 #define TBE 0x20


DHT11 DHT(10); // connect to pin 10;

int stepsPerRevolution = 2048;
Stepper Vent = Stepper(stepsPerRevolution, 22, 24, 26 , 28);

const int WATERSENSORPIN = 2;

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


volatile unsigned int waterLevelPin = 0; // Pin ??
// LCD pins <--> Arduino pins
const int RS = 9, EN = 8, D4 = 4, D5 = 5, D6 = 6, D7 = 7; // connect RS(9) to Blue, en(8) to black

float ventPosition = 45; // limited from 0 - 90;
int waterLevel = 0;
int threshold = 500;
int state = 1; // 0 = idle, 1 = running ; 2 = DISABLED ; 3 = error -- Starts Disabled 

LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

void setup() {
  *LIGHT_DDR |= 0xFF;
  *LIGHT_DDR &= 0b11101111; // set pin 10 to read for humidity sensor.

  Serial.begin(9600);

  Vent.setSpeed(2);
  UARTStart(9600);
  lcd.begin(16, 2);
  adc_init();
}

void loop() {
  
  char* temp;
  char* hum;
  unsigned char waterlevel;
  intToCharArray(getTemp(),&temp);
  intToCharArray(getHumidity(),&hum);

  switch (state){
    case 0:
      // IDLE
      // check display temp hum
      //check water elvel
      waterlevel = adc_read(WATERSENSORPIN);
        if(waterlevel < 20){
          state = 3;
          break;
        }
      LCDMonitor(temp,hum);
      setStateLED('g');
      break;
    case 1:
      // RUNNING
      //check water level
      LCDMonitor(temp,hum);
      setStateLED('b');
      moveVent(-1);
      break;
    case 2:
      // DISABLED
      lcd.clear();
      setStateLED('y');
      break;
    case 3:
      // ERROR 
      LCDDisplay(0,"Water Level is");
      LCDDisplay(1,"too low!");
      setStateLED('r');
      break;
  } 
// remove after testing
  delay(1000);
//state++;
if(state == 4){
  state = 0;
}

// Memory freeing
  free(temp);
  free(hum);
}
// Functions
//

//return 1 if okay 0 if too low
int checkWaterLevel(){
  pinMode(waterLevelPin, OUTPUT);
  waterLevel = adc_read(0);

 if(waterLevel >= threshold){
  return 1;
  }
  else if(waterLevel< threshold){
    return 0;
  }
}

// the movement up or down
void moveVent(int direction){
  if (direction == -1 && ventPosition > 0){
    Vent.step(-stepsPerRevolution/48); //7.5* shift
    ventPosition -= 7.5;
  } else if (ventPosition < 90 && direction == 1){
    Vent.step(stepsPerRevolution/48);
    ventPosition += 7.5;
  }
}

void controlFan(int onOff){
  if (onOff == 1){
    //set pin high
  } else if (onOff == 0){
    //set pin low
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