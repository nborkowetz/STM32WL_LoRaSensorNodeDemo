#ifndef SENSOR_NODE_H
#define SENSOR_NODE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
  void (*SensorPowerSet)(bool enabled);
  bool (*AdcStartDma)(uint16_t *buffer, uint32_t sample_count);
  void (*AdcStopDma)(void);
  uint32_t (*GetTimeMs)(void);
} SensorNodePlatform_t;

typedef enum
{
  SENSOR_NODE_IDLE,
  SENSOR_NODE_WARMUP,
  SENSOR_NODE_ACQUISITION,
  SENSOR_NODE_PROCESSING,
  SENSOR_NODE_ERROR
} SensorNodeState_t;

void SensorNode_Init(const SensorNodePlatform_t *platform);
void SensorNode_Process(void);
SensorNodeState_t SensorNode_GetState(void);

/* Call only from the corresponding HAL ADC/DMA callbacks. */
void SensorNode_OnAdcHalfComplete(void);
void SensorNode_OnAdcComplete(void);
void SensorNode_OnAdcError(void);

#endif /* SENSOR_NODE_H */
