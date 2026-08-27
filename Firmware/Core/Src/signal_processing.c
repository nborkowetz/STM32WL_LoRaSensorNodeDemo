#include "signal_processing.h"

#include <limits.h>
#include <string.h>

#include "app_config.h"

static const int16_t fir_coefficients[APP_FIR_TAP_COUNT] = APP_FIR_COEFFICIENTS_Q15;

typedef struct
{
  int16_t history[APP_FIR_TAP_COUNT];
  uint16_t history_index;
  uint16_t decimation_phase;
  int16_t minimum;
  int16_t maximum;
  int64_t sum;
  uint64_t sum_of_squares;
  uint32_t input_count;
  uint32_t output_count;
} SignalProcessingContext_t;

static SignalProcessingContext_t context;

static uint32_t IntegerSqrt64(uint64_t value)
{
  uint64_t result = 0U;
  uint64_t bit = (uint64_t) 1U << 62;

  while (bit > value)
  {
    bit >>= 2;
  }

  while (bit != 0U)
  {
    if (value >= (result + bit))
    {
      value -= result + bit;
      result = (result >> 1) + bit;
    }
    else
    {
      result >>= 1;
    }
    bit >>= 2;
  }

  return (uint32_t) result;
}

void SignalProcessing_Init(void)
{
  SignalProcessing_Reset();
}

void SignalProcessing_Reset(void)
{
  memset(&context, 0, sizeof(context));
  context.minimum = INT16_MAX;
  context.maximum = INT16_MIN;
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
    int64_t accumulator = 0;
    uint16_t tap;
    int16_t signed_sample = (int16_t) ((int32_t) adc_samples[sample_index] - 2048);

    context.history[context.history_index] = signed_sample;
    context.history_index = (uint16_t) ((context.history_index + 1U) % APP_FIR_TAP_COUNT);
    context.input_count++;

    if (context.decimation_phase == 0U)
    {
      uint16_t history_position = context.history_index;

      for (tap = 0U; tap < APP_FIR_TAP_COUNT; tap++)
      {
        history_position = (history_position == 0U) ? (APP_FIR_TAP_COUNT - 1U) : (history_position - 1U);
        accumulator += (int32_t) context.history[history_position] * fir_coefficients[tap];
      }

      accumulator >>= 15;
      if (accumulator > INT16_MAX)
      {
        accumulator = INT16_MAX;
      }
      else if (accumulator < INT16_MIN)
      {
        accumulator = INT16_MIN;
      }

      if ((int16_t) accumulator < context.minimum)
      {
        context.minimum = (int16_t) accumulator;
      }
      if ((int16_t) accumulator > context.maximum)
      {
        context.maximum = (int16_t) accumulator;
      }

      context.sum += accumulator;
      context.sum_of_squares += (uint64_t) (accumulator * accumulator);
      context.output_count++;
    }

    context.decimation_phase++;
    if (context.decimation_phase >= APP_DECIMATION_FACTOR)
    {
      context.decimation_phase = 0U;
    }
  }
}

void SignalProcessing_Finalize(SignalStatistics_t *statistics)
{
  if (statistics == NULL)
  {
    return;
  }

  memset(statistics, 0, sizeof(*statistics));
  statistics->input_sample_count = context.input_count;
  statistics->output_sample_count = context.output_count;

  if (context.output_count != 0U)
  {
    statistics->minimum = context.minimum;
    statistics->maximum = context.maximum;
    statistics->mean_q15 = (int32_t) (context.sum / (int64_t) context.output_count);
    statistics->rms_q15 = IntegerSqrt64(context.sum_of_squares / context.output_count);
  }
}
