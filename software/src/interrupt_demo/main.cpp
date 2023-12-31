#include <unistd.h>
#include "UioInterface.h"
#include "PciDevice.h"

//================================================================================
// This is an example of a class that provides the interrupt-service routine
//================================================================================
class InterruptHandler : public IntrControlBase
{
protected:

    virtual void isr(uint32_t pending, int IRQ, uint32_t count)
    {
        printf("IRQ %u was detected %u times\n", IRQ, count);
    }
};
//================================================================================


//================================================================================
// Global objects, constants, and variables
//================================================================================
InterruptHandler handler;
UioInterface     UIO;
PciDevice        PCI;
std::string      device = "10ee:903f";
const uint32_t   INTR_CTRL_BASE_ADDR = 0x0000;
//================================================================================


//================================================================================
// Forward declarations
//================================================================================
void initializeInterrupts(uint8_t* userspacePtr, std::string device);
//================================================================================


//================================================================================
// main() - Performs program setup, initializes interrupts, then hangs
//================================================================================
int main()
{
    try
    {
        // Map the FPGA's registers into userspace
        PCI.open(device);

        // Get a userspace pointer to the first resource region for this PCI device
        uint8_t* userspacePtr = PCI.resourceList()[0].baseAddr;

        // Initialize the interrupt system
        initializeInterrupts(userspacePtr, device);

        // This thread is now free to go off and do other things
        printf("Waiting for interrupts\n");
        while(1) sleep(999999999);
    }
    catch(const std::exception& e)
    {
        printf("%s\n", e.what());
        exit(1);
    }
}
//================================================================================




//================================================================================
// initializeInterrupts() - Takes all the steps neccessary to enable, detect and
//                          report interrupts.
//================================================================================
void initializeInterrupts(uint8_t* userspacePtr, std::string device)
{
     // Tell the interrupt handler where to find its registers
    handler.initialize(userspacePtr, INTR_CTRL_BASE_ADDR);

    // Initialize the userspace I/O subsystem
    UIO.initialize(device, &handler);

    // Enable all the interrupt sources
    handler.setIrqMask(0xFFFFFFFF);

    // And globally enable interrupts
    handler.setGlobalEnable(true);
}
//================================================================================