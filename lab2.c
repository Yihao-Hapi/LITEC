#include <c8051_SDCC.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
//Function Prototpes
unsigned char random(void);
void Port_Init(void);
void ADC_Init(void);
unsigned char read_AD_input(unsigned char n);
void Timer_Init(void);
void Timer0_ISR(void) __interrupt 1;
void Interrupt_Init(void);
void stats_led_off(void);
void stats_led_red(void);
void stats_led_green(void);
void leds_buzzer_off(void);
void player_setup(unsigned char player);
unsigned char random(void);
void light_four_leds(void);
void wait_for_one_sec(void);
void input_conversion(void);

//Global Variables
__sbit __at 0xA6 LED0;
__sbit __at 0xA4 LED1;
__sbit __at 0xA3 LED2;
__sbit __at 0xA1 LED3;
__sbit __at 0xA7 STATSBILED0;
__sbit __at 0xA5 STATSBILED1;
__sbit __at 0xA2 PLAYERBILED0;
__sbit __at 0xA0 PLAYERBILED1;
__sbit __at 0xB7 Buzzer;
__sbit __at 0xB5 SS;
__sbit __at 0xB3 PB;

unsigned int counts1;
unsigned int counts2;
unsigned char input_char;
unsigned char AD_VALUE;
unsigned int AD_RESULT;
unsigned int delay_period;
unsigned int delay_count;
unsigned char round = 1;
int player_score = 0;
unsigned int total_counts;
unsigned int trial_number = 1;
unsigned char a;
unsigned char b;
unsigned char player = 1;
char gamer_input;
char real_gamer_input;
unsigned int current1;
unsigned int current2;
int player_total[3] = {0,0,0};

void main(void)
{
 	Sys_Init(); /* Initialize the C8051 board */
 	putchar(' ');
 	Interrupt_Init();
 	Timer_Init();
 	Port_Init(); /* Configure P1.0 for analog input */
	ADC_Init(); /* Initialize A/D conversion */
	printf("Start\r\n");
	leds_buzzer_off();
	stats_led_off();
	PLAYERBILED1 = 0;
	PLAYERBILED0 = 0;
	printf("after");

 	while (round <= 2)
 	{	
 		printf("enter key to read A/D input \r\n");
		input_char = getchar();
 		AD_VALUE = read_AD_input(1); /* Read the A/D value on P1.1 */
 		AD_RESULT = AD_VALUE;
 		delay_period = AD_RESULT*18 +500;
 		delay_count = delay_period /35.5;
 		stats_led_red();
 		printf("\r\nRound %d begins\r\n",round);
 		printf("The A/D input is %d\r\n",AD_VALUE);
 		printf("The delay period is %d ms\r\n",delay_period);
		printf("Delay count is %d\r\n",delay_count);
		round++;
 		while (player <= 3)
 		{
 			player_setup(player);
 			player_score = 0;
 			while(SS);
 			while(trial_number <= 5)
 			{
 				trial_number++;
				leds_buzzer_off();
				light_four_leds();
 				TR0 = 1;
				counts1 = 0;
 				current2 = counts1;
				gamer_input = 0xFF;
 				while(gamer_input == 0xFF & PB & current2 < delay_count)
 				{
 					gamer_input = getchar_nw();
 					current2 = counts1;
 				}
 				if(gamer_input != 0xFF)
 				{
 					input_conversion();
 					if(real_gamer_input == b)
 					{
 						
 						player_score += 100;
 						stats_led_green();
 						current1 = counts1;
 						current2 = counts1 + 7;
 						while(counts1 < current2);

 					}
 					else
 					{
 						stats_led_red();
 						current1 = counts1;
 						current2 = counts1 + 7;
 						while(counts1 < current2)
 						{
 							Buzzer = 0;
 						}
 						Buzzer = 1;
 					}

 				}
 				else if(!PB)
 				{
 					current1 = counts1;
 					current2 = counts1 + 3;
 					while(counts1 < current2);
 				}
 				while(SS)
 				{
 					STATSBILED0 = !STATSBILED0;
 					STATSBILED1 = !STATSBILED1;
 				}
 				stats_led_off();
 				TR0 = 0;
 			}
 			player_total[player-1] += player_score;
 			printf("Yihao Huang scores: %d points;\r\n Zenan JIN scores: %d ;\r\nJiachen Yang scores: %d;\r\n",player_total[0],player_total[1],player_total[2]);
 			player_total[player-1] -= counts2/1.40625;
 			printf("Yihao Huang scores: %d points;\r\n Zenan JIN scores: %d ;\r\nJiachen Yang scores: %d;\r\n Count is: %d\r\n",player_total[0],player_total[1],player_total[2],counts2);
			trial_number = 1;
			player++;
			counts2 = 0;
 		}
		player = 1;
		counts2 = 0;
 	}
 	printf("Yihao Huang scores: %d points;\r\n Zenan JIN scores: %d ;\r\nJiachen Yang scores: %d;\r\n",player_total[0],player_total[1],player_total[2]);
}

void stats_led_off(void)
{
	STATSBILED0 = 0;
	STATSBILED1 = 0;
}

void stats_led_green(void)
{
	STATSBILED0 = 1;
	STATSBILED1 = 0;
}

void stats_led_red(void)
{
	STATSBILED1 = 1;
	STATSBILED0 = 0;
}

void leds_buzzer_off(void)
{
	LED0 = 1;
	LED1 = 1;
	LED2 = 1;
	LED3 = 1;
	Buzzer = 1;
}
void player_setup(unsigned char player)
{
	if(player == 1)
	{
		PLAYERBILED0 = 0;
		PLAYERBILED1 = 0;
		printf("\r\nWelcome YIHAO HUANG\r\n\n");
	}
	else if(player == 2)
	{
		PLAYERBILED0 = 0;
		PLAYERBILED1 = 1;
		printf("\r\nWelcome ZENAN JIN\r\n\n");
	}
	else if(player == 3)
	{
		PLAYERBILED0 = 1;
		PLAYERBILED1 = 0;
		printf("\r\nWelcome JIACHEN YANG\r\n\n");
	}
	printf("Please enter a number corresponding to the 4 leds\r\nPush the pushbottom to pass\r\nTurn off the slide switch to pause\r\n\n");
	wait_for_one_sec();
	printf("Get Ready!!!\r\n");
	wait_for_one_sec();
	printf("3\r\n");
	wait_for_one_sec();
	printf("2\r\n");
	wait_for_one_sec();
	printf("1\r\n");
	wait_for_one_sec();
	printf("Go!\r\n");
	wait_for_one_sec();
}


unsigned char random(void)
{
    return (rand()%16);  
}

void light_four_leds(void)
{
	a = random();
	b = a;
	printf("%d",a);
	LED3 = ~a%2;
	a /= 2;
	LED2 = ~a%2;
	a /= 2;
	LED1 = ~a%2;
	a /= 2;
	LED0 = ~a%2;
}

void wait_for_one_sec(void)
{
	TR0 = 1;
	current1 = counts1;
	current2 = current1+28;
	while(counts1 <= current2);
	TR0 = 0;
	counts1 = 0;
}

void input_conversion(void)
{
	if(gamer_input <= 57)
	{
		real_gamer_input = gamer_input - 48;
	}
	else if(gamer_input >= 97)
	{
		real_gamer_input = gamer_input - 87;
	}
	else
	{
		real_gamer_input = 100;
	}
}

void Port_Init(void)
{
 	P1MDIN &= ~0x02; /* Set P1.1 for analog input */
 	P1MDOUT &= ~0x02; /* Set P1.1 to open drain */
 	P1 |= 0x02; /* Send logic 1 to input pin P1.1 */

 	P2MDOUT |= 0xFF;

 	P3MDOUT |= 0x80;
 	P3MDOUT &= ~0x28;
 	P3 |= 0x28;
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
	ADC1CN = ADC1CN & ~0x20; /* Clear the “Conversion Completed” flag */
	ADC1CN = ADC1CN | 0x10; /* Initiate A/D conversion */
	while ((ADC1CN & 0x20) == 0x00); /* Wait for conversion to complete */
	return ADC1; /* Return digital value in ADC1 register */
}

void Interrupt_Init(void)
{
	IE |= 0x82;
}

void Timer_Init(void)
{
	
	CKCON &= ~0x08;  // Timer0 uses SYSCLK/12
	TMOD &= 0xF0;   // clear the 4 least significant bits
	TMOD |= 0x01;   // Timer0 mode 16
	TR0 = 0;        // Stop Timer0
	TL0 = 0;        // Clear low byte of register T0
	TH0 = 0;        // Clear high byte of register T0

}
void Timer0_ISR(void) __interrupt 1
{
	counts1++;
	counts2++;
}
