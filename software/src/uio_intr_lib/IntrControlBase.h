
//==========================================================================================================
// IntrControlBase.h - Defines a class that manages an interrrupt controller
//
// This is intended to be used as a base class with the "isr()" routine over-ridden
//==========================================================================================================
#pragma once
#include <stdint.h>
#include <string>

class IntrControlBase
{

protected:

    // This gets called any time an interrupt occurs
    virtual void isr(uint32_t pending, int IRQ, uint32_t count) = 0;

public:

    // We need the userspace pointer to the PCI device and the AXI base address 
    // of the interrupt controller
    void        initialize(uint8_t* userspacePtr, uint32_t baseAddress);

    // Causes an interrupt on one or more IRQs
    void        generateInterrupt(uint32_t irqs);

    // Set and get the global-interrupt-disable bit
    bool        getGlobalEnable();
    void        setGlobalEnable(bool enable);

    // Get and set the per-IRQ interrupt mask
    uint32_t    getIrqMask();
    void        setIrqMask(uint32_t mask);

    // This is the top-level interrupt handler
    void        topLevelHandler();


private:

    volatile uint32_t* axiReg_;
};

