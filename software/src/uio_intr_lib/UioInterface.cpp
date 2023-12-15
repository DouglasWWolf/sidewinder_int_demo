#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <filesystem>
#include <string>
#include <thread>
#include <stdexcept>
#include "UioInterface.h"

static volatile int bitBucket;
namespace fs=std::filesystem;


//=================================================================================================
// throwRuntime() - Throws a runtime exception
//=================================================================================================
static void throwRuntime(const char* fmt, ...)
{
    char buffer[1024];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);

    throw std::runtime_error(buffer);
}
//=================================================================================================



//=================================================================================================
// getBDF() - Converts a <vendorID:deviceID> into a PCI BDF
//=================================================================================================
static std::string getBDF(std::string device)
{
    char command[64];
    char buffer[256];
    std::string retval = "0000:";

    // Build the command "lspci -d {device}"
    sprintf(command, "lspci -d %s", device.c_str());

    // Open the lpsci command as a process
    FILE* fp = popen(command, "r");

    // If we couldn't do that, give up
    if (fp == nullptr) return "";

    // Fetch the first line of the output
    if (!fgets(buffer, sizeof buffer, fp))
    {
        fclose(fp);
        return "";
    }

    // Find the first space in the line
    char* p = strchr(buffer, ' ');
    if (p == nullptr)
    {
        fclose(fp);
        return "";
    }

    // Terminate the string at the first space. 
    *p = 0;
    
    // The PCI BDF of the device is the first token in the buffer
    return retval + buffer;
}
//=================================================================================================


//=================================================================================================
// registerUioDevice() - Registers our device with the Linux UIO subsystem
//=================================================================================================
static void registerUioDevice(std::string device)
{
    char buffer[100];

    const char* filename = "/sys/bus/pci/drivers/uio_pci_generic/new_id";

    // Get a copy of our device name in vendorID:deviceID format
    strcpy(buffer, device.c_str());

    // Replace the ':' delimeter with a space
    char* delimeter = strchr(buffer, ':');
    if (delimeter == nullptr) return;
    *delimeter = ' ';

    // Append a linefeed to the end
    strcat(buffer, "\n");

    // Open the psuedo-file that allows us to register a new device
    int fd = open(filename, O_WRONLY);

    // If we can't open it, something is awry
    if (fd == -1) throwRuntime("Cant open %s", filename);

    // It doesn't matter if this call fails because the device is already registered
    bitBucket = write(fd, buffer, strlen(buffer));

    // We're done with the file descriptor
    close(fd);
}
//=================================================================================================


//=================================================================================================
// extractIndexFromUioName() - A UIO name is something like "/sys/class/uio/uio3".
//                             This routine returns the numeric value at the end of the string
//=================================================================================================
static int extractIndexFromUioName(std::string name)
{
    // Find the nul at the end of the string
    const char* p = strchr(name.c_str(), 0);

    // Walk backwards until we encounter a character that isn't a digit
    while (*(p-1) >= '0' && *(p-1) <= '9') --p;

    // And hand the caller the integer value at the end of the string
    return atoi(p);
}
//=================================================================================================


//=================================================================================================
// findUioIndex() - Returns the UIO index of our device
//=================================================================================================
static int findUioIndex(std::string bdf)
{
    std::string directory = "/sys/class/uio";

    // We're looking for a symlink target that contains "/<bdf>/uio/uio"
    std::string searchKey = "/" + bdf + "/uio/uio";

    // Loop through the entry for each device in the specified directory...
    for (auto const& entry : fs::directory_iterator(directory)) 
    {
        // Ignore any entry that isn't a symbolic link
        if (!entry.is_symlink()) continue;

        // Fetch the path of the source of the symlink
        fs::path source = entry.path();

        // Find the target of the symlink
        fs::path target = fs::read_symlink(source);

        // Get the filename (or folder name) of the target
        std::string targetFilename = target.string();

        // If we found the key we're looking for, return the associated UIO index
        if (targetFilename.find(searchKey) != std::string::npos)
        {
            return extractIndexFromUioName(source.string());
        }
    }

    // If we get here, we couldn't find the BDF we were looking for
    return -1;
}
//=================================================================================================



//=================================================================================================
// initialize() - Registers our device with the Linux UIO subsystem and returns the 
//                UIO index that corresponds to our device
//
// Passed: device = PCI device name in vendorID:deviceID format
//=================================================================================================
void UioInterface::initialize(std::string device, IntrControlBase* handler)
{
    // Store the pointer to the interrupt handler
    handler_ = handler;

    // Convert the device ID into a BDF
    std::string bdf = getBDF(device);

    // If this device isn't installed, drop dead
    if (bdf.empty()) throwRuntime("PCI device %s not found", device.c_str());

    // Make sure the generic UIO PCI device driver is loaded
    bitBucket = system("modprobe uio_pci_generic");

    // Register our device with the UIO subsystem
    registerUioDevice(device);

    // Fetch the UIO index that corresponds to our device
    int uioIndex = findUioIndex(bdf);

    // If we couldn't find a valid index, complain and give up
    if (uioIndex < 0) throwRuntime("Can't initialize UIO subsystem for device %s", device.c_str());

    // Spawn "monitorInterrupts()" in its own thread
    std::thread th(&UioInterface::monitorInterrupts, this, uioIndex);

    // Let it keep running, even when "thread" goes out of scope
    th.detach();
}
//=================================================================================================


//==========================================================================================================
// This is a list of reasons that monitorInterrupts() could crash
//==========================================================================================================
enum 
{
    CRASH_OPEN_PDEVICE = 1,
    CRASH_OPEN_CONFIG  = 2,
    CRASH_PREAD_1      = 3,
    CRASH_PREAD_2      = 4,
    CRASH_READ_LEN     = 5
};

class crash
{
public:
    crash(int reason) {code = reason;}
    int code;
};
//==========================================================================================================


//=================================================================================================
// monitorInterrupts() - Sits in a loop reading interrupt notifications and distributing 
//                       notifications to the FIFOs that track each interrupt source
//=================================================================================================
void UioInterface::monitorInterrupts(int uioDevice)
{
    int      uiofd;
    int      configfd;
    int      err;
    uint32_t notification;
    uint8_t  commandHigh;
    char     filename[64];

    while (true) try
    {    
        // Generate the filename of the psudeo-file that notifies us of interrupts
        sprintf(filename, "/dev/uio%d", uioDevice);

        // Open the psuedo-file that notifies us of interrupts
        uiofd = open(filename, O_RDONLY);
        if (uiofd < 0) throw crash(CRASH_OPEN_PDEVICE);

        // Generate the filename of the PCI config-space psuedo-file
        sprintf(filename, "/sys/class/uio/uio%d/device/config", uioDevice);

        // Open the file that gives us access to the PCI device's confiuration space
        configfd = open(filename, O_RDWR);
        if (configfd < 0) throw crash(CRASH_OPEN_CONFIG);

        // Fetch the upper byte of the PCI configuration space command word
        err = pread(configfd, &commandHigh, 1, 5);
        if (err != 1) throw crash(CRASH_PREAD_1);
    
        // Turn off the "Disable interrupts" flag
        commandHigh &= ~0x4;

        // Loop forever, monitoring incoming interrupt notifications
        while (true)
        {
            // Enable (or re-enable) interrupts
            err = pwrite(configfd, &commandHigh, 1, 5);
            if (err != 1) throw crash(CRASH_PREAD_2);

            // Wait for notification that an interrupt has occured
            err = read(uiofd, &notification, 4);

            // If this read fails, it means that a hot-reset of the PCI bus occured
            if (err == -1)
            {
                close(configfd);
                close(uiofd);
                usleep(2000000);
                break;
            }
            
            // If we didn't read exactly 4 bytes, something is seriously wrong
            if (err != 4) throw crash(CRASH_READ_LEN);

            // Give the ISR a chance to handle and clear the interrupts
            handler_->topLevelHandler();
        }
    }

    // Call the crash handler, and exit the thread
    catch(crash& reason)
    {
        crashHandler(reason.code);
    }
}
//=================================================================================================



//=================================================================================================
// crashHandler() - Default crash handler - gets called if monitorInterrupts() crashes
//=================================================================================================
void UioInterface::crashHandler(int reason)
{
    printf("interrupt monitoring crashed! reason = %d\n", reason);
}
//=================================================================================================

