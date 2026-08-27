#ifndef SENSOR_NODE_TELEMETRY_H
#define SENSOR_NODE_TELEMETRY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "signal_processing.h"

typedef struct
{
  uint32_t sequence;
  uint32_t acquisition_timestamp_ms;
  SignalStatistics_t signal;
} Telemetry_t;

void Telemetry_Update(uint32_t timestamp_ms, const SignalStatistics_t *statistics);
bool Telemetry_IsReady(void);
size_t Telemetry_Serialize(uint8_t *payload, size_t capacity);

#endif /* SENSOR_NODE_TELEMETRY_H */
