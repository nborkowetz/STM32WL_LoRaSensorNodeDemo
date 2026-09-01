#include "telemetry.h"

#include <string.h>

#include "app_config.h"

#define TELEMETRY_RECORD_SIZE          (APP_TELEMETRY_PAYLOAD_SIZE)

static Telemetry_t batch[APP_TELEMETRY_BATCH_COUNT];
static uint16_t batch_sequence;
static uint8_t batch_count;
static bool serialized_record_pending;

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

bool Telemetry_Update(uint32_t timestamp_ms, const SignalStatistics_t *statistics)
{
  if ((statistics == NULL) || (batch_count >= APP_TELEMETRY_BATCH_COUNT))
  {
    return false;
  }

  if (batch_count == 0U)
  {
    batch_sequence++;
  }
  batch[batch_count].acquisition_timestamp_ms = timestamp_ms;
  memcpy(batch[batch_count].mean, statistics->mean,
         sizeof(batch[batch_count].mean));
  memcpy(batch[batch_count].variance, statistics->variance,
         sizeof(batch[batch_count].variance));
  batch_count++;
  return true;
}

bool Telemetry_IsReady(void)
{
  return (batch_count == APP_TELEMETRY_BATCH_COUNT);
}

size_t Telemetry_Serialize(uint8_t *payload, size_t capacity)
{
  size_t offset = APP_TELEMETRY_HEADER_SIZE;
  uint8_t acquisition;

  if (!Telemetry_IsReady() || (payload == NULL) ||
      (capacity < TELEMETRY_RECORD_SIZE))
  {
    return 0U;
  }

  payload[0] = 4U; /* Payload format version. */
  payload[1] = APP_TELEMETRY_BATCH_COUNT;
  PutU16Be(&payload[2], batch_sequence);
  PutU32Be(&payload[4], batch[0].acquisition_timestamp_ms);

  for (acquisition = 0U; acquisition < APP_TELEMETRY_BATCH_COUNT; acquisition++)
  {
    uint8_t channel;

    for (channel = 0U; channel < APP_ADC_CHANNEL_COUNT; channel++)
    {
      uint16_t mean = batch[acquisition].mean[channel] & 0x0FFFU;
      uint16_t variance = batch[acquisition].variance[channel] & 0x0FFFU;

      payload[offset++] = (uint8_t) (mean >> 4);
      payload[offset++] = (uint8_t) ((mean << 4) | (variance >> 8));
      payload[offset++] = (uint8_t) variance;
    }
  }

  serialized_record_pending = true;
  return offset;
}

void Telemetry_CommitSerialized(void)
{
  if (!serialized_record_pending || !Telemetry_IsReady())
  {
    return;
  }

  batch_count = 0U;
  serialized_record_pending = false;
}
