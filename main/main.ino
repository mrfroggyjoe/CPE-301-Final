// CPE FINAL PROJECT
//Sofia
//Joe
//Helen

 #define RDA 0x80
 #define TBE 0x20  
 volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
 volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
 volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
 volatile unsigned int  *myUBRR0  = (unsigned int *) 0x00C4;
 volatile unsigned char *myUDR0   = (unsigned char *)0x00C6;


void setup() {
  // put your setup code here, to run once:
  UARTStart(9600);
}

void loop() {
  // put your main code here, to run repeatedly:

}
// Functions
//

//return 1 if okay 0 if too low
int checkWaterLevel(){

}

// the movement up or down
void moveVent(int direction){

}

// display message via LCD
void LCDDisplay(){

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

void UARTOut(unsigned char out){
  while(!(*myUCSR0A & 0b00100000)){}
  *myUDR0 = out;
}
