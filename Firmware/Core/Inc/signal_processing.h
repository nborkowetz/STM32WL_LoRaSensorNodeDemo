#ifndef SENSOR_NODE_SIGNAL_PROCESSING_H
#define SENSOR_NODE_SIGNAL_PROCESSING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
  int16_t minimum;
  int16_t maximum;
  int32_t mean_q15;
  uint32_t rms_q15;
  uint32_t input_sample_count;
  uint32_t output_sample_count;
} SignalStatistics_t;

void SignalProcessing_Init(void);
void SignalProcessing_Reset(void);
void SignalProcessing_ProcessBlock(const uint16_t *adc_samples, size_t count);
void SignalProcessing_Finalize(SignalStatistics_t *statistics);

#endif /* SENSOR_NODE_SIGNAL_PROCESSING_H */
