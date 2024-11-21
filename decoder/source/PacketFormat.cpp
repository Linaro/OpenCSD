/******************************************************************************
       Module: PICP.cpp
     Engineer: Vimalraj Rajasekharan
  Description: Class to define and manage Probe server packet formatting
  Date           Initials    Description
  01-Dec-2023    VR          Initial
******************************************************************************/
#include <stdio.h>
#include <string.h> 
#include "PacketFormat.h"
#ifdef __linux__
#include <arpa/inet.h>
#endif

/****************************************************************************
     Function: PICP
     Engineer: Vimalraj Rajasekharan
        Input: ulMaxDataSize - Maximum possible data size
               eType - Command type
               eCmd - Command value
       Output: None
       Return: None
  Description: Constructor
Date           Initials    Description
31-Oct-2023    VR          Initial
****************************************************************************/
PICP::PICP(uint32_t ulMaxDataSize, PICPType eType, uint32_t eCmd)
    : m_bIsValid(false)
    , m_bIsCreated(true)
    , m_pucBuffer(NULL)
    , m_ulCurIndex(0)
    , m_eType(eType)
    , m_eCommand(eCmd)
    , m_ulMaxDataSize(ulMaxDataSize)
    , m_ulCRC(0xDEADC0DE)
{
    m_pucBuffer = new uint8_t[sizeof(PICPHeader) + m_ulMaxDataSize + sizeof(PICPFooter)];

    if (m_pucBuffer)
    {
        // Fill the header
        PICPHeader *pHeader = reinterpret_cast<PICPHeader *>(m_pucBuffer);
        pHeader->id = 0;
        pHeader->type = static_cast<uint8_t>(eType);
        pHeader->command = htonl(eCmd);
        pHeader->datalength = 0;

        m_bIsValid = true;
        m_ulCurIndex = sizeof(PICPHeader);
    }
}

/****************************************************************************
     Function: PICP
     Engineer: Vimalraj Rajasekharan
        Input: pBuffer - Pointer to input buffer containing message
               ulSize - Maximum packet size
       Output: None
       Return: None
  Description: Constructor
Date           Initials    Description
31-Oct-2023    VR          Initial
****************************************************************************/
PICP::PICP(uint8_t *pBuffer, uint32_t ulSize)
    : m_bIsValid(false)
    , m_bIsCreated(false)
    , m_pucBuffer(NULL)
    , m_ulCurIndex(0)
    , m_eType(PICP_TYPE_INVALID)
    , m_eCommand(PICP_CMD_INVALID)
    , m_ulMaxDataSize(0)
    , m_ulCRC(0)
{
    m_pucBuffer = pBuffer;
    if (m_pucBuffer && ((sizeof(PICPHeader) + sizeof(PICPFooter)) <= ulSize))
    {
        PICPHeader *ptrHeader = (PICPHeader*)m_pucBuffer;
        m_eType = (PICPType)ptrHeader->type;
        m_eCommand = ntohl(ptrHeader->command);
        m_ulMaxDataSize = ntohl(ptrHeader->datalength);
        // Validate header, data length and CRC
        if ((m_ulMaxDataSize + sizeof(PICPHeader) + sizeof(PICPFooter)) <= ulSize)
        {
            m_ulCRC = ntohl(((PICPFooter *) (pBuffer + ulSize - sizeof(PICPFooter)))->crc);
            if (m_ulCRC == 0xDEADC0DE)
            {
                m_bIsValid = true;
                m_ulCurIndex = sizeof(PICPHeader);
            }
        }
    }
}

/****************************************************************************
     Function: ~PICP
     Engineer: Vimalraj Rajasekharan
        Input: None
       Output: None
       Return: None
  Description: Destructor
Date           Initials    Description
31-Oct-2023    VR          Initial
****************************************************************************/
PICP::~PICP()
{
    if (m_pucBuffer && m_bIsCreated)
    {
        delete[] m_pucBuffer;
    }
}

/****************************************************************************
     Function: Validate
     Engineer: Vimalraj Rajasekharan
        Input: None
       Output: None
       Return: True if success, false otherwise
  Description: Validate the packet formatting
Date           Initials    Description
31-Oct-2023    VR          Initial
****************************************************************************/
bool PICP::Validate()
{
    return m_bIsValid;
}

/****************************************************************************
     Function: AttachData
     Engineer: Vimalraj Rajasekharan
        Input: pBuffer - Probe Serial number
               ulSize - Probe type
       Output: None
       Return: True if success, false otherwise
  Description: Append input data to the message packet
Date           Initials    Description
31-Oct-2023    VR          Initial
****************************************************************************/
bool PICP::AttachData(const uint8_t *pBuffer, uint32_t ulSize)
{
    // Add data to the buffer and increment index and data size
    if (pBuffer == NULL)
        return false;

    if (ulSize > (m_ulMaxDataSize - (m_ulCurIndex - sizeof(PICPHeader))))
        return false;

    if (!m_bIsCreated)
        return false;

    memcpy(&m_pucBuffer[m_ulCurIndex], pBuffer, ulSize);
    m_ulCurIndex += ulSize;

    return true;
}

/****************************************************************************
     Function: SetType
     Engineer: Vimalraj Rajasekharan
        Input: eType - Message type
       Output: None
       Return: True if success, false otherwise
  Description: Set packet type
Date           Initials    Description
31-Oct-2023    VR          Initial
****************************************************************************/
bool PICP::SetType(PICPType eType)
{
    if (!m_bIsValid)
        return false;

    PICPHeader *pHeader = reinterpret_cast<PICPHeader *>(m_pucBuffer);
    pHeader->type = static_cast<uint8_t>(eType);

    return true;
}

/****************************************************************************
     Function: SetResponse
     Engineer: Vimalraj Rajasekharan
        Input: ulResponse - Response to be set
       Output: None
       Return: True if success, false otherwise
  Description: Set packet return value
Date           Initials    Description
31-Oct-2023    VR          Initial
****************************************************************************/
bool PICP::SetResponse(uint32_t ulResponse)
{
    if (!m_bIsValid)
        return false;

    PICPHeader *pHeader = reinterpret_cast<PICPHeader *>(m_pucBuffer);
    pHeader->command = htonl(ulResponse);

    return true;
}

/****************************************************************************
     Function: SetDataSize
     Engineer: Vimalraj Rajasekharan
        Input: ulSize - Data size to be set
       Output: None
       Return: True if success, false otherwise
  Description: Set total data size the packet contains
Date           Initials    Description
31-Oct-2023    VR          Initial
****************************************************************************/
bool PICP::SetDataSize(uint32_t ulSize)
{
    if ((m_ulCurIndex + ulSize) <= (sizeof(PICPHeader) + m_ulMaxDataSize))
    {
        m_ulCurIndex += ulSize;
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************************
     Function: GetNextData
     Engineer: Vimalraj Rajasekharan
        Input: ulSize - Size of data to be read
       Output: pBuffer - Pointer to buffer to store data
       Return: True if success, false otherwise
  Description: Get next data from current packet
Date           Initials    Description
31-Oct-2023    VR          Initial
****************************************************************************/
bool PICP::GetNextData(uint8_t *pBuffer, uint32_t ulSize)
{
    // Read from buffer and return pointer according to index
    if (pBuffer == NULL)
        return false;

    if (ulSize && (ulSize > (m_ulMaxDataSize - (m_ulCurIndex - sizeof(PICPHeader)))))
        return false;

    if (!m_bIsValid || m_bIsCreated)
        return false;

    memcpy(pBuffer, &m_pucBuffer[m_ulCurIndex], ulSize);
    m_ulCurIndex += ulSize;

    return true;
}

/****************************************************************************
     Function: GetNextData
     Engineer: Vimalraj Rajasekharan
        Input: None
       Output: ulSize - Pointer to store size of next data
       Return: Pointer to next data
  Description: Get address of next data from current packet
Date           Initials    Description
31-Oct-2023    VR          Initial
****************************************************************************/
uint8_t *PICP::GetNextDataAddress(uint32_t *ulSize)
{
    if (!m_bIsValid)
        return NULL;

    // Read from buffer and return pointer according to index
    if (ulSize == NULL)
    {
        // Return the start address of data
        return &m_pucBuffer[sizeof(PICPHeader)];
    }
    else
    {
        *ulSize = m_ulMaxDataSize - (m_ulCurIndex - sizeof(PICPHeader));
        return &m_pucBuffer[m_ulCurIndex];
    }
}

/****************************************************************************
     Function: GetNextData
     Engineer: Vimalraj Rajasekharan
        Input: None
       Output: ulSize - Pointer to store size of packet
       Return: Pointer to next data
  Description: Get address of current packet
Date           Initials    Description
31-Oct-2023    VR          Initial
****************************************************************************/
uint8_t *PICP::GetPacketToSend(uint32_t *ulSize)
{
    if (!m_bIsCreated)
        return m_pucBuffer;

    // Fill the header
    PICPHeader *pHeader = reinterpret_cast<PICPHeader *>(m_pucBuffer);
    pHeader->datalength = htonl(m_ulCurIndex - sizeof(PICPHeader));

    // Attach CRC (hardcoded now)
    *reinterpret_cast<uint32_t *>(&m_pucBuffer[m_ulCurIndex]) = htonl(m_ulCRC);

    if (ulSize != NULL)
    {
        *ulSize = m_ulCurIndex + sizeof(m_ulCRC);
    }

    return m_pucBuffer;
}
