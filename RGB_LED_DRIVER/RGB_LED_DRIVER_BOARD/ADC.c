/***********************************************************************************
ECET 179
LAB 12
Richardson

	Analog to Digital.C
	
	This script gets ADC value from built in ADC on ATMEGA328

Created by: John Fritz

		modified clock 
************************************************************************************/

uint8_t read_ADC(void);
void init_ADC(void);

void init_ADC(void)
{
                                // setup the Analog to Digital Converter
	ADMUX = 0x40;		// start by selecting the voltage reference - Avcc
	ADMUX = ADMUX | 0x00;   // Select the ADC channel - channel 0
	ADMUX = ADMUX | 0x20;	// set for Left Justified - Only using 8 bit of resolution
	ADMUX |= (1 << REFS0);	//Sets reference to AVcc with external cap on AREF
	
	ADCSRA = 0x04;	// select the ADC clock frequency - Clock / 128
	ADCSRA = ADCSRA | 0x80;	// enable the ADC

}

	
uint8_t read_ADC(void)
{
	uint8_t value;			// 8-bit value to hold the result
	
	ADMUX = ADMUX & 0xE0;		// clear the channel data

	ADCSRA = ADCSRA | 0x40;	// start a conversion
	
 	while( (ADCSRA & 0x10) == 0 )	// wait for conversion to be completed
	{
	}
	
	value = ADCH;		// get the upper 8-bits
	ADCSRA = ADCSRA | 0x10;	// clear the conversion flag
	
	return value;			// send back the 8-bit result
}
	
uint8_t read_ADC_Channel(uint8_t channel)		//selects 
{
	
	
	uint8_t value;			// 8-bit value to hold the result
	
	ADMUX = ADMUX & 0xE0;		// clear the channel data
	ADMUX = ADMUX | channel;	//selects ADC channel based on value passed to function
	ADCSRA = ADCSRA | 0x40;	// start a conversion
	
	while( (ADCSRA & 0x10) == 0 )	// wait for conversion to be completed
	{
	}
	
	value = ADCH;		// get the upper 8-bits
	ADCSRA = ADCSRA | 0x10;	// clear the conversion flag
	
	return value;			// send back the 8-bit result
}