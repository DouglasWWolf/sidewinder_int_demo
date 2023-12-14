#include "IntrControlBase.h"

// These are the control and status registers of the interrupt controller
enum
{
    REG_IRQ_PENDING        =  0,
    REG_IRQ_ACK            =  1,
    REG_IRQ_MASK           =  2,
    REG_GLOB_ENABLE        =  3,
    REG_COUNTERS           = 32
};


uint32_t IntrControlBase::getIrqMask()
{
    return axiReg_[REG_IRQ_MASK];
}

void IntrControlBase::setIrqMask(uint32_t mask)
{
    axiReg_[REG_IRQ_MASK] = mask;
}


bool IntrControlBase::getGlobalEnable()
{
    return axiReg_[REG_GLOB_ENABLE] & 1;
}


void IntrControlBase::setGlobalEnable(bool flag)
{
    axiReg_[REG_GLOB_ENABLE] = flag;
}



void IntrControlBase::generateInterrupt(uint32_t irqs)
{
    axiReg_[REG_IRQ_PENDING] = irqs;
}

//=============================================================================
// initialize() - Determines the userspace address of the first AXI register
//                of our interrupt controller
//=============================================================================
void IntrControlBase::initialize(uint8_t* userspacePtr, uint32_t baseAddress)
{
    axiReg_ = (uint32_t*)(userspacePtr + baseAddress);
}
//=============================================================================

//=============================================================================
// topLevelHandler() - The userspace I/O interrupt monitor calls this any 
//                     time it detects that the interrupt controller raised
//                     the IRQ_REQ line.
//=============================================================================
void IntrControlBase::topLevelHandler()
{
    int      i;
    uint32_t counter[32];

    // Find out which interrupts are pending
    uint32_t pending = axiReg_[REG_IRQ_PENDING];

    // If there are no interrupts pending then this was spurious, we're done
    if (pending == 0) return;

    // Read the counter for every pending IRQ so we can allow IRQ_REQ to
    // de-assert as quickly as possible.  Reading a counter clears
    // the "pending" status in the interrupt controller
    for (i=0; i<32; ++i) if (pending & (1<<i))
    {
        counter[i] = axiReg_[REG_COUNTERS + i];
    }

    // Now call the interrupt service routines
    for (i=0; i<32; ++i) if (pending & (1<<i))
    {
        isr(pending, i, counter[i]);
    }

}
//=============================================================================

