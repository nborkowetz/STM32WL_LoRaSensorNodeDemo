#include "radio_service.h"

#include <stdint.h>

#include "app_config.h"
#include "sensor_node.h"
#include "smtc_modem_api.h"
#include "stm32_timer.h"
#include "sys_app.h"
#include "telemetry.h"

#ifndef STACK_ID
#define STACK_ID (0U)
#endif

static UTIL_TIMER_Object_t tx_timer;
static volatile bool tx_due;
static bool joined;
static bool tx_in_progress;
static uint8_t tx_payload[APP_TX_PAYLOAD_MAX_SIZE];

static void TxTimerCallback(void *context)
{
  (void) context;
  tx_due = true;
}

void RadioService_Init(void)
{
  tx_due = false;
  joined = false;
  tx_in_progress = false;

  UTIL_TIMER_Create(&tx_timer, APP_TX_PERIOD_MS,
                    UTIL_TIMER_PERIODIC, TxTimerCallback, NULL);
  UTIL_TIMER_Start(&tx_timer);
}

void RadioService_Process(void)
{
  size_t payload_size;
  smtc_modem_return_code_t result;

  if (!tx_due || !joined || tx_in_progress || !Telemetry_IsReady() ||
      (SensorNode_GetState() != SENSOR_NODE_IDLE))
  {
    return;
  }

  payload_size = Telemetry_Serialize(tx_payload, sizeof(tx_payload));
  if (payload_size == 0U)
  {
    return;
  }

  result = smtc_modem_request_uplink(STACK_ID, APP_LORAWAN_PORT,
                                     APP_LORAWAN_CONFIRMED,
                                     tx_payload, (uint8_t) payload_size);
  if (result == SMTC_MODEM_RC_OK)
  {
    tx_due = false;
    tx_in_progress = true;
    APP_LOG(TS_ON, VLEVEL_M, "Uplink requested: %u bytes\r\n", (unsigned int) payload_size);
  }
  else
  {
    APP_LOG(TS_ON, VLEVEL_M, "Uplink deferred, modem RC=%d\r\n", result);
  }
}

bool RadioService_IsTxInProgress(void)
{
  return tx_in_progress;
}

void RadioService_OnJoined(void)
{
  joined = true;
}

void RadioService_OnJoinFailed(void)
{
  joined = false;
}

void RadioService_OnTxDone(void)
{
  tx_in_progress = false;
}
