/*
 * RGB_LED_DRIVER_BOARD.c
 *
 * Created: 5/25/2015 2:42:35 PM
 *  Author: John
 */ 


#define F_CPU 16000000L

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "ADC.c"

#define GREEN	OCR1A
#define RED		OCR1B
#define BLUE	OCR2A

#define SW0		2
#define SW1		3
#define SW2		4

#define EQ_RESET	6
#define STROBE		5

uint8_t AudioLevel[7] = {0};

uint8_t out_low;
uint8_t out_mid;
uint8_t out_high;

const int fade_led_delay	= 300;		//in milliseconds
const int strobe_led_delay	= 75;		//in milliseconds

volatile uint8_t display_select = 0xFF;
/*
Display Select Decoder:

0x00: MusicOnLed()
0x01: FadeOnLed()
0x02: StrobeOnLed()
.
.
.
0xFF: Main();

*/

//Initialization Functions
void InitTimer1(void);
void InitTimer2(void);
void InitExtInt(void);
void InitIO(void);

//Main Functions
void FadeOnLed(void);
void StrobeOnLed(uint8_t, uint8_t, uint8_t);
void MusicOnLed(void);
void ColorOnLed(uint8_t, uint8_t, uint8_t);

//Supporting functions
void IncrementUp(uint8_t, uint8_t);
void IncrementDown(uint8_t, uint8_t);
void GetAudioBandLevel(void);

int main(void)
{
	InitIO();
	init_ADC();
	InitTimer1();
	InitTimer2();
	InitExtInt();
	
    while(1)
    {
        switch(display_select)
		{
			case 0x00:
				MusicOnLed();
			break;
			case 0x01:
				FadeOnLed();
			break;
			case 0x02:
				StrobeOnLed(100, 100, 100);
			break;
			/*
			.
			.	TODO:: Sync up pin change interrupts for the last push button
			.
			*/
			case 0xFF:
				ColorOnLed(1,0,1);
			break;
		}
    }
}

void InitTimer1(void)
{
	TCCR1A |= (1 << COM1A1)|(1 << COM1B1)|(1 << WGM10);		//Sets timer1 clear on compare match (output low level) for output compares A and B
	TCCR1B |= (1 << CS12);									//Sets prescalar to clk/256								
	//TIMSK1 |= (1 << OCIE1B)|(1 << OCIE1A);				//Enables interrupts for OCR1A and OCR1B (not required)
}

void InitTimer2(void)
{
	TCCR2A |= (1 << COM2A1)|(1 << COM2B1)|(1 << WGM20);		//Sets timer2 clear on compare match (output low level) for output compares A and B
	TCCR2B |= (1 << CS22)|(1 << CS21);						//Sets prescalar to clk/256
	//TIMSK2 |= (1 << OCIE2A);								//Enables interrupts for OCR2A (not required)
}

void InitExtInt(void)
{
	EICRA	|= (1 << ISC11)|(1 << ISC01);		//Enables falling edge interrupt triggering on INT1 and INT0
	EIMSK	|= (1 << INT1)|(1 << INT0);			//Enables external interrupts on INT1 and INT0
	sei();										//Enables global interrupts
}

void InitIO(void)
{
	DDRB	= 0x3F;											//Data direction for PORTB set to output
	DDRD	&= ~(1 << SW0) | ~(1 << SW1) | ~(1 << SW2);		//Data direction for SWs set to input
	PORTD	|= (1 << SW0) | (1 << SW1) | (1 << SW2);		//Internal pullup resistors turned on
	DDRC	= 0x00;											//Data direction for PORTC set as input
	PORTC	= 0x3F;											//Internal pullup resistors turned on
}

void FadeOnLed(void)
{
	uint8_t increment_amount = 120;			//max brightness of each color
	ColorOnLed(increment_amount, 0, 0);		//sets first color up as red
	
	/*
	RED		= 1
	GREEN	= 2
	BLUE	= 3
	*/
	while(display_select == 0x01)
	{
		IncrementUp(2, increment_amount);
		IncrementDown(1, increment_amount);
		IncrementUp(3, increment_amount);
		IncrementUp(1, increment_amount);
		IncrementDown(2, increment_amount);
		IncrementDown(1, increment_amount);
		IncrementDown(3, increment_amount);
		IncrementUp(1, increment_amount);
		IncrementUp(2, increment_amount);
		IncrementDown(2, increment_amount);
	}
}

void StrobeOnLed(uint8_t red, uint8_t green, uint8_t blue)
{
	ColorOnLed(red, green, blue);
	_delay_ms(strobe_led_delay);
	ColorOnLed(0, 0, 0);
	_delay_ms(strobe_led_delay);
}

void MusicOnLed(void)
{
		
	GetAudioBandLevel();
	out_low		= (AudioLevel[0] + AudioLevel[1]) / 2;						// Average of two Lowest Bands
	out_mid		= (AudioLevel[2] + AudioLevel [3] + AudioLevel[4]) / 3;		// Average of three Middle Bands
	out_high	= (AudioLevel[5] + AudioLevel[6]) / 2;						// Average of two Highest Bands
		
	float low_multiplier	= 1.5;
	float mid_multiplier	= 1;
	float high_multiplier	= 1;
		
	uint8_t led_out_low		=	out_low * low_multiplier;
	uint8_t led_out_mid		=	out_mid * mid_multiplier;
	uint8_t led_out_high	=	out_high * high_multiplier;
		
	if(out_low >= (255 / low_multiplier))
	{
		led_out_low = 255;		//set to max brightness if overflow condition would otherwise occur
	}
	if(out_mid >= (255 / mid_multiplier))
	{
		led_out_mid = 255;		//set to max brightness if overflow condition would otherwise occur
	}
	if(out_high >= (212 / high_multiplier))
	{
		led_out_high = 255;		//set to max brightness if overflow condition would otherwise occur
	}
		
	//assign to outputs
	GREEN	= led_out_low;
	RED		= led_out_mid;
	BLUE	= led_out_high;
	//_delay_ms(10);			//old delay before menu implementation
}

void GetAudioBandLevel(void)
{
	uint8_t audio_band = 0;
	DDRD	|=	(1 << STROBE)|(1 << EQ_RESET);				//PORTD bit strobe and reset pins output
	PORTD	&=	~((1 << STROBE)|(1 << EQ_RESET));			//sets strobe and reset low
	PORTD	|=	(1 << EQ_RESET);							//reset goes high
	_delay_us(100);											//delay 100usec for setup time req
	PORTD	|=	(1 << STROBE);								//strobe goes high
	_delay_us(50);											//strobe delays
	PORTD	&=	~(1 << STROBE);								//strobe goes low
	_delay_us(50);											//strobe delays
	PORTD	&=	~(1 << EQ_RESET);							//reset reset
	PORTD	|=	(1 << STROBE);								//strobe goes high
	_delay_us(50);
	
	for(audio_band = 0; audio_band < 7; audio_band++)
	{		
		PORTD	&=	~(1 << STROBE);				//resets (set strobe pin low (active))
		_delay_us(30);							//setup time for capture
		AudioLevel[audio_band] = read_ADC();	//reads 8 bit resolution audio level from audio bandpass filter
		PORTD	|=	(1 << STROBE);					//sets (set strobe pin high again)
		_delay_us(50);							//not sure if needed - check datasheet
	}	
}

void IncrementUp(uint8_t led, uint8_t max_val)
{
	uint8_t i = 0;
	
	while((i < max_val) & (display_select == 0x01))
	{
		switch(led)
		{
			case 1:
				RED		= i;
			break;
			case 2:
				GREEN	= i;
			break;
			case 3:
				BLUE	= i;
			break;
		}
		i++;
		_delay_ms(fade_led_delay);
	}
}

void IncrementDown(uint8_t led, uint8_t max_val)
{
	uint8_t i = 0;
	
	while((i < max_val) & (display_select == 0x01))
	{
		switch(led)
		{
			case 1:
			RED		= max_val - i;
			break;
			case 2:
			GREEN	= max_val - i;
			break;
			case 3:
			BLUE	= max_val - i;
			break;
		}
		i++;
		_delay_ms(fade_led_delay);
	}
}

void ColorOnLed(uint8_t red, uint8_t green, uint8_t blue)
{
	RED = red;
	GREEN = green;
	BLUE = blue;
}

ISR(INT0_vect)
{
	display_select = 0x00;
}

ISR(INT1_vect)
{
	display_select = 0x01; 
}

/*
//Not required unless using overflow interrupts
ISR(TIMER1_COMPA_vect)
{
	
}

ISR(TIMER1_COMPB_vect)
{
	
}

ISR(TIMER2_COMPA_vect)
{
	
}
*/