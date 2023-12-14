
//=================================================================================================
// UioInterface.h - Defines an interface to the Linux Userspace-I/O subsystem
//=================================================================================================
#pragma once
#include <string>
#include "IntrControlBase.h"


//-------------------------------------------------------------------
// This class manages the Linux Userspace I/O subsystem to receive
// interrupts from a PCI device
//-------------------------------------------------------------------
class UioInterface
{
public:

    // Initializes the Linux Userspace-I/O subsystem
    void    initialize(std::string device, IntrControlBase* pHandler);

    // This gets called if "monitorInterrupts" crashes.  Override this!
    virtual void crashHandler(int reason);

protected:
    
    // Spawns a thread that waits for incoming interrupt notifications
    void    spawnInterruptMonitor(int uioDevice);  

    // This runs in its own thread
    void    monitorInterrupts(int uioDevice);

    // This points to the class that will serve as an interrupt handler
    IntrControlBase* handler_;
};
//-------------------------------------------------------------------

