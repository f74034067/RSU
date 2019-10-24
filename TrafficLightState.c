#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "typedefine.h"

#define DLE_VAL 0xAA
#define STX_VAL 0xBB
#define ETX_VAL 0xCC
#define ACK_VAL 0xDD
#define NAK_VAL 0xEE
#define ADDR0_VAL 0xFF
#define ADDR1_VAL 0xFF
#define ACK_LEN0_VAL 0x00
#define ACK_LEN1_VAL 0x08
#define NAK_LEN0_VAL 0x00
#define NAK_LEN1_VAL 0x09


#define ERR_CKS    0x1
#define ERR_FRAME  0x2
#define ERR_ADDR   0x4
#define ERR_LENGTH 0x8

#define HEADER_LEN 10 // Packet_len - info_len
#define MAX_MSG_LEN 65535
#define MAX_PAYLOAD_LEN (MAX_MSG_LEN - HEADER_LEN)
#define ACK_INFO_LEN 0
#define NAK_INFO_LEN 1

void TLMessage_init(TLMessage* msg)
{
  msg->DLE_1 = DLE_VAL;
}

void TLMessage_clean(TLMessage* msg)
{
  msg->DLE_2 = 0;
  msg->ETX = 0;
}

void read_header(int fd, TLMessage* msg)
{
  TLMessage_clean(msg);
  read(fd, &msg->SEQ, 5);
}

void read_tail(int fd, TLMessage* msg)
{
  read(fd, &msg->DLE_2, 2);
}

int check_sum(TLMessage* msg, int info_len)
{
  uint8_t CKS = 0;
  uint8_t header_CKS = 0;
  uint8_t info_CKS = 0;
  uint8_t *tmp = NULL;

  tmp = &msg->DLE_1;
  for (int i = 0; i < 9; i++)
  {
    header_CKS ^= *tmp;
    tmp++;
  }

  tmp = msg->INFO;
  for (int i = 0; i < info_len; i++)
  {
    info_CKS ^= *tmp;
    tmp++;
  }
  CKS = header_CKS ^ info_CKS;
  return CKS;
}

int error_length(TLMessage* msg)
{
  if(msg->DLE_2 == DLE_VAL && msg->ETX == ETX_VAL)
    return 0;
  return 1;
}

int error_cks(TLMessage* msg, int CKS)
{
  if(msg->CKS == CKS)
    return 0;
  return 1;
}

int error_escape(TLMessage* msg, int info_len)
{
  uint8_t *tmp = msg->INFO;
  for (int i = 0; i < info_len; i++)
  {

    tmp++;
  }
}

int send_ack(int fd, TLMessage* msg)
{
  printf("into ack\n");
  uint8_t output_byte[ACK_LEN1_VAL];
  uint8_t CKS = 0;

  msg->TYPE = ACK_VAL;
  msg->ADDR[0] = ADDR0_VAL;
  msg->ADDR[1] = ADDR1_VAL;
  msg->LEN[0] = ACK_LEN0_VAL;
  msg->LEN[1] = ACK_LEN1_VAL;
  TLMessage_clean(msg);

  CKS = check_sum(msg, ACK_INFO_LEN);

  memcpy(output_byte, &msg->DLE_1, 7);
  output_byte[7] = CKS;
  for (int i = 0; i < 8; i++){
    printf("%x ", output_byte[i]);
  }
  //write(fd, output_byte, ACK_LEN1);
  printf("\n");
}

int send_nak(int fd, TLMessage* msg, uint8_t ERR)
{
  printf("into nak\n");
  uint8_t output_byte[NAK_LEN1_VAL];
  uint8_t CKS = 0;
  printf("ERR: %x\n", ERR);

  msg->TYPE = NAK_VAL;
  msg->ADDR[0] = ADDR0_VAL;
  msg->ADDR[1] = ADDR1_VAL;
  msg->LEN[0] = NAK_LEN0_VAL;
  msg->LEN[1] = NAK_LEN1_VAL;
  TLMessage_clean(msg);
  memcpy(msg->INFO, &ERR, 1);

  CKS = check_sum(msg, NAK_INFO_LEN);

  memcpy(output_byte, &msg->DLE_1, 7);
  output_byte[7] = ERR;
  output_byte[8] = CKS;
  for (int i = 0; i < 9; i++){
    printf("%x ", output_byte[i]);
  }
  //write(fd, output_byte, NAK_LEN1);
  printf("\n");
}

void recv_info(int fd, TLMessage* msg)
{
  printf("into recv_info\n");
  read_header(fd, msg);

  uint16_t packet_len = 0;
  uint16_t info_len = 0;
  uint8_t CKS = 0;

  packet_len = (uint16_t)msg->LEN[0] << 8 | msg->LEN[1];
  info_len = packet_len - HEADER_LEN;

  read(fd, msg->INFO, info_len);

  read_tail(fd, msg);
  if(error_length(msg) == 1){
    send_nak(fd, msg, ERR_LENGTH);
    return;
  }

  read(fd, &msg->CKS, 1);
  CKS = check_sum(msg, info_len);
  if(error_cks(msg, CKS) == 1){
    send_nak(fd, msg, ERR_CKS);
    return;
  }

  send_ack(fd, msg);
}

int recv_ack(int fd, TLMessage* msg)
{
  printf("into recv_ACK\n");
  read_header(fd, msg);

  uint8_t CKS = 0;
  read(fd, &msg->CKS, 1);

  CKS = check_sum(msg, ACK_INFO_LEN);
}

int recv_nak(int fd, TLMessage* msg)
{
  printf("into recv_NAK\n");
  read_header(fd, msg);
  read(fd, msg->INFO, 1);

  uint8_t CKS = 0;
  read(fd, &msg->CKS, 1);

  CKS = check_sum(msg, NAK_INFO_LEN);
}

void recv_traffic_light_state()
{
  int fd = open( "test1", O_RDWR );
  //int fd = open( "/dev/ttyS0", O_RDWR );
  if (fd == -1){
    perror(" file open failed ");
  }

  uint8_t input_byte[1];
  int len;
  bool escape_flag = 0;

  TLMessage *msg = (TLMessage*)malloc(HEADER_LEN + MAX_PAYLOAD_LEN);
  TLMessage_init(msg);

  while (len = read(fd, input_byte, 1) > 0)
  {
    //printf("input: %x ", input_byte[0]);
    /* 0xAA */
    if ( input_byte[0] == DLE_VAL && escape_flag == 0 ) {
      escape_flag = 1;
      continue;
    }

    if ( escape_flag == 1 ) {

      /* 0xAA 0xBB */
      if ( input_byte[0] == STX_VAL ) {
        msg->TYPE = STX_VAL;
        recv_info(fd, msg);
      }

      /* 0xAA 0xDD */
      else if ( input_byte[0] == ACK_VAL ) {
        msg->TYPE = ACK_VAL;
        recv_ack(fd, msg);
      }

      /* 0xAA 0xEE */
      else if ( input_byte[0] == NAK_VAL ) {
        msg->TYPE = NAK_VAL;
        recv_nak(fd, msg);
      }

      else {
        escape_flag = 0;
      }
    }
    printf("next\n");

    escape_flag = 0;
  }
}
