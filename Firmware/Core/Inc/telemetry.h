#ifndef SENSOR_NODE_TELEMETRY_H
#define SENSOR_NODE_TELEMETRY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "signal_processing.h"

typedef struct
{
  uint32_t acquisition_timestamp_ms;
  uint16_t mean[APP_ADC_CHANNEL_COUNT];
  uint32_t variance[APP_ADC_CHANNEL_COUNT];
} Telemetry_t;

bool Telemetry_Update(uint32_t timestamp_ms, const SignalStatistics_t *statistics);
bool Telemetry_IsReady(void);
size_t Telemetry_Serialize(uint8_t *payload, size_t capacity);
void Telemetry_CommitSerialized(void);

#endif /* SENSOR_NODE_TELEMETRY_H */
