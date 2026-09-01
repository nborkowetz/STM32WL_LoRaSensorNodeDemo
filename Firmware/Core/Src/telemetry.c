#include "telemetry.h"

#include <string.h>

#include "app_config.h"

#define TELEMETRY_RECORD_SIZE          (APP_TELEMETRY_PAYLOAD_SIZE)

static Telemetry_t batch[APP_TELEMETRY_BATCH_COUNT];
static uint16_t batch_sequence;
static uint8_t batch_count;
static bool serialized_record_pending;

static void PutU32Be(uint8_t *destination, uint32_t value)
{
  destination[0] = (uint8_t) (value >> 24);
  destination[1] = (uint8_t) (value >> 16);
  destination[2] = (uint8_t) (value >> 8);
  destination[3] = (uint8_t) value;
}

/* Code 0 represents zero. Codes 1..14 represent the inclusive ranges
 * 2^(code-1)..2^code-1 ADC^2. Code 15 represents every value >= 16384 ADC^2. */
static uint8_t EncodeVarianceLog4(uint32_t variance)
{
  uint8_t code = 0U;

  while ((variance != 0U) && (code < 15U))
  {
    variance >>= 1;
    code++;
  }
  return code;
}

static void PutBitsMsbFirst(uint8_t *payload, size_t *bit_offset,
                            uint32_t value, uint8_t bit_count)
{
  uint8_t bit;

  for (bit = bit_count; bit > 0U; bit--)
  {
    size_t byte_index = *bit_offset / 8U;
    uint8_t destination_bit = (uint8_t) (7U - (*bit_offset % 8U));

    if (((value >> (bit - 1U)) & 1U) != 0U)
    {
      payload[byte_index] |= (uint8_t) (1U << destination_bit);
    }
    (*bit_offset)++;
  }
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
  size_t bit_offset = APP_TELEMETRY_HEADER_SIZE * 8U;
  uint8_t acquisition;

  if (!Telemetry_IsReady() || (payload == NULL) ||
      (capacity < TELEMETRY_RECORD_SIZE))
  {
    return 0U;
  }

  memset(payload, 0, TELEMETRY_RECORD_SIZE);
  payload[0] = 6U; /* Payload format version; implies six acquisitions. */
  payload[1] = (uint8_t) batch_sequence;
  PutU32Be(&payload[2], batch[0].acquisition_timestamp_ms);

  for (acquisition = 0U; acquisition < APP_TELEMETRY_BATCH_COUNT; acquisition++)
  {
    uint8_t channel;

    for (channel = 0U; channel < APP_ADC_CHANNEL_COUNT; channel++)
    {
      uint16_t mean10 = (batch[acquisition].mean[channel] >> 2U) & 0x03FFU;
      uint8_t variance4 = EncodeVarianceLog4(
        batch[acquisition].variance[channel]);

      PutBitsMsbFirst(payload, &bit_offset, mean10, APP_TELEMETRY_MEAN_BITS);
      PutBitsMsbFirst(payload, &bit_offset, variance4,
                      APP_TELEMETRY_VARIANCE_BITS);
    }
  }

  serialized_record_pending = true;
  return (bit_offset + 7U) / 8U;
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
