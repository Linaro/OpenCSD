/******************************************************************************
       Module: SocketIntf.h
     Engineer: Vimalraj Rajasekharan
  Description: Header of class for Probe server interface to be used by clients
  Date           Initials    Description
  01-Dec-2023    VR          Initial
******************************************************************************/
#pragma once
#include <stdint.h>
#include "ProbeIntf.h"

#ifdef _WIN32
#include <windows.h>
#include <winsock.h>
#endif // WIN32

#define BUFF_SIZE 2*1024

#ifdef __LINUX
typedef int32_t SOCKET;
#define INVALID_SOCKET  (SOCKET)(~0)
#endif

class SocketIntf : public ProbeIntf
{
    SOCKET m_socket;
    uint16_t m_usPort;

public:
    SocketIntf(uint16_t usPort);
    ~SocketIntf();

    virtual int32_t open();
    virtual int32_t close();

    virtual int32_t write(uint8_t *data, uint32_t size);
    virtual int32_t read(uint8_t *data, uint32_t *size);
    virtual uint32_t readtrace(uint8_t *data, uint32_t *size);
};
