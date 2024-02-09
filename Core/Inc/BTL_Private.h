/*
 * BTL_Interface.h
 *
 *  Created on: Jan 27, 2024
 *      Author: Xavi Omar
 */

#ifndef INC_BTL_PRIVATE_H_
#define INC_BTL_PRIVATE_H_

#define BTL_V_MAJOR '1'
#define BTL_V_MINOR '1'
#define BTL_V_PATCH '1'

#define BTL_DATA_RECORD_TYPE    0
#define BTL_EXTEND_RECORD_TYPE  1

#define BTL_CC_0                0
#define BTL_CC_1                1

#define BTL_ADD_0               2
#define BTL_ADD_1               3
#define BTL_ADD_2               4
#define BTL_ADD_3               5

#define BTL_RT_0                6
#define BTL_RT_1                7

#define BTL_DATA_0              8
#define BTL_DATA_1              9
#define BTL_DATA_2              10
#define BTL_DATA_3              11


#define BTL_CMD_TYPE            2

#define BTL_BUFFER_RECORDS0     4

#define BTL_DONE_FLAG           3

#define BTL_BUFFER_NEXT_SIZE0   5
#define BTL_BUFFER_NEXT_SIZE1   6

#define BTL_DATA_START          7

#define MAX_TIMEOUT             150

typedef struct
{
  uint8_t BTL_CC;
  uint32_t BTL_ADD;
  uint8_t  BTL_RECORD_TYPE;
  uint8_t BTL_DATA;
  uint8_t BTL_CHECKSUM;
  uint16_t BTL_RECORD_INDEX;
  uint16_t BTL_BUFFER_POINTER;
  uint16_t BTL_ADDRESS_HIGH;
  uint8_t BTL_NO_OF_BUFFER_RECORDS;
} BTL_RecordTypeDef;

typedef enum
{
  BTL_OK       = 0x00U,
  BTL_ERROR    = 0x01U,
} BTL_StatusTypeDef;

typedef enum
{
	BTL_GET_VER_CMD              = 0x10U,
	BTL_GET_HELP_CMD             = 0x11U,
	BTL_GET_CID_CMD              = 0x12U,
	BTL_GET_RDP_STATUS_CMD       = 0x13U,
	BTL_GO_TO_ADDR_CMD           = 0x14U,
	BTL_FLASH_ERASE_CMD          = 0x15U,
	BTL_MEM_WRITE_CMD            = 0x16U,
	BTL_ED_W_PROTECT_CMD         = 0x17U,
	BTL_MEM_READ_CMD             = 0x18U,
	BTL_READ_SECTOR_STATUS_CMD   = 0x19U,
	BTL_OTP_READ_CMD             = 0x20U,
	BTL_CHANGE_ROP_Level_CMD     = 0x21U,
	BTL_ERROR_CMD                = 0x22U,
} BTL_CMDTypeDef;


#endif /* INC_BTL_PRIVATE_H_ */
