#include "telemetry.h"

#include <string.h>

static Telemetry_t latest;
static bool telemetry_ready;

static void PutU16Be(uint8_t *destination, uint16_t value)
{
  destination[0] = (uint8_t) (value >> 8);
  destination[1] = (uint8_t) value;
}

static void PutU32Be(uint8_t *destination, uint32_t value)
{
  destination[0] = (uint8_t) (value >> 24);
  destination[1] = (uint8_t) (value >> 16);
  destination[2] = (uint8_t) (value >> 8);
  destination[3] = (uint8_t) value;
}

void Telemetry_Update(uint32_t timestamp_ms, const SignalStatistics_t *statistics)
{
  if (statistics == NULL)
  {
    return;
  }

  latest.sequence++;
  latest.acquisition_timestamp_ms = timestamp_ms;
  latest.signal = *statistics;
  telemetry_ready = true;
}

bool Telemetry_IsReady(void)
{
  return telemetry_ready;
}

size_t Telemetry_Serialize(uint8_t *payload, size_t capacity)
{
  const size_t required_size = 20U;

  if (!telemetry_ready || (payload == NULL) || (capacity < required_size))
  {
    return 0U;
  }

  payload[0] = 1U; /* Payload format version. */
  PutU32Be(&payload[1], latest.sequence);
  PutU32Be(&payload[5], latest.acquisition_timestamp_ms);
  PutU16Be(&payload[9], (uint16_t) latest.signal.minimum);
  PutU16Be(&payload[11], (uint16_t) latest.signal.maximum);
  PutU32Be(&payload[13], (uint32_t) latest.signal.mean_q15);
  PutU16Be(&payload[17], (uint16_t) latest.signal.rms_q15);
  payload[19] = (uint8_t) (latest.signal.output_sample_count & 0xFFU);

  return required_size;
}
