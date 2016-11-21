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
void Execute_PW(void);
void ADC_Init(void);
unsigned char read_AD_input(unsigned char n);
void get_error(void);
void set_neutral_for_1s(void);
void calibrate_thrust_angle(void);
int get_Kp(void);
int get_Kd(void);

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
__xdata unsigned int PW_CENTER = 2765;
__xdata unsigned int PW_MIN = 2028;
__xdata unsigned int PW_MAX = 3502;
unsigned int rudder_pw;

unsigned int calculated_rudder_pw;
unsigned int PCA_counts;
int heading = 0;
int distance = 0;
int distance_input = 0;
char keypad;
char check;
unsigned int speed_level_int;
int desired_heading = 0;
int Kp;
int Kd;
int error;
int previous_error;
int speed;
long tmp_pw;
unsigned int print_counts = 0;
unsigned char AD_read;
unsigned int AD_value;
unsigned int battery_read;



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
	printf("start\r\n");
    Kp = get_Kp();
    Kd = get_Kd();
    set_neutral_for_1s();
    calibrate_thrust_angle();
	printf("desired_heading  distance  error  rudder_pw  battery_read\r\n");
    while(1)
    { // AD_value * 2.4 / gain / 256 * 11.5k ohm / 1.5 k ohm * 1000
        Read_Ranger_and_Compass();
        if(distance > 90) distance_input = 90;
        else if(distance < 10) distance_input = 10;
        else distance_input = distance;
        desired_heading = 45*(distance_input - 50) + 1800;
        get_error();
	
        speed = error - previous_error;
        while((speed > 200) || (speed < -200))
        {
            rudder_pw = PW_CENTER;
            Execute_PW();
			Read_Ranger_and_Compass();
            get_error();
            previous_error = error;
            Read_Ranger_and_Compass();
            get_error();
            speed = error - previous_error;
        } 
        tmp_pw = (long)Kp*error/10 + (long)Kd*speed + PW_CENTER; 
        calculated_rudder_pw = (unsigned int)tmp_pw;
        if (tmp_pw > (long)PW_MAX) tmp_pw = PW_MAX; 
        else if (tmp_pw < (long)PW_MIN) tmp_pw = PW_MIN; 
        rudder_pw = (unsigned int)tmp_pw; 
        
        previous_error = error; 
        Execute_PW();
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
    P1MDIN &= ~0x08; /* Set P1.3 for analog input */
    P1MDOUT &= ~0x08; /* Set P1.3 to open drain */
    P1 |= 0x08; /* Send logic 1 to input pin P1.3 */

    P0MDOUT |= 0xF0;  //set output pin for CEX0 or CEX2 in push-pull mode
   
}

//-----------------------------------------------------------------------------
// XBR0_Init
//-----------------------------------------------------------------------------
//
// Set up the crossbar
//
void XBR0_Init()
{
    XBR0 = 0x25;  //configure crossbar as directed in the laboratory

}

//-----------------------------------------------------------------------------
// PCA_Init
//-----------------------------------------------------------------------------
//
// Set up Programmable Counter Array
//
void PCA_Init(void)
{
    PCA0CPM0 = PCA0CPM1 = PCA0CPM2= PCA0CPM3= 0xC2;// reference to the sample code in Example 4.5 -Pulse Width Modulation 
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
    PCA_counts++;
	print_counts++; // reference to the sample code in Example 4.5 -Pulse Width Modulation 
    // implemented using the PCA (Programmable Counter Array), p. 50 in Lab Manual.
}

void Read_Ranger_and_Compass(void)
{
	unsigned char Data1[1];
    unsigned char addr = 0; 
	wait_for_100ms();
   
    distance = (int)ReadRanger();
    heading = (int)ReadCompass(); //read data from compass and ranger
     //print compass and ranger read
    
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


void Execute_PW()
{	unsigned int thrust_pw = 5530 - rudder_pw;
	if(print_counts > 40)
	{

        printf("%u               %u        %d        %u         %u\r\n",desired_heading, distance, error, rudder_pw, battery_read);//print current speed and heading PW
        print_counts = 0;
	}

    
    PCA0CP0 = PCA0CP2 = 0xFFFF - rudder_pw;
    PCA0CP3 = 0xFFFF - thrust_pw; //change PCA compare module value

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

void get_error(void)
{
    error = desired_heading-heading;
    if(error > 1800) error -= 3600;
    if(error < -1800) error += 3600;
}

void set_neutral_for_1s(void)
{
    PCA0CP0 = 0xFFFF - PW_CENTER;
    PCA0CP2 = 0xFFFF - PW_CENTER;
    PCA0CP3 = 0xFFFF - PW_CENTER;
    wait_for_1s();
}

void calibrate_thrust_angle(void)
{
    char input = 'a';
	unsigned int angle_pw = 2275;
	unsigned int a = 2028;
	unsigned int b = 3502;
    printf("Thrust angle calibration start. \r\n");
  
    while(input != 'c')
    {
        input = getchar();
        if(input == 'a')
        {
            angle_pw -= 10;
            if(angle_pw < a) angle_pw = a;
        }

        else if(input == 'b')
        {
            angle_pw += 10;
            if(angle_pw > b) angle_pw = b;
        }
        PCA0CP1 = 0xFFFF - angle_pw;
        printf("angle_pw is %u",angle_pw);
    }
    printf("Calibration Completed. \r\n");
}

int get_Kp(void)
{
    char input;
    int Kp_input;
    printf("Please set Kp value\r\n");
    input = getchar();
    while((input != 'a')&&(input != 'b')&&(input != 'c')&&(input != 'd'))
    {
        printf("Please enter another value\r\n");
        input = getchar();
    }
    if(input == 'a') Kp_input = 1;
    else if(input == 'b') Kp_input = 5;
    else if(input == 'c') Kp_input = 30;
    else if(input == 'd') Kp_input = 120;
    printf("The value of Kp is %u\r\n",Kp_input);
    return Kp_input;
}    

int get_Kd(void)
{
    char input;
    int Kd_input;
    printf("Plese set Kd value\r\n");
    input = getchar();
    while((input != 'a')&&(input != 'b')&&(input != 'c')&&(input != 'd'))
    {
        printf("Please enter another value\r\n");
        input = getchar();
    }
    if(input == 'a') Kd_input = 0;
    else if(input == 'b') Kd_input = 10;
    else if(input == 'c') Kd_input = 70;
    else if(input == 'd') Kd_input = 180;
    printf("The value of Kd is %u\r\n",Kd_input);
    return Kd_input;

}