/******************************************************************************
       Module: linuxutils.cpp
     Engineer: Vimalraj Rajasekharan
  Description: Linux dependent utility functions

  Date           Initials    Description
  30-Nov-2023    VR          Initial
  ****************************************************************************/
#include "linuxutils.h"

/****************************************************************************
     Function: linuxevent_init
        Input: linuxevent *ev
                  Pointer to the linuxevent structure
       Output: N/A
  Description: Initialises the linuxevent
Date         Initials    Description
26-Nov-2015  MOH         Initial
****************************************************************************/
void linuxevent_init(linuxevent *ev) 
{
   pthread_mutex_init(&ev->mutex, 0);
   pthread_cond_init(&ev->cond, 0);
   ev->triggered = false;
}

/****************************************************************************
     Function: linuxevent_trigger
        Input: linuxevent *ev
                  Pointer to the linuxevent structure
       Output: N/A
  Description: Triggers the linuxevent
Date         Initials    Description
26-Nov-2015  MOH         Initial
****************************************************************************/
void linuxevent_trigger(linuxevent *ev)
{
   pthread_mutex_lock(&ev->mutex);
   ev->triggered = true;
   pthread_cond_signal(&ev->cond);
   pthread_mutex_unlock(&ev->mutex);
}

/****************************************************************************
     Function: linuxevent_wait
        Input: linuxevent *ev
                  Pointer to the linuxevent structure
       Output: N/A
  Description: Waits for the linuxevent
Date         Initials    Description
26-Nov-2015  MOH         Initial
****************************************************************************/
void linuxevent_wait(linuxevent *ev)
{
   pthread_mutex_lock(&ev->mutex);
   while (!ev->triggered)
      pthread_cond_wait(&ev->cond, &ev->mutex);
   ev->triggered = false;
   pthread_mutex_unlock(&ev->mutex);
}

/****************************************************************************
     Function: linuxevent_destroy
        Input: linuxevent *ev
                  Pointer to the linuxevent structure
       Output: N/A
  Description: Destroys the linuxevent
Date         Initials    Description
26-Nov-2015  MOH         Initial
****************************************************************************/
void linuxevent_destroy(linuxevent *ev) 
{
   pthread_cond_destroy(&ev->cond);
   pthread_mutex_destroy(&ev->mutex);
}

/****************************************************************************
     Function: closesocket
        Input: lSOCKET socketfd
                  Socket descriptor
       Output: Error value
  Description: Closes an open socket
Date         Initials    Description
30-Nov-2023  VR          Initial
****************************************************************************/
int closesocket(SOCKET socketfd)
{
    return close(socketfd);
}

int CreateThread(void *(*routine)(void *), void *arg)
{
    pthread_t tyThread;
    return pthread_create(&tyThread, NULL, routine, arg);
}
