/* Sample code for Lab 3.1. This code provides a basic start. */
#include <c8051_SDCC.h>
#include <stdio.h>
#include <stdlib.h>
#include <i2c.h>
//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------
void Port_Init(void);
void PCA_Init (void);
void XBR0_Init(void);
void SMB_Init(void);
void Steering_Servo(void);
void PCA_ISR ( void ) __interrupt 9;
void Interrupt_Init(void);
unsigned int ReadRanger(void);
//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
unsigned char new_range = 0;
unsigned int a;

unsigned char PCA_counts = 0;
unsigned char input;

//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
void main(void)
{
    // initialize board
	unsigned char Data1[1];
	unsigned char addr;


    Sys_Init();
    putchar(' '); //the quotes in this line may not format correctly
    Port_Init();
    XBR0_Init();
    PCA_Init();
    Interrupt_Init();
    SMB_Init();
	
    while(1)
    {
    	input = getchar();
    	if(input == 49)
    	{
    		while(1)
            {
                if(new_range)
                {
                    a = ReadRanger();
                    new_range = 0;
                    Data1[0] = 0x51;
                    addr = 0xE0;
                    i2c_write_data(addr,0,Data1,1);         
                    printf("The new range is %d\r\n",a);
                }
            }
                
    	}
    	else if(input == 50)
    	{
    		while(1)
            {
                if(new_range)
                {
                    a = ReadCompass();
                    new_range = 0;
                    Data1[0] = 0x51;
                    addr = 0xC0;
                    i2c_write_data(addr,0,Data1,1);         
                    printf("The new heading is %d\r\n",a);
                }
            }
                
    	}

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
    //P1MDOUT |= 0x05;  //set output pin for CEX0 or CEX2 in push-pull mode

//    P0MDOUT |= 0xC0;
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
void PCA_ISR ( void ) __interrupt 9
{
    if(CF)
    {
        PCA0 = 28672;
        CF = 0;
        PCA_counts++; 
        if(PCA_counts >= 6)
        {
        	new_range = 1;
        	PCA_counts = 0;
        }
    }
    else PCA0CN &= 0xC0; 
    // reference to the sample code in Example 4.5 -Pulse Width Modulation 
    // implemented using the PCA (Programmable Counter Array), p. 50 in Lab Manual.
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

 