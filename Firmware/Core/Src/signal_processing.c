#include "signal_processing.h"

#include <limits.h>
#include <string.h>

typedef struct
{
  int32_t x1;
  int32_t x2;
  int32_t y1;
  int32_t y2;
} BiquadState_t;

typedef struct
{
  uint32_t discard_count;
  BiquadState_t biquad[APP_IIR_BIQUAD_STAGE_COUNT];
  int64_t sum;
  uint32_t input_count;
  uint32_t output_count;
} SignalProcessingContext_t;

static const int32_t biquad_coefficients_q30[APP_IIR_BIQUAD_STAGE_COUNT][5] =
  APP_IIR_BIQUAD_COEFFICIENTS_Q30;
static SignalProcessingContext_t contexts[APP_ADC_CHANNEL_COUNT];
static uint16_t next_channel;

static uint16_t QStateToAdc12(int64_t value)
{
  const int64_t maximum = (int64_t) 4095 << APP_IIR_STATE_FRACTIONAL_BITS;

  if (value >= maximum)
  {
    return 4095U;
  }
  if (value <= 0)
  {
    return 0U;
  }
  value += (int64_t) 1 << (APP_IIR_STATE_FRACTIONAL_BITS - 1U);
  return (uint16_t) (value >> APP_IIR_STATE_FRACTIONAL_BITS);
}

static int32_t ProcessIir(SignalProcessingContext_t *context, int32_t input)
{
  uint16_t stage;
  int32_t output = input;

  for (stage = 0U; stage < APP_IIR_BIQUAD_STAGE_COUNT; stage++)
  {
    BiquadState_t *state = &context->biquad[stage];
    const int32_t *coefficient = biquad_coefficients_q30[stage];
    int64_t accumulator = ((int64_t) coefficient[0] * input) +
                          ((int64_t) coefficient[1] * state->x1) +
                          ((int64_t) coefficient[2] * state->x2) -
                          ((int64_t) coefficient[3] * state->y1) -
                          ((int64_t) coefficient[4] * state->y2);

    if (accumulator >= 0)
    {
      accumulator = (accumulator + (1LL << 29)) >> 30;
    }
    else
    {
      accumulator = -(((-accumulator) + (1LL << 29)) >> 30);
    }
    if (accumulator > INT32_MAX)
    {
      output = INT32_MAX;
    }
    else if (accumulator < INT32_MIN)
    {
      output = INT32_MIN;
    }
    else
    {
      output = (int32_t) accumulator;
    }
    state->x2 = state->x1;
    state->x1 = input;
    state->y2 = state->y1;
    state->y1 = output;
    input = output;
  }
  return output;
}

void SignalProcessing_Init(void)
{
  SignalProcessing_Reset();
}

void SignalProcessing_Reset(void)
{
  uint16_t channel;

  memset(contexts, 0, sizeof(contexts));
  next_channel = 0U;
  for (channel = 0U; channel < APP_ADC_CHANNEL_COUNT; channel++)
  {
    contexts[channel].discard_count = APP_IIR_DISCARD_SAMPLE_COUNT;
  }
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
    int32_t filtered;

    context->input_count++;
    filtered = ProcessIir(context,
      (int32_t) adc_samples[sample_index] << APP_IIR_STATE_FRACTIONAL_BITS);
    if (context->discard_count != 0U)
    {
      context->discard_count--;
    }
    else
    {
      context->sum += filtered;
      context->output_count++;
    }

    next_channel = (uint16_t) ((next_channel + 1U) % APP_ADC_CHANNEL_COUNT);
  }
}

void SignalProcessing_Finalize(SignalAverages_t *averages)
{
  uint16_t channel;

  if (averages == NULL)
  {
    return;
  }

  memset(averages, 0, sizeof(*averages));
  for (channel = 0U; channel < APP_ADC_CHANNEL_COUNT; channel++)
  {
    const SignalProcessingContext_t *context = &contexts[channel];

    averages->input_sample_count[channel] = context->input_count;
    averages->averaged_sample_count[channel] = context->output_count;
    if (context->output_count != 0U)
    {
      int64_t rounded_sum = context->sum + ((int64_t) context->output_count / 2);
      averages->mean[channel] = QStateToAdc12(
        rounded_sum / (int64_t) context->output_count);
    }
  }
}
