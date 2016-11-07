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
void Read_Ranger_and_Compass(void);
void SMB_Init(void);
void PCA_ISR( void ) __interrupt 9;
void Interrupt_Init(void);
unsigned int ReadRanger(void);
unsigned int ReadCompass(void);
void wait_for_100ms(void);
void wait_for_1s(void);
void heading_to_east(void);
char get_speed(void);
signed int get_desired_heading(void);
int get_steering_gain(void);
void Execute_PW(void);
void ADC_Init(void);
unsigned char read_AD_input(unsigned char n);
//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
unsigned int PW_CENTER = 2765;
unsigned int PW_MIN = 2028;
unsigned int PW_MAX = 3502;
unsigned int PW1 = 0;
unsigned int PW2 = 0;
unsigned int PCA_counts;
signed int heading = 0;
unsigned int distance = 0;
char keypad;
char check;
char speed_level;
unsigned int speed_level_int;
signed int desired_heading = 0;
int steering_gain;
signed int error;

unsigned char AD_read;
unsigned int AD_value;
unsigned int battery_read;

__sbit __at 0xB0 SS;

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
    ADC_Init();

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
    wait_for_1s();
	speed_level = get_speed();
    speed_level_int = (unsigned int) speed_level;
    desired_heading = get_desired_heading();
    steering_gain = get_steering_gain();

    while(1)
    {
        AD_read = read_AD_input(7);
        printf("AD_read is %c\r\n", AD_read);
        AD_value = (unsigned int) AD_value ;
        battery_read = AD_value * 71.875; // AD_value * 2.4 / gain / 256 * 11.5k ohm / 1.5 k ohm * 1000
        printf("The battery read is %u\r\n", battery_read);
        Read_Ranger_and_Compass();
        if(distance > 50)
        {
            heading_to_east();
            PW2 = PW_CENTER + (speed_level_int - 48)*80;
            Execute_PW();
        } 
        else if(distance < 10)
        {
            PW1 = PW2 = PW_CENTER;
            Execute_PW();

        }
        else if(distance <= 50)
        {
            PW1 = PW_MIN;
            PW2 = PW_CENTER + (speed_level_int - 48)*80;
            Execute_PW();
            wait_for_1s();

        }//read data from ranger and compass, then change PCA compare module
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
    P1MDIN &= ~0x80; /* Set P1.7 for analog input */
    P1MDOUT &= ~0x80; /* Set P1.7 to open drain */
    P1 |= 0x80; /* Send logic 1 to input pin P1.7 */

    P1MDOUT |= 0x05;  //set output pin for CEX0 or CEX2 in push-pull mode
    P3MDOUT &= 0x81;    //set P3.0 to input
    P3 = 0x81;          //set P3.0 to hign impedance
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

void ADC_Init(void)
{
    REF0CN = 0x03; /* Set Vref to use internal reference voltage (2.4 V)
*/
    ADC1CN = 0x80; /* Enable A/D converter (ADC1) */
    ADC1CF |= 0x01; /* Set A/D converter gain to 1 */
}

unsigned char read_AD_input(unsigned char n)
{
    AMX1SL = n; /* Set P1.n as the analog input for ADC1 */
    ADC1CN = ADC1CN & ~0x20; /* Clear the ��Conversion Completed�� flag */
    ADC1CN = ADC1CN | 0x10; /* Initiate A/D conversion */
    while ((ADC1CN & 0x20) == 0x00); /* Wait for conversion to complete */
    return ADC1; /* Return digital value in ADC1 register */
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

void Read_Ranger_and_Compass(void)
{
	unsigned char Data1[1];
    unsigned char addr = 0; 
	wait_for_100ms();
   
    distance = (int)ReadRanger();
    heading = (int)ReadCompass(); //read data from compass and ranger
    printf("Distance is %u\r\n",distance);
    printf("Heading is %u\r\n",heading); //print compass and ranger read
    
    Data1[0] = 0x51;
    addr = 0xE0;
    i2c_write_data(addr,0,Data1,1); //pin for ranger
    
     //convert reading to PW change
}

void pause(void)
{
    PCA_counts = 0;
    while(PCA_counts <= 1);
   
}

void wait_for_100ms(void)
{
    PCA_counts = 0;
    while(PCA_counts <= 5);
   
    
}//wait for 5 PCA counts which last 100ms

void wait_for_1s(void)
{
    PCA_counts = 0;
    while(PCA_counts <= 50);
}

void heading_to_east(void)
{
    
    error = heading - desired_heading;
    printf("desire heading is %d\r\n",desired_heading);
    printf("error is %d\r\n",error);
    if(error < -1800)
    {
        error += 3600;
    }
    else if(error > 1800)
    {
        error -= 3600;
    }
    PW1 = PW_CENTER + error* (steering_gain-48) /2;
    if(PW1 < PW_MIN)
    {
        PW1 = PW_MIN;
    }
    else if(PW1 > PW_MAX)
    {
        PW1 = PW_MAX;
    }

}//control servo input signal PW according to compass reading

void Execute_PW()
{
    printf("Speed_PW: %u\r\n", PW2);
    printf("Heading_PW: %u\r\n\n", PW1); //print current speed and heading PW
    /*while(SS)
    {
        PW2 = PW_CENTER = PW1;
        PCA0CP2 = 0xFFFF - PW2;
        PCA0CP0 = 0xFFFF - PW1;
         //while SS is off, turn off motor
    }
    */
    PCA0CP2 = 0xFFFF - PW2;
    PCA0CP0 = 0xFFFF - PW1; //change PCA compare module value

}

char get_speed(void)
{
        
        lcd_clear();
        lcd_print("Please enter intial speed level\r\n");
        pause();
        keypad = read_keypad();
        pause();
        check = 13;
        while(keypad == 0xFF)
        {
            pause();
            keypad = read_keypad();
            pause(); 
        
        }
         // If the keypad is read too frequently (no delay), it will
                    // lock up and stop responding. Must power down to reset.
        
        if (keypad != 0xFF)   // keypad = -1 if no key is pressed
        {           // Note: fast read results in multiple lines on terminal
                    // A longer delay will reduce multiple keypad reads but a
                    // better approach is to wait for a -1 between keystrokes.
             // This pauses for 1 PCA0 counter clock cycle (20ms) 
        
            lcd_clear();
            lcd_print("Your speed level is: %c\r",keypad);  
            wait_for_1s();
        
            if(keypad == 0)printf("   **Wire Connection/XBR0 Error**   ");
            while(check != 0xFF)
            {
                check = read_keypad();
            }
        }      
        return keypad;     // A returned value of 0 (0x00) indicates wiring error
}

signed int get_desired_heading(void)
{
        int a;
        int keypad_int;
        lcd_clear();
        lcd_print("Please enter desired heading\r\n");
        pause();
        keypad = read_keypad();
        pause();
        check = 13;
        while(keypad == 0xFF)
        {
            pause();
            keypad = read_keypad();
            pause(); 
        
        }
         // If the keypad is read too frequently (no delay), it will
                    // lock up and stop responding. Must power down to reset.
        
        if (keypad != 0xFF)   // keypad = -1 if no key is pressed
        {           // Note: fast read results in multiple lines on terminal
                    // A longer delay will reduce multiple keypad reads but a
                    // better approach is to wait for a -1 between keystrokes.
             // This pauses for 1 PCA0 counter clock cycle (20ms) 
            keypad_int = (int) keypad;
            a = 900*(keypad_int - 49);
            lcd_clear();
            lcd_print("Your desired heading is: %u\r",a);  
            wait_for_1s();
        
            if(keypad == 0)printf("   **Wire Connection/XBR0 Error**   ");
            while(check != 0xFF)
            {
                check = read_keypad();
            }
        }      
        return a;     // A returned value of 0 (0x00) indicates wiring error
}

int get_steering_gain(void)
{
        int a;
        lcd_clear();
        lcd_print("Please enter steering gain\r\n");
        pause();
        keypad = read_keypad();
        pause();
        check = 13;
        while(keypad == 0xFF)
        {
            pause();
            keypad = read_keypad();
            pause(); 
        
        }
         // If the keypad is read too frequently (no delay), it will
                    // lock up and stop responding. Must power down to reset.
        
        if (keypad != 0xFF)   // keypad = -1 if no key is pressed
        {           // Note: fast read results in multiple lines on terminal
                    // A longer delay will reduce multiple keypad reads but a
                    // better approach is to wait for a -1 between keystrokes.
             // This pauses for 1 PCA0 counter clock cycle (20ms) 
        
            lcd_clear();
            lcd_print("Your steering gain is: %c\r",keypad);  
            wait_for_1s();
        
            if(keypad == 0)printf("   **Wire Connection/XBR0 Error**   ");
            while(check != 0xFF)
            {
                check = read_keypad();
            }
        }      
        a = (int)keypad;
        return a;     // A returned value of 0 (0x00) indicates wiring error
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
