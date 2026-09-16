#include "sensor_node.h"

#include <stddef.h>
#include <string.h>

#include "app_config.h"
#include "signal_processing.h"
#include "stm32_timer.h"
#include "sys_app.h"
#include "telemetry.h"
#include "utilities_def.h"

#define ADC_EVENT_HALF   (1UL << 0)
#define ADC_EVENT_FULL   (1UL << 1)
#define ADC_EVENT_ERROR  (1UL << 2)

static SensorNodePlatform_t platform_api;
static SensorNodeState_t state;
static UTIL_TIMER_Object_t acquisition_timer;
static UTIL_TIMER_Object_t warmup_timer;
static uint16_t adc_dma_buffer[APP_ADC_DMA_BUFFER_SAMPLES];
static volatile uint32_t adc_events;
static volatile bool acquisition_due;
static volatile bool warmup_elapsed;
static uint32_t processed_samples;

UTIL_TIMER_Status_t create_status;
UTIL_TIMER_Status_t start_status;

static void AcquisitionTimerCallback(void *context)
{
  (void) context;
  acquisition_due = true;
}

static void WarmupTimerCallback(void *context)
{
  (void) context;
  warmup_elapsed = true;
}

static uint32_t TakeAdcEvents(void)
{
  uint32_t events;

  UTILS_ENTER_CRITICAL_SECTION();
  events = adc_events;
  adc_events = 0U;
  UTILS_EXIT_CRITICAL_SECTION();

  return events;
}

static void FinishAcquisition(void)
{
  if (platform_api.AdcStopDma != NULL)
  {
    platform_api.AdcStopDma();
  }
  state = SENSOR_NODE_PROCESSING;
  APP_LOG(TS_ON, VLEVEL_M, "Stop  acquisition\r\n");
}

static void ProcessAdcBlock(const uint16_t *samples, uint32_t available_samples)
{
  uint32_t remaining_samples = APP_ACQUISITION_TOTAL_SAMPLES - processed_samples;
  uint32_t samples_to_process = (available_samples < remaining_samples) ?
                                available_samples : remaining_samples;

  if (samples_to_process != 0U)
  {
    SignalProcessing_ProcessBlock(samples, samples_to_process);
    processed_samples += samples_to_process;
  }
}

void SensorNode_Init(const SensorNodePlatform_t *platform)
{
  APP_LOG(TS_ON, VLEVEL_M, "Sensor initialization\r\n");

  memset(&platform_api, 0, sizeof(platform_api));
  if (platform != NULL)
  {
    platform_api = *platform;
  }

  SignalProcessing_Init();
  state = SENSOR_NODE_IDLE;
  acquisition_due = false;
  warmup_elapsed = false;
  adc_events = 0U;

  create_status = UTIL_TIMER_Create(&acquisition_timer, APP_ACQUISITION_PERIOD_MS,
                    UTIL_TIMER_PERIODIC, AcquisitionTimerCallback, NULL);
  UTIL_TIMER_Create(&warmup_timer, APP_SENSOR_WARMUP_MS,
                    UTIL_TIMER_ONESHOT, WarmupTimerCallback, NULL);
  start_status = UTIL_TIMER_Start(&acquisition_timer);

  APP_LOG(TS_ON, VLEVEL_M, "Acq timer start=%d start=%d running=%u\r\n",
          create_status,
          start_status, 
          (unsigned int)UTIL_TIMER_IsRunning(&acquisition_timer));
}

void SensorNode_Process(void)
{
  switch (state)
  {
    case SENSOR_NODE_IDLE:
      if (acquisition_due)
      {
        acquisition_due = false;
        if (platform_api.SensorPowerSet != NULL)
        {
          platform_api.SensorPowerSet(true);
        }
        UTIL_TIMER_SetPeriod(&warmup_timer, APP_SENSOR_WARMUP_MS);
        UTIL_TIMER_Start(&warmup_timer);
        state = SENSOR_NODE_WARMUP;
        APP_LOG(TS_ON, VLEVEL_M, "Start sensor warmup\r\n");
      }
      break;

    case SENSOR_NODE_WARMUP:
      if (warmup_elapsed)
      {
        warmup_elapsed = false;
        processed_samples = 0U;
        adc_events = 0U;
        SignalProcessing_Reset();

        if ((platform_api.AdcStartDma == NULL) ||
            !platform_api.AdcStartDma(adc_dma_buffer, APP_ADC_DMA_BUFFER_SAMPLES))
        {
          state = SENSOR_NODE_ERROR;
        }
        else
        {
          state = SENSOR_NODE_ACQUISITION;
          APP_LOG(TS_ON, VLEVEL_M, "Starting acquisition\r\n");
        }
      }
      break;

    case SENSOR_NODE_ACQUISITION:
    {
      uint32_t events = TakeAdcEvents();

      if ((events & ADC_EVENT_ERROR) != 0U)
      {
        state = SENSOR_NODE_ERROR;
        break;
      }
      if ((events & ADC_EVENT_HALF) != 0U)
      {
        ProcessAdcBlock(&adc_dma_buffer[0], APP_ADC_DMA_HALF_SAMPLES);
        APP_LOG(TS_ON, VLEVEL_M, "Half data processed\r\n");
      }
      if (((events & ADC_EVENT_FULL) != 0U) &&
          (processed_samples < APP_ACQUISITION_TOTAL_SAMPLES))
      {
        ProcessAdcBlock(&adc_dma_buffer[APP_ADC_DMA_HALF_SAMPLES], APP_ADC_DMA_HALF_SAMPLES);
        APP_LOG(TS_ON, VLEVEL_M, "Full data processed\r\n");
      }
      if (processed_samples >= APP_ACQUISITION_TOTAL_SAMPLES)
      {
        FinishAcquisition();
      }
      break;
    }

    case SENSOR_NODE_PROCESSING:
    {
      SignalStatistics_t statistics;
      uint32_t timestamp = (platform_api.GetTimeMs != NULL) ? platform_api.GetTimeMs() : 0U;

      SignalProcessing_Finalize(&statistics);
      if (!Telemetry_Update(timestamp, &statistics))
      {
        APP_LOG(TS_ON, VLEVEL_M, "Telemetry batch still pending; acquisition not stored\r\n");
      }

      if (platform_api.SensorPowerSet != NULL)
      {
        platform_api.SensorPowerSet(false);
      }
      APP_LOG(TS_ON, VLEVEL_M, "Acquisition complete: %lu samples\r\n",
              statistics.input_sample_count[0]);

      APP_LOG(TS_ON, VLEVEL_M, "misured voltage reference: %lu\r\n", statistics.mean[3]);
      state = SENSOR_NODE_IDLE;
      break;
    }

    case SENSOR_NODE_ERROR:
    default:
      if (platform_api.AdcStopDma != NULL)
      {
        platform_api.AdcStopDma();
      }
      if (platform_api.SensorPowerSet != NULL)
      {
        platform_api.SensorPowerSet(false);
      }
      APP_LOG(TS_ON, VLEVEL_M, "Sensor node acquisition error\r\n");
      state = SENSOR_NODE_IDLE;
      break;
  }
}

SensorNodeState_t SensorNode_GetState(void)
{
  return state;
}

void SensorNode_OnAdcHalfComplete(void)
{
  adc_events |= ADC_EVENT_HALF;
}

void SensorNode_OnAdcComplete(void)
{
  adc_events |= ADC_EVENT_FULL;
}

void SensorNode_OnAdcError(void)
{
  adc_events |= ADC_EVENT_ERROR;
}
