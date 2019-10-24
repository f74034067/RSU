#include <stdint.h>


typedef struct TrafficLightMessage
{
  uint8_t CKS;
  uint8_t DLE_1;  /*Start of Packet*/
  uint8_t TYPE;   /*Type of Packet*/
  uint8_t SEQ;
  uint8_t ADDR[2];
  uint8_t LEN[2];
  uint8_t DLE_2;
  uint8_t ETX;
  uint8_t INFO[];
} TLMessage;

typedef struct TrafficLightState
{
  uint8_t SignalMap;
  uint8_t SignalStatus[2];
  uint8_t GreenSignalMap;
  uint8_t YellowSignalMap;
  uint8_t RedSignalMap;
  uint8_t SubPhaseID;
  uint8_t StepID;
  uint8_t StepSec[2];
  uint8_t StepEastWest;
  uint8_t StepSouthNorth;
} TLState;
