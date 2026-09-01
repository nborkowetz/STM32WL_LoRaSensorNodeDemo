#include "signal_processing.h"

#include <string.h>

typedef struct
{
  uint64_t sum;
  uint64_t sum_of_squares;
  uint32_t input_count;
} SignalProcessingContext_t;

static SignalProcessingContext_t contexts[APP_ADC_CHANNEL_COUNT];
static uint16_t next_channel;

static uint16_t SaturateTo12Bits(uint64_t value)
{
  if (value > 4095U)
  {
    return 4095U;
  }
  return (uint16_t) value;
}

void SignalProcessing_Init(void)
{
  SignalProcessing_Reset();
}

void SignalProcessing_Reset(void)
{
  memset(contexts, 0, sizeof(contexts));
  next_channel = 0U;
}

void SignalProcessing_ProcessBlock(const uint16_t *adc_samples, size_t count)
{
  size_t sample_index;

  if (adc_samples == NULL)
  {
    return;
  }

  for (sample_index = 0U; sample_index < count; sample_index++)
  {
    SignalProcessingContext_t *context = &contexts[next_channel];
    uint32_t sample = adc_samples[sample_index] & 0x0FFFU;

    context->input_count++;
    context->sum += sample;
    context->sum_of_squares += (uint64_t) sample * sample;

    next_channel = (uint16_t) ((next_channel + 1U) % APP_ADC_CHANNEL_COUNT);
  }
}

void SignalProcessing_Finalize(SignalStatistics_t *statistics)
{
  uint16_t channel;

  if (statistics == NULL)
  {
    return;
  }

  memset(statistics, 0, sizeof(*statistics));
  for (channel = 0U; channel < APP_ADC_CHANNEL_COUNT; channel++)
  {
    const SignalProcessingContext_t *context = &contexts[channel];

    statistics->input_sample_count[channel] = context->input_count;
    statistics->averaged_sample_count[channel] = context->input_count;
    if (context->input_count != 0U)
    {
      uint64_t count = context->input_count;
      uint64_t variance_numerator =
        (count * context->sum_of_squares) - (context->sum * context->sum);
      uint64_t variance_denominator = count * count;

      statistics->mean[channel] = SaturateTo12Bits(
        (context->sum + (count / 2U)) / count);
      statistics->variance[channel] = SaturateTo12Bits(
        (variance_numerator + (variance_denominator / 2U)) /
        variance_denominator);
    }
  }
}
