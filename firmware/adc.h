#ifndef ADC_H
#define	ADC_H

#include "types.h"

#define ADC_CHANNELS 6u

struct adc {
        uint8_t enabled;
        uint8_t updated;
        union {
                uint16_t value;
                struct {
                        uint8_t l;
                        uint8_t h;
                };
        } data;
};
extern struct adc _adc[ADC_CHANNELS];

void adc_init(void);
void adc_service(void);

#endif	/* ADC_H */
