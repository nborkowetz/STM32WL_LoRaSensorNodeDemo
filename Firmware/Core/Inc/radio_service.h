#ifndef SENSOR_NODE_RADIO_SERVICE_H
#define SENSOR_NODE_RADIO_SERVICE_H

#include <stdbool.h>

void RadioService_Init(void);
void RadioService_Process(void);
bool RadioService_IsTxInProgress(void);

/* Forward the corresponding LBM events from lora_app.c. */
void RadioService_OnJoined(void);
void RadioService_OnJoinFailed(void);
void RadioService_OnTxDone(void);

#endif /* SENSOR_NODE_RADIO_SERVICE_H */
