#ifndef SENSOR_NODE_APP_CONFIG_H
#define SENSOR_NODE_APP_CONFIG_H

#include <stdbool.h>

/* All application timing values use milliseconds unless explicitly stated. */
#define APP_ACQUISITION_PERIOD_MS       (5UL * 60UL * 1000UL)
#define APP_SENSOR_WARMUP_MS            (1000UL)

#define APP_ADC_CHANNEL_COUNT           (4U)
#define APP_ADC_SAMPLE_RATE_HZ          (2000UL) /* Per channel. */
#define APP_ACQUISITION_DURATION_MS     (1000UL)
#define APP_ADC_DMA_BUFFER_SAMPLES      (512U)
#define APP_ADC_DMA_HALF_SAMPLES        (APP_ADC_DMA_BUFFER_SAMPLES / 2U)
#define APP_ACQUISITION_TOTAL_SAMPLES   \
  ((APP_ADC_SAMPLE_RATE_HZ * APP_ADC_CHANNEL_COUNT * APP_ACQUISITION_DURATION_MS) / 1000UL)

#define APP_LORAWAN_PORT                (10U)
#define APP_LORAWAN_CONFIRMED           (false)
#define APP_TX_PAYLOAD_MAX_SIZE         (51U)
#define APP_TELEMETRY_BATCH_COUNT       (3U)
#define APP_TELEMETRY_HEADER_SIZE       (8U)
#define APP_TELEMETRY_VALUE_BITS        (12U)
#define APP_TELEMETRY_CHANNEL_SIZE      (3U) /* 12-bit mean + 12-bit variance. */
#define APP_TELEMETRY_PAYLOAD_SIZE      \
  (APP_TELEMETRY_HEADER_SIZE + \
   (APP_TELEMETRY_BATCH_COUNT * APP_ADC_CHANNEL_COUNT * \
    APP_TELEMETRY_CHANNEL_SIZE))

#if ((APP_ADC_DMA_BUFFER_SAMPLES % 2U) != 0U)
#error "APP_ADC_DMA_BUFFER_SAMPLES must be even"
#endif

#if ((APP_ADC_DMA_HALF_SAMPLES % APP_ADC_CHANNEL_COUNT) != 0U)
#error "Each DMA half-buffer must contain complete ADC scan sequences"
#endif

#if ((APP_ACQUISITION_TOTAL_SAMPLES % APP_ADC_CHANNEL_COUNT) != 0U)
#error "APP_ACQUISITION_TOTAL_SAMPLES must contain complete ADC scan sequences"
#endif

#if (APP_TELEMETRY_VALUE_BITS != 12U)
#error "Telemetry serializer currently requires 12-bit values"
#endif

#if (APP_TX_PAYLOAD_MAX_SIZE < APP_TELEMETRY_PAYLOAD_SIZE)
#error "APP_TX_PAYLOAD_MAX_SIZE is too small for one telemetry record"
#endif

#endif /* SENSOR_NODE_APP_CONFIG_H */
