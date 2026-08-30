#ifndef SENSOR_NODE_APP_CONFIG_H
#define SENSOR_NODE_APP_CONFIG_H

#include <stdbool.h>

/* All application timing values use milliseconds unless explicitly stated. */
#define APP_ACQUISITION_PERIOD_MS       (5UL * 60UL * 1000UL)
#define APP_SENSOR_WARMUP_MS            (1000UL)

#define APP_ADC_CHANNEL_COUNT           (4U)
#define APP_ADC_SAMPLE_RATE_HZ          (20000UL) /* Per channel. */
#define APP_ACQUISITION_DURATION_MS     (1000UL)
#define APP_ADC_DMA_BUFFER_SAMPLES      (512U)
#define APP_ADC_DMA_HALF_SAMPLES        (APP_ADC_DMA_BUFFER_SAMPLES / 2U)
#define APP_ACQUISITION_TOTAL_SAMPLES   \
  ((APP_ADC_SAMPLE_RATE_HZ * APP_ADC_CHANNEL_COUNT * APP_ACQUISITION_DURATION_MS) / 1000UL)

/* Q30 fixed-point biquads, applied at APP_ADC_SAMPLE_RATE_HZ. Coefficient
 * order: { b0, b1, b2, a1, a2 }, with y = b*x - a*y. Defaults:
 * second-order Butterworth low-pass, fc=100 Hz, fs=20 kHz. */
#define APP_IIR_BIQUAD_STAGE_COUNT      (1U)
#define APP_IIR_STATE_FRACTIONAL_BITS   (16U)
/* Keep this a multiple of 20 ms so the remaining one-second window contains
 * an integer number of 50 Hz mains cycles. */
#define APP_IIR_TRANSIENT_MS            (40U)
#define APP_IIR_DISCARD_SAMPLE_COUNT    \
  ((APP_ADC_SAMPLE_RATE_HZ * APP_IIR_TRANSIENT_MS) / 1000UL)
#define APP_IIR_BIQUAD_COEFFICIENTS_Q30 \
  { { 259157, 518315, 259157, -2099786147, 1027080952 } }

#define APP_LORAWAN_PORT                (10U)
#define APP_LORAWAN_CONFIRMED           (false)
#define APP_TX_PAYLOAD_MAX_SIZE         (51U)
#define APP_TELEMETRY_BATCH_COUNT       (6U)
#define APP_TELEMETRY_HEADER_SIZE       (8U)
#define APP_TELEMETRY_MEAN_BITS         (12U)
#define APP_TELEMETRY_PAYLOAD_SIZE      \
  (APP_TELEMETRY_HEADER_SIZE + \
   ((APP_TELEMETRY_BATCH_COUNT * APP_ADC_CHANNEL_COUNT * \
     APP_TELEMETRY_MEAN_BITS + 7U) / 8U))

#if ((APP_ADC_DMA_BUFFER_SAMPLES % 2U) != 0U)
#error "APP_ADC_DMA_BUFFER_SAMPLES must be even"
#endif

#if ((APP_ADC_DMA_HALF_SAMPLES % APP_ADC_CHANNEL_COUNT) != 0U)
#error "Each DMA half-buffer must contain complete ADC scan sequences"
#endif

#if ((APP_ACQUISITION_TOTAL_SAMPLES % APP_ADC_CHANNEL_COUNT) != 0U)
#error "APP_ACQUISITION_TOTAL_SAMPLES must contain complete ADC scan sequences"
#endif

#if ((APP_ADC_CHANNEL_COUNT % 2U) != 0U)
#error "12-bit telemetry packing requires an even ADC channel count"
#endif

#if (APP_IIR_BIQUAD_STAGE_COUNT == 0U)
#error "APP_IIR_BIQUAD_STAGE_COUNT must be greater than zero"
#endif

#if ((APP_ACQUISITION_TOTAL_SAMPLES / APP_ADC_CHANNEL_COUNT) <= APP_IIR_DISCARD_SAMPLE_COUNT)
#error "Acquisition must leave IIR outputs after discarding the startup transient"
#endif

#if (APP_TELEMETRY_MEAN_BITS != 12U)
#error "Telemetry serializer currently requires 12-bit ADC means"
#endif

#if (APP_TX_PAYLOAD_MAX_SIZE < APP_TELEMETRY_PAYLOAD_SIZE)
#error "APP_TX_PAYLOAD_MAX_SIZE is too small for one telemetry record"
#endif

#endif /* SENSOR_NODE_APP_CONFIG_H */
