
#include <p18f14k50.h>

#include "adc.h"

struct adc _adc[ADC_CHANNELS];
bool _adc_sampling;
uint8_t _adc_current_channel;

void adc_init(void) {
	uint8_t i;

	// Channel 0 -- Analog0 ANS6/RC2 	
	TRISCbits.TRISC2 = 1;
	ANSELbits.ANS6 = 1;

	// Channel 1 -- Analog1 ANS5/RC1 
	TRISCbits.TRISC1 = 1;
	ANSELbits.ANS5 = 1;

	// Channel 2 -- Analog2 ANS4/RC0 	
	TRISCbits.TRISC0 = 1;
	ANSELbits.ANS4 = 1;

	// Channel 3 -- Analog3 ANS9/RC7
	TRISCbits.TRISC7 = 1;
	ANSELHbits.ANS9 = 1;

	// Channel 4 -- Analog4 ANS8/RC6 
	TRISCbits.TRISC6 = 1;
	ANSELHbits.ANS8 = 1;

	// Channel 5 -- Analog5 ANS7/RC3 
	TRISCbits.TRISC3 = 1;
	ANSELbits.ANS7 = 1;

	// Disable ADC channels used by other modules.
	// RB5 is used by the UART
	ANSELHbits.ANS11 = 0;
	// RB4 is used by the I2C
	ANSELHbits.ANS10 = 0;

	for (i = 0; i < ADC_CHANNELS; i++) {
		_adc[i].updated = 0;
		_adc[i].enabled = 1;
		_adc[i].data.value = 0xffff;
	}
	_adc_current_channel = 0;
	_adc_sampling = false;
}

void adc_service(void) {
	uint8_t next_channel = 0;

	if (ADCON0bits.GO)
		return;

	if (_adc_sampling) {
		_adc[_adc_current_channel].updated++;
		_adc[_adc_current_channel].data.l = ADRESL;
		_adc[_adc_current_channel].data.h = ADRESH;
		_adc_sampling = false;
	}

	// Find the next enabled channel to start sampling on
	next_channel = _adc_current_channel+1;
	while(next_channel != _adc_current_channel) {
		next_channel = next_channel % ADC_CHANNELS;

		if (_adc[next_channel].enabled)
			break;

		next_channel++;
	}

	if (!_adc[next_channel].enabled)
		return;

	_adc_current_channel = next_channel;
	//Select channel
	switch(_adc_current_channel){
	case 0:
		ADCON0bits.CHS0 = 1;
		ADCON0bits.CHS1 = 0;
		ADCON0bits.CHS2 = 0;
		ADCON0bits.CHS3 = 1;	
		break;
	case 1:
		ADCON0bits.CHS0 = 0;
		ADCON0bits.CHS1 = 0;
		ADCON0bits.CHS2 = 0;
		ADCON0bits.CHS3 = 1;
		break;
	case 2:
		ADCON0bits.CHS0 = 1;
		ADCON0bits.CHS1 = 1;
		ADCON0bits.CHS2 = 1;
		ADCON0bits.CHS3 = 0;
		break;
	case 3:
		ADCON0bits.CHS0 = 0;
		ADCON0bits.CHS1 = 0;
		ADCON0bits.CHS2 = 1;
		ADCON0bits.CHS3 = 0;
		break;
	case 4:
		ADCON0bits.CHS0 = 1;
		ADCON0bits.CHS1 = 0;
		ADCON0bits.CHS2 = 1;
		ADCON0bits.CHS3 = 0;
		break;
	case 5:
		ADCON0bits.CHS0 = 0;
		ADCON0bits.CHS1 = 1;
		ADCON0bits.CHS2 = 1;
		ADCON0bits.CHS3 = 0;
		break;
	default:
		return;
	}
		
	//Configure ADC Module
	ADCON2 = 0xBF; 
	ADCON1 = 0x00;

	//Turn off ADC interrupt
	PIE1bits.ADIE = 0;

	//Turn on ADC Module
	ADCON0bits.ADON = 1;

	//Start Conversion
	ADCON0bits.GO = 1;
	_adc_sampling = true;
}
