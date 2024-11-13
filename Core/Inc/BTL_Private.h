/*****************************************************/
/*                 SWC: Bootloader                   */
/*            Author: Abdulrahman Omar               */
/*                 Version: v 1.0                    */
/*              Date: 27 Jan - 2024                  */
/*****************************************************/

#ifndef INC_BTL_PRIVATE_H_
#define INC_BTL_PRIVATE_H_

/* Bit positions for various fields in a record */
#define BTL_CC_0                  0
#define BTL_CC_1                  1

#define BTL_ADD_0                 2
#define BTL_ADD_1                 3
#define BTL_ADD_2                 4
#define BTL_ADD_3                 5

#define BTL_RT_0                  6
#define BTL_RT_1                  7

#define BTL_DATA_0                8
#define BTL_DATA_1                9
#define BTL_DATA_2                10
#define BTL_DATA_3                11

#define BTL_FULL_ADD0             0
#define BTL_FULL_ADD1             1
#define BTL_FULL_ADD2             2
#define BTL_FULL_ADD3             3
#define BTL_FULL_ADD4             4
#define BTL_FULL_ADD5             5
#define BTL_FULL_ADD6             6
#define BTL_FULL_ADD7             7

/* Bit positions for various fields in a dataBuffer */
#define BTL_CMD_TYPE              2

#define BTL_BUFFER_RECORDS0       4

#define BTL_DONE_FLAG             3

#define BTL_BUFFER_NEXT_SIZE0     5
#define BTL_BUFFER_NEXT_SIZE1     6

#define BTL_DATA_START            7

/* Some MCU and Bootloader related data */
#define BTL_BOOTLOADER_SIZE       0x8000 /* 32 Kilobyte */

#define BTL_MIN_ADDRESS 		  0x08000000
#define BTL_MAX_ADDRESS 		  0x0805FFFF

#define BTL_MIN_CC 		          0x00
#define BTL_MAX_CC 		          0x10

#define MAX_TIMEOUT               5

/* Version Information */
#define BTL_V_MAJOR '1'
#define BTL_V_MINOR '1'
#define BTL_V_PATCH '1'

/* Structure to hold information about the record being processed */
typedef struct
{
  uint8_t BTL_CC;                   /* Character count field of the record */
  uint32_t BTL_ADD;                 /* Address field of the record */
  uint8_t  BTL_RECORD_TYPE;         /* Record type field of the record */
  uint8_t BTL_DATA;                 /* Byte data field of the record */
  uint8_t BTL_CHECKSUM;             /* Checksum field of the record */
  uint16_t BTL_RECORD_INDEX;        /* Index indicating the position of the record in the data buffer */
  uint16_t BTL_BUFFER_POINTER;      /* Pointer to the start of current record */
  uint16_t BTL_ADDRESS_HIGH;        /* High bytes of the address field in the record */
  uint8_t BTL_NO_OF_BUFFER_RECORDS; /* Number of records in the received buffer for iteration */
} BTL_RecordTypeDef;


/* Enumeration for Bootloader Status */
typedef enum
{
  BTL_OK       = 0x00U,
  BTL_ERROR    = 0x01U,
} BTL_StatusTypeDef;

/* Enumeration for Record Types */
typedef enum
{
  BTL_DATA_RECORD_TYPE           = 0x00U, /* Data Record */
  BTL_EOF_RECORD_TYPE            = 0x01U, /* End-of-File Record */
  BTL_EXT_SEGMENT_ADDR_RECORD    = 0x02U, /* Extended Segment Address Record */
  BTL_EXT_LINEAR_ADDR_RECORD     = 0x04U, /* Extended Linear Address Record */
  BTL_START_LINEAR_ADDR_RECORD   = 0x05U, /* Start Linear Address Record (MDK-ARM only) */
} BTL_RecordTypeTypeDef;

/* Enumeration for Bootloader Commands */
typedef enum
{
	BTL_GET_VERSION              = 0x01U,
	BTL_GET_HELP                 = 0x02U,
	BTL_GET_ID                   = 0x03U,
	BTL_APP_FLASH                = 0x04U,
	BTL_FLASH_ERASE              = 0x05U,
	BTL_MEM_READ                 = 0x06U,
	BTL_OTP_READ                 = 0x07U,
	BTL_ERROR_CMD                = 0x08U,
} BTL_CMDTypeDef;

#endif /* INC_BTL_PRIVATE_H_ */
