/* Sample code for Lab 3.1. This code provides a basic start. */
#include <c8051_SDCC.h>
#include <stdio.h>
#include <stdlib.h>
//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------
void Port_Init(void);
void PCA_Init (void);
void XBR0_Init(void);
void Steering_Servo(void);
void PCA_ISR ( void ) __interrupt 9;
void Interrupt_Init(void);
//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
unsigned int PW_CENTER = 2765;
unsigned int PW_MIN = 2028;
unsigned int PW_MAX = 3502;
unsigned int PW1 = 0;
unsigned int PW2 = 0;
unsigned int PCA_counts;

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

    //print beginning message
    printf("Embedded Control Steering Calibration\r\n");
    // set the PCA output to a neutral setting
    if((PW1 != PW_CENTER) || (PW2 != PW_CENTER) )
    {
        printf("Please input a random number to calibrate.\r\n");
        getchar();
        PW1 = PW2 = PW_CENTER;
        printf("Calibration completed. The pulsewidth is %d\r\n",PW1);
    }
    printf("Wait for 1 second.\r\n");
    PCA_counts = 0;
    while(PCA_counts < 50);
    while(1)
        Steering_Servo();
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

void Interrupt_Init()
{
    EIE1 |= 0x08;    // enable PCA interrupts
    EA = 1;          // enable all interrupts
}


//-----------------------------------------------------------------------------
// PCA_ISR
//-----------------------------------------------------------------------------
//
// Interrupt Service Routine for Programmable Counter Array Overflow Interrupt
//
void PCA_ISR ( void ) __interrupt 9
{
    if(CF)
    {
        PCA0 = 28672;
        CF = 0;
    }
    else PCA0CN &= 0xC0; 
    PCA_counts++; // reference to the sample code in Example 4.5 -Pulse Width Modulation 
    // implemented using the PCA (Programmable Counter Array), p. 50 in Lab Manual.
}

void Steering_Servo()
{
    char input;
    //wait for a key to be pressed
    input = getchar();
    if(input == 'l')  // single character input to increase the pulsewidth
    {
        PW1 -= 10;
        if(PW1 < PW_MIN)  // check if less than pulsewidth minimum
        {
            PW1 = PW_MIN;
        }    // set SERVO_PW to a minimum value
    }
    else if(input == 'r')  // single character input to decrease the pulsewidth
    {
        PW1 += 10;
        if(PW1 > PW_MAX)  // check if pulsewidth maximum exceeded
        {
            PW1 = PW_MAX;
        }     // set PW to a maximum value
    }
    else if(input == 'f')  // single character input to decrease the pulsewidth
    {
        PW2 += 10;
        if(PW2 > PW_MAX)  // check if pulsewidth maximum exceeded
        {
            PW2 = PW_MAX;
        }     // set PW to a maximum value
    }
    else if(input == 's')  // single character input to decrease the pulsewidth
    {
        PW2 -= 10;
        if(PW1 < PW_MIN)  // check if pulsewidth maximum exceeded
        {
            PW1 = PW_MIN;
        }     // set PW to a maximum value
    }
    else if(input == 'c')
    {
        PW1 = PW2 = PW_CENTER;
    }
    printf("PW1: %u\r\n", PW1);
    printf("PW2: %u\r\n\n", PW2);
    PCA0CP0 = 0xFFFF - PW1;
    PCA0CP2 = 0xFFFF - PW2;
}
