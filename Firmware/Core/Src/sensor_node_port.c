#include "sensor_node_port.h"

#include <stdbool.h>
#include <stdint.h>

#include "main.h"
#include "sensor_node.h"

extern ADC_HandleTypeDef hadc;
extern TIM_HandleTypeDef htim2;

static void SensorPowerSet(bool enabled)
{
  HAL_GPIO_WritePin(EN_12V_DCDC_GPIO_Port, EN_12V_DCDC_Pin,
                    enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static bool AdcStartDma(uint16_t *buffer, uint32_t sample_count)
{
  if (HAL_ADC_Start_DMA(&hadc, (uint32_t *) buffer, sample_count) != HAL_OK)
  {
    return false;
  }

  if (HAL_TIM_Base_Start(&htim2) != HAL_OK)
  {
    (void) HAL_ADC_Stop_DMA(&hadc);
    return false;
  }

  return true;
}

static void AdcStopDma(void)
{
  (void) HAL_TIM_Base_Stop(&htim2);
  (void) HAL_ADC_Stop_DMA(&hadc);
}

static uint32_t GetTimeMs(void)
{
  return HAL_GetTick();
}

void SensorNodePort_Init(void)
{
  static const SensorNodePlatform_t platform =
  {
    .SensorPowerSet = SensorPowerSet,
    .AdcStartDma = AdcStartDma,
    .AdcStopDma = AdcStopDma,
    .GetTimeMs = GetTimeMs
  };

  SensorNode_Init(&platform);
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *adc)
{
  if (adc == &hadc)
  {
    SensorNode_OnAdcHalfComplete();
  }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *adc)
{
  if (adc == &hadc)
  {
    SensorNode_OnAdcComplete();
  }
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *adc)
{
  if (adc == &hadc)
  {
    SensorNode_OnAdcError();
  }
}
