#ifndef SENSOR_NODE_APP_CONFIG_H
#define SENSOR_NODE_APP_CONFIG_H

#include <stdbool.h>

/* All application timing values use milliseconds unless explicitly stated. */
#define APP_ACQUISITION_PERIOD_MS       (5UL * 60UL * 1000UL)
#define APP_TX_PERIOD_MS                (30UL * 60UL * 1000UL)
#define APP_SENSOR_WARMUP_MS            (1000UL)

#define APP_ADC_SAMPLE_RATE_HZ          (20000UL)
#define APP_ACQUISITION_DURATION_MS     (1000UL)
#define APP_ADC_DMA_BUFFER_SAMPLES      (512U)
#define APP_ADC_DMA_HALF_SAMPLES        (APP_ADC_DMA_BUFFER_SAMPLES / 2U)
#define APP_ACQUISITION_TOTAL_SAMPLES   \
  ((APP_ADC_SAMPLE_RATE_HZ * APP_ACQUISITION_DURATION_MS) / 1000UL)

/* Q15 FIR followed by integer decimation. Replace these example coefficients. */
#define APP_FIR_TAP_COUNT               (8U)
#define APP_DECIMATION_FACTOR           (4U)
#define APP_FIR_COEFFICIENTS_Q15        \
  { 4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096 }

#define APP_LORAWAN_PORT                (10U)
#define APP_LORAWAN_CONFIRMED           (false)
#define APP_TX_PAYLOAD_MAX_SIZE         (32U)

#if ((APP_ADC_DMA_BUFFER_SAMPLES % 2U) != 0U)
#error "APP_ADC_DMA_BUFFER_SAMPLES must be even"
#endif

#if (APP_DECIMATION_FACTOR == 0U)
#error "APP_DECIMATION_FACTOR must be greater than zero"
#endif

#endif /* SENSOR_NODE_APP_CONFIG_H */
