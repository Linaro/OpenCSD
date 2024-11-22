/******************************************************************************
       Module: PICP.h
     Engineer: Vimalraj Rajasekharan
  Description: Header of class to define and manage Probe server
               packet formatting
  Date           Initials    Description
  01-Dec-2023    VR          Initial
******************************************************************************/
#pragma once
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#endif // WIN32

typedef enum PICPType
{
    PICP_TYPE_RESPONSE = 0x00,
    PICP_TYPE_PROBE = 0x01,
    PICP_TYPE_INTERNAL = 0x02,
    PICP_TYPE_PROGRESS = 0x03,

    PICP_TYPE_INVALID
} ePICPType;

typedef enum PICPCommand
{
    PICP_CMD_OPEN_DEVICE,
    PICP_CMD_CLOSE_DEVICE,

    PICP_CMD_BULK_READ,
    PICP_CMD_REGISTER_READ,
    PICP_CMD_BULK_WRITE,
    PICP_CMD_REGISTER_WRITE,

    PICP_CMD_PROCESS_COMMAND,
    PICP_CMD_TRACE_READ,

    PICP_CMD_DEVICE_LIST,
    PICP_CMD_FLASH_FPGA,
    PICP_CMD_LOAD_FPGA,
    PICP_CMD_ERASE_FPGA,

    PICP_CMD_TERMINATE,

    PICP_CMD_TRACE_START_STREAMING,
    PICP_CMD_TRACE_STOP_STREAMING,

    PICP_CMD_INVALID
} ePICPCommand;

struct PICPHeader
{
    uint8_t id;
    uint8_t type;
    uint32_t command;
    uint32_t datalength;
};

struct PICPFooter
{
    uint32_t crc;
};

// Probe Interface Communication Protocol (PICP)
class PICP
{
private:
    bool m_bIsValid;
    bool m_bIsCreated;
    uint8_t *m_pucBuffer;
    uint32_t m_ulCurIndex;

    PICPType m_eType;
    uint32_t m_eCommand;
    uint32_t m_ulMaxDataSize;
    uint32_t m_ulCRC;

    PICP();

public:
    PICP(uint32_t ulMaxDataSize, PICPType eType, uint32_t eCmd = PICP_CMD_INVALID);
    PICP(uint8_t *pBuffer, uint32_t ulSize);
    ~PICP();

    bool Validate();
    bool AttachData(const uint8_t *pBuffer, uint32_t ulSize);
    bool GetNextData(uint8_t* pBuffer, uint32_t ulSize);
    uint8_t *GetNextDataAddress(uint32_t *ulSize);
    uint8_t *GetPacketToSend(uint32_t *ulSize = NULL);

    bool SetType(PICPType eType);
    bool SetResponse(uint32_t ulResponse);
    bool SetDataSize(uint32_t ulSize);

    PICPType GetType() { return m_eType; };
    PICPCommand GetCommand() { return static_cast<PICPCommand>(m_eCommand); };
    uint32_t GetResponse() { return m_eCommand; };
    uint32_t GetDataSize() { return m_ulMaxDataSize; };
    uint32_t GetCurrentPacketSize() { return (m_ulCurIndex + sizeof(PICPFooter)); };
    static uint32_t GetMinimumSize() { return (sizeof(PICPHeader) + sizeof(PICPFooter)); };
};
