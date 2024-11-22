/******************************************************************************
       Module: ProbeIntf.h
     Engineer: Vimalraj Rajasekharan
  Description: Header of interface class for Probe Server interface
  Date           Initials    Description
  01-Dec-2023    VR          Initial
******************************************************************************/
#pragma once
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#endif // WIN32

#define ERROR_PROBE_INTF_SUCCESS                    0x30
#define ERROR_PROBE_INTF_SERVER_UNREACHABLE         0x31

class ProbeIntf
{
public:
    virtual ~ProbeIntf() {}

public:
    virtual int32_t open() = 0;
    virtual int32_t close() = 0;

    virtual int32_t write(uint8_t *data, uint32_t size) = 0;
    virtual int32_t read(uint8_t *data, uint32_t* size) = 0;
    virtual uint32_t readtrace(uint8_t *data, uint32_t *size) = 0;
};
