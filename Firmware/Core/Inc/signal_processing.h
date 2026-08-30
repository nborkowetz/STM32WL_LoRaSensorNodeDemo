#ifndef SENSOR_NODE_SIGNAL_PROCESSING_H
#define SENSOR_NODE_SIGNAL_PROCESSING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_config.h"

typedef struct
{
  uint16_t mean[APP_ADC_CHANNEL_COUNT];
  uint32_t input_sample_count[APP_ADC_CHANNEL_COUNT];
  uint32_t averaged_sample_count[APP_ADC_CHANNEL_COUNT];
} SignalAverages_t;

void SignalProcessing_Init(void);
void SignalProcessing_Reset(void);
void SignalProcessing_ProcessBlock(const uint16_t *adc_samples, size_t count);
void SignalProcessing_Finalize(SignalAverages_t *averages);

#endif /* SENSOR_NODE_SIGNAL_PROCESSING_H */
