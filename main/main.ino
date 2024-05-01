// CPE FINAL PROJECT
//Sofia
//Joe
//Helene
 #include <LiquidCrystal.h>
#include <DHT11.h>
#include <Stepper.h>
 #define RDA 0x80
 #define TBE 0x20  
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


volatile unsigned int waterLevelPin = 4; // Pin 10 
int stepsPerRevolution = 0;
// LCD pins <--> Arduino pins
const int RS = 9, EN = 8, D4 = 4, D5 = 5, D6 = 6, D7 = 7; // connect RS(9) to 

int waterLevel = 0;
int threshold = 500;
int state = 0; // 0 = idle, 1 = running ; 2 = DISABLED ; 3 = error

LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

void setup() {
  // put your setup code here, to run once:
  *LIGHT_DDR |= 0xFF;

  UARTStart(9600);
  lcd.begin(16, 2);
}

void loop() {
  // put your main code here, to run repeatedly:
  switch (state){
    case 0:
      // IDLE
      LCDDisplay("Green");
      setStateLED('g');
      break;
    case 1:
      // RUNNING
      LCDDisplay("Blue");
      setStateLED('b');
      break;
    case 2:
      // DISABLED
      LCDDisplay("Yellow");
      setStateLED('y');
      break;
    case 3:
      // ERROR 
      LCDDisplay("Red");
      setStateLED('r');
      break;
  } 
// remove after testing
  delay(1000);
state++;
if(state == 4){
  state = 0;
}
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
  lcd.print("Water level too low");
  return 0;
}
}

// the movement up or down
void moveVent(int direction){
const int Revolution = 2038;
Stepper myStepper = Stepper(stepsPerRevolution, 8, 10, 9, 11);
}

// display message via LCD
void LCDDisplay(char c[]){
  lcd.clear(); // Clear the LCD display
  lcd.setCursor(0, 0); // Set the cursor to the top-left position
  lcd.write(c); // Print the number to the LCD
}

// display message via UART/Serial monitor
void UARTDisplay(unsigned char message[],int length){
  for (int i = 0; i < length;i++){
    UARTOut(message[i]);
  }
  UARTOut('\n');
}

// cheack temp
float getTemp(){
  //int chk = DHT11.read(DHT11PIN);

  Serial.print("Humidity (%): ");
  //Serial.println((float)DHT11.humidity, 2);

  Serial.print("Temperature  (C): ");
 // Serial.println((float)DHT11.temperature, 2);
}

//check humidity
float getHumidity(){

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