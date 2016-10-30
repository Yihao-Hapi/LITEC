/* Sample code for Lab 3.1. This code provides a basic start. */
#include <c8051_SDCC.h>
#include <stdio.h>
#include <stdlib.h>
#include <i2c.h>
//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------
void Port_Init(void);
void PCA_Init(void);
void XBR0_Init(void);
void Steering_Servo(void);
void SMB_Init(void);
void PCA_ISR( void ) __interrupt 9;
void Interrupt_Init(void);
unsigned int ReadRanger(void);
unsigned int ReadCompass(void);
void wait_for_100ms(void);
//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
unsigned int PW_CENTER = 2765;
unsigned int PW_MIN = 2028;
unsigned int PW_MAX = 3502;
unsigned int PW1 = 0;
unsigned int PW2 = 0;
unsigned int PCA_counts;
unsigned int heading = 0;
unsigned int distance = 0;

//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
void main(void){
    // initialize board
    Sys_Init();
    putchar(' '); //the quotes in this line may not format correctly
    Port_Init();
    XBR0_Init();
    PCA_Init();
    Interrupt_Init();
    SMB_Init();

    //print beginning message
    printf("Embedded Control Steering Calibration\r\n");
    // set the PCA output to a neutral setting
    if((PW1 != PW_CENTER) || (PW2 != PW_CENTER) )
    {
        printf("Please input a random number to calibrate.\r\n");
        getchar();
        PW1 = PW_CENTER;
		PW2 = PW_CENTER;
        printf("Calibration completed. The pulsewidth is %d\r\n",PW1);
    }
    printf("Wait for 1 second.\r\n");
    PCA_counts = 0;
    while(PCA_counts < 50);
	printf("a");
    while(1)
    {
        
		wait_for_100ms();
        printf("a");
        Steering_Servo();
    }

}

//-----------------------------------------------------------------------------
// Port_Init
//-----------------------------------------------------------------------------
//
// Set up ports for input and output
//
void Port_Init()
{
    P1MDOUT |= 0x05;  //set output pin for CEX0 or CEX2 in push-pull mode
}

//-----------------------------------------------------------------------------
// XBR0_Init
//-----------------------------------------------------------------------------
//
// Set up the crossbar
//
void XBR0_Init()
{
    XBR0 = 0x27;  //configure crossbar as directed in the laboratory

}

//-----------------------------------------------------------------------------
// PCA_Init
//-----------------------------------------------------------------------------
//
// Set up Programmable Counter Array
//
void PCA_Init(void)
{
    PCA0CPM0 = PCA0CPM2 = 0xC2;// reference to the sample code in Example 4.5 -Pulse Width Modulation 
    PCA0CN = 0x40;
    PCA0MD = 0x81;// implemented using the PCA (Programmable Counter Array), p. 50 in Lab Manual.
}

void Interrupt_Init(void)
{
    EIE1 |= 0x08;    // enable PCA interrupts
    EA = 1;          // enable all interrupts
}
void SMB_Init(void)
{
    SMB0CR = 0x93;
    ENSMB = 1;
}


//-----------------------------------------------------------------------------
// PCA_ISR
//-----------------------------------------------------------------------------
//
// Interrupt Service Routine for Programmable Counter Array Overflow Interrupt
//
void PCA_ISR(void) __interrupt 9
{
    if(CF)
    {
        PCA0 = 28672;
        CF = 0;
    }
    PCA0CN &= 0xC0; 
    PCA_counts++; // reference to the sample code in Example 4.5 -Pulse Width Modulation 
    // implemented using the PCA (Programmable Counter Array), p. 50 in Lab Manual.
}

void Steering_Servo(void)
{
	unsigned char Data1[1];
    unsigned char addr = 0; 
	unsigned int current_heading = heading;
    distance = ReadRanger();
    heading = ReadCompass();
    printf("Distance is %u\r\n",distance);
    printf("Heading is %u\r\n",heading);
    
    PW1 = steer_control(current_heading,heading,PW_MAX,PW_MIN,PW_CENTER);
    Data1[0] = 0x51;
    addr = 0xE0;
    i2c_write_data(addr,0,Data1,1);
    
    if(distance <= 10)
    {
        PW2 = PW_MAX;
    }
    else if((distance > 10)&&(distance < 40))
    {
        PW2 = PW_CENTER + 24.56*(40-distance);
    }
    else if((distance >= 40)&&(distance <= 50))
    {
        PW2 = PW_CENTER;
    }
    else if((distance > 50)&&(distance <90))
    {
        PW2 = PW_CENTER - 18.425*(distance-50);
    }
    else if(distance >= 90)
    {
        PW2 = PW_MIN;
    }
	printf("Speed_PW: %u\r\n\n", PW2);
	printf("Heading_PW: %u\r\n", PW1);
    
    PCA0CP2 = 0xFFFF - PW2;
}

unsigned int ReadRanger(void)
{
    unsigned char Data[2];
    unsigned int range = 0;
    unsigned char addr = 0xE0;
    i2c_read_data(addr,2,Data,2);
    range = ( (Data[0] <<8) | Data[1]);
    return range;
}

unsigned int ReadCompass(void) 
{
    unsigned char Data[2];
    unsigned int heading = 0;
    unsigned char addr = 0xC0;
    i2c_read_data(addr,2,Data,2);
    heading = ( (Data[0] <<8) | Data[1]);
    return heading;
}

void wait_for_100ms(void)
{
    PCA_counts = 0;
    while(PCA_counts <= 5);
   
    
}

unsigned int steer_control(unsigned int current_direction,unsigned int direction,unsigned int max_pw,unsigned int min_pw,unsigned int cali_pw){

    //function used to control the direction of the wheels

    int change = direction - current_direction;

    unsigned int pw;

    if (1800 >= change > 0){ // turn right

        pw = cali_pw+((max_pw - cali_pw) * (change/1800.0));

    }

    else if (3600 >= change > 1800){// turn right more than 180 degrees, turn left

        unsigned int reverse = 3600 - change;

        pw = cali_pw - (cali_pw-min_pw)*(reverse/1800.0);

    }

    else if (-1800 <= change < 0){ // turn left

        pw = cali_pw - (cali_pw - min_pw) *(abs(change)/1800.0);

    }

    else if(-3600 <= change< -1800){

        unsigned int reverse = 3600 - abs(change); // turn left more than 180 degrees, turn right

        pw = cali_pw + (max_pw-cali_pw) *(reverse/1800.0);

    }

    return pw;

}