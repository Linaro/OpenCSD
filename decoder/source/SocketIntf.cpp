/******************************************************************************
       Module: SocketIntf.cpp
     Engineer: Vimalraj Rajasekharan
  Description: Class for Probe server interface to be used by clients
  Date           Initials    Description
  01-Dec-2023    VR          Initial
******************************************************************************/
#include <stdio.h>
#include <errno.h>
#include "SocketIntf.h"
#include "PacketFormat.h"
#ifdef __LINUX
#include "linuxutils.h"
#endif

/****************************************************************************
     Function: SocketIntf
     Engineer: Vimalraj Rajasekharan
        Input: usPort - Port number of Probe Server
       Output: None
       Return: None
  Description: Constructor
Date           Initials    Description
31-Oct-2023    VR          Initial
****************************************************************************/
SocketIntf::SocketIntf(uint16_t usPort)
    : m_socket(INVALID_SOCKET)
    , m_usPort(usPort)
{
#ifndef __LINUX
    static WSADATA wsaData;
    int err;

    // Initialize Winsock
    if ((err = WSAStartup(MAKEWORD(1, 1), &wsaData)) != 0)
    {
        printf("WSAStartup failed with error: %d\n", err);
        return;
    }
#endif
}

/****************************************************************************
     Function: ~SocketIntf
     Engineer: Vimalraj Rajasekharan
        Input: None
       Output: None
       Return: None
  Description: Destructor
Date           Initials    Description
31-Oct-2023    VR          Initial
****************************************************************************/
SocketIntf::~SocketIntf()
{
    if (m_socket != INVALID_SOCKET)
    {
        closesocket(m_socket);
    }
#ifndef __LINUX
    WSACleanup();
#endif
}

/****************************************************************************
     Function: open
     Engineer: Vimalraj Rajasekharan
        Input: None
       Output: None
       Return: Error value
  Description: Open Socket Client Interface
Date           Initials    Description
31-Oct-2023    VR          Initial
****************************************************************************/
int32_t SocketIntf::open()
{
    sockaddr_in serv_addr;
    int err = 0;

    if (m_socket != INVALID_SOCKET)
    {
        // Already opened
        return err;
    }

    if ((m_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(m_usPort);

    if ((err = connect(m_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0)
    {
        printf("\nConnection Failed \n");
#ifndef __LINUX
        if (h_errno == WSAHOST_NOT_FOUND)
        {
            err = ERROR_PROBE_INTF_SERVER_UNREACHABLE;
        }
#else
        if ((errno == EHOSTUNREACH) || (errno == EHOSTDOWN) || (errno == ETIMEDOUT))
        {
            err = ERROR_PROBE_INTF_SERVER_UNREACHABLE;
        }
#endif
    }

    return err;
}

/****************************************************************************
     Function: close
     Engineer: Vimalraj Rajasekharan
        Input: None
       Output: None
       Return: Error value
  Description: Close Socket Client Interface
Date           Initials    Description
31-Oct-2023    VR          Initial
****************************************************************************/
int32_t SocketIntf::close()
{
    int32_t ret = -1;

    if (m_socket != INVALID_SOCKET)
    {
        ret = closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }

    return ret;
}

/****************************************************************************
     Function: write
     Engineer: Vimalraj Rajasekharan
        Input: data - Pointer to buffer containing data
               size - Size of data to be written
       Output: None
       Return: Total bytes written
  Description: Write data to Socket Client Interface
Date           Initials    Description
31-Oct-2023    VR          Initial
****************************************************************************/
int32_t SocketIntf::write(uint8_t *data, uint32_t size)
{
    if (m_socket == INVALID_SOCKET)
        return -1;

    if (data == NULL)
        return -1;

    // Send to Server
    // TODO: Check error and completion of transfer of full data
    int writtenBytes = 0;
    uint32_t totalBytes = 0;

    while (totalBytes < size)
    {
        writtenBytes = send(m_socket, (const char *) (data + totalBytes), (size - totalBytes), 0);
        if (writtenBytes < 0)
        {
            totalBytes = writtenBytes;
            break;
        }
        else
        {
            totalBytes += writtenBytes;
        }
    }

    return totalBytes;
}

/****************************************************************************
     Function: read
     Engineer: Vimalraj Rajasekharan
        Input: None
       Output: data - Pointer to buffer to store data
               size - Size of data read
       Return: Total bytes read
  Description: Read data from Socket Client Interface
Date           Initials    Description
31-Oct-2023    VR          Initial
****************************************************************************/
int32_t SocketIntf::read(uint8_t *data, uint32_t *size)
{
    if (m_socket == INVALID_SOCKET)
        return -1;

    if (data == NULL)
        return -1;

    // Recieve from client
    // TODO: Check error and completion of transfer of full data
    int readBytes = 0;
    int totalBytes = 0;
    int remainingBytes = sizeof(PICPHeader);
    if (remainingBytes > (int) *size)
    {
        return -1;
    }

    readBytes = recv(m_socket, (char *) data, remainingBytes, 0);
    if (readBytes < 0)
    {
        // This is not expected, return error
        totalBytes = -1;
    }
    else if (readBytes == sizeof(PICPHeader))
    {
        remainingBytes = sizeof(PICPHeader) + ntohl(((PICPHeader *) data)->datalength) + sizeof(PICPFooter);
        if (remainingBytes > (int)*size)
        {
            return -1;
        }
        remainingBytes -= readBytes;

        totalBytes = readBytes;
        while (remainingBytes > 0)
        {
            readBytes = recv(m_socket, (char *) (data + totalBytes), remainingBytes, 0);
            if (readBytes <= 0)
            {
                // Read error
                totalBytes = -1;
                break;
            }
            remainingBytes -= readBytes;
            totalBytes += readBytes;
        }
        *size = totalBytes;
    }
    else
    {
        // This is not expected, return error
        totalBytes = -1;
    }

    return totalBytes;
}

/****************************************************************************
     Function: readtrace
     Engineer: Vimalraj Rajasekharan
        Input: None
       Output: data - Pointer to buffer to store data
               size - Size of data read
       Return: Total bytes read
  Description: Read trace data from Socket Client Interface
Date           Initials    Description
31-Oct-2023    VR          Initial
****************************************************************************/
uint32_t SocketIntf::readtrace(uint8_t *data, uint32_t *size)
{
    if (m_socket == INVALID_SOCKET)
        return (uint32_t)(-1);

    if (data == NULL)
        return (uint32_t)(-1);

    // Recieve from client
    // TODO: Check error and completion of transfer of full data
    bool bIsResponse = true;
    int readBytes = 0;
    int totalBytes = 0;
    int remainingBytes = *size;

    while (remainingBytes > 0)
    {
        readBytes = recv(m_socket, (char *) (data + totalBytes), remainingBytes, 0);
        if (bIsResponse)
        {
            PICP retPacket(data, sizeof(PICPHeader) + sizeof(PICPFooter));
            if (retPacket.Validate())
            {
                if (retPacket.GetResponse() != 0)
                {
                    // Read error
                    totalBytes = -1;
                    break;
                }
            }
            bIsResponse = false;
        }
        if (readBytes <= 0)
        {
            // Read error
            totalBytes = -1;
            break;
        }
        remainingBytes -= readBytes;
        totalBytes += readBytes;
    }
    *size = totalBytes;

    return totalBytes;
}
