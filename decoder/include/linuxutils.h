/******************************************************************************
       Module: linuxutils.h
     Engineer: SVimalraj Rajasekharan
  Description: Class for parsing command line arguemnts

  Date           Initials    Description
  30-Nov-2023    VR          Initial
  ****************************************************************************/
#pragma once
#include <stdint.h>
#include <stdio.h>
#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR    (uint32_t)(-1)
#endif

typedef int32_t SOCKET;

typedef struct 
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    uint8_t triggered;
} linuxevent;


void linuxevent_init(linuxevent *ev);
void linuxevent_trigger(linuxevent *ev);
void linuxevent_wait(linuxevent *ev);
void linuxevent_destroy(linuxevent *ev);

int closesocket(SOCKET socketfd);
int CreateThread(void *(*start_routine)(void *), void *arg);
