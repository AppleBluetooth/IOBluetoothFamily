/*
 * Copyright (c) 2021 Apple Inc. All rights reserved.
 * Copyright (c) 2021 cjiang. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include <IOKit/bluetooth/transport/IOBluetoothHostControllerUSBTransport.h>
#include <IOKit/bluetooth/IOBluetoothMemoryBlock.h>
#include <IOKit/IONotifier.h>
#include <IOKit/IOPlatformExpert.h>
#include <IOKit/pwr_mgt/RootDomain.h>

#define super IOBluetoothHostControllerTransport

#define RequireFailureLog(err) os_log(OS_LOG_DEFAULT, "REQUIRE failure: %s - file: %s:%d\n", err, "/System/Volumes/Data/SWE/macOS/BuildRoots/2288acc43c/Library/Caches/com.apple.xbs/Sources/IOBluetoothFamily_kexts/IOBluetoothFamily-8004.1.18.3/Core/Family/HCI/Transports/USB/IOBluetoothHostControllerUSBTransport/IOBluetoothHostControllerUSBTransport.cpp", 0)
#define CheckFailureLog(err) os_log(OS_LOG_DEFAULT, "CHECK failure: %s - file: %s:%d\n", err, "/System/Volumes/Data/SWE/macOS/BuildRoots/2288acc43c/Library/Caches/com.apple.xbs/Sources/IOBluetoothFamily_kexts/IOBluetoothFamily-8004.1.18.3/Core/Family/HCI/Transports/USB/IOBluetoothHostControllerUSBTransport/IOBluetoothHostControllerUSBTransport.cpp", 0)
#define RequireNoErrFailureLog(err) os_log(OS_LOG_DEFAULT, "REQUIRE_NO_ERR failure: 0x%x - file: %s:%d\n", "/System/Volumes/Data/SWE/macOS/BuildRoots/2288acc43c/Library/Caches/com.apple.xbs/Sources/IOBluetoothFamily_kexts/IOBluetoothFamily-8004.1.18.3/Core/Family/HCI/Transports/USB/IOBluetoothHostControllerUSBTransport/IOBluetoothHostControllerUSBTransport.cpp", 0)

enum
{
    kPowerStateOff = 0,
    kPowerStateInitial,
    kPowerStateOn,
    kPowerStateCount
};

static IOPMPowerState powerStateArray[kPowerStateCount] =
{
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, kIOPMInitialDeviceState, kIOPMPowerOn, kIOPMInitialDeviceState, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, kIOPMDeviceUsable, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
};

OSDefineMetaClassAndAbstractStructors(IOBluetoothHostControllerUSBTransport, super)

bool IOBluetoothHostControllerUSBTransport::init(OSDictionary * dictionary)
{
    CreateOSLogObject();
    mExpansionData = IONew(ExpansionData, 1);
    if (!mExpansionData)
        return false;
    bzero(mExpansionData, 8);
    mExpansionData->reserved = 0LL;
    *(_BYTE *)(this + 1710) = 0;
    *(_QWORD *)(this + 328) = 0LL;
    *(_QWORD *)(this + 1392) = 0LL;
    *(_QWORD *)(this + 1448) = 0LL;
    *(_QWORD *)(this + 1504) = 0LL;
    *(_QWORD *)(this + 1536) = 0LL;
    *(_QWORD *)(this + 1608) = 0LL;
    *(_BYTE *)(this + 1440) = 0;
    *(_BYTE *)(this + 1496) = 0;
    *(_BYTE *)(this + 1572) = 0;
    *(_WORD *)(this + 1668) = 0;
    *(_QWORD *)(this + 1576) = 0LL;
    *(_DWORD *)(this + 1568) = 0;
    *(_QWORD *)(this + 1592) = 0LL;
    *(_QWORD *)(this + 1640) = 0LL;
    *(_QWORD *)(this + 1656) = 0LL;
    *(_QWORD *)(this + 1680) = 0LL;
    *(_DWORD *)(this + 1648) = 0;
    *(_DWORD *)(this + 1604) = 0;
    *(_DWORD *)(this + 1600) = 0;
    *(_DWORD *)(this + 1664) = 0;
    *(_DWORD *)(this + 1704) = 0;
    *(_WORD *)(this + 1708) = 0;
    *(_WORD *)(this + 182) = 0;
    *(_DWORD *)(this + 196) = 2;
    *(_DWORD *)(this + 1711) = 0;
    *(_DWORD *)(this + 1716) = 250;
    *(_WORD *)(this + 1720) = 0;
    *(_BYTE *)(this + 1722) = 0;
    *(_WORD *)(this + 344) = 0;
    *(_WORD *)(this + 1442) = 0;
    *(_WORD *)(this + 1498) = 0;
    *(_DWORD *)(this + 1724) = 0;
    bzero((void *)(this + 370), 1021);
    return super::init();
}

void IOBluetoothHostControllerUSBTransport::free()
{
    if ( *(_QWORD *)(this + 1688) )
    {
        IOFree(*(_QWORD *)(this + 1688), 1021LL);
        *(_QWORD *)(this + 1688) = 0LL;
    }
    if (mMessageReceiverNotifier)
    {
        mMessageReceiverNotifier->remove();
        mMessageReceiverNotifier = NULL;
    }
    OSSafeReleaseNULL(mInterruptReadDataBuffer);
    OSSafeReleaseNULL(mBulkInReadDataBuffer);
    DestroyTransportSCOParameters();
    IOSafeDeleteNULL(mExpansionData, ExpansionData, 1);
    mBluetoothUSBHostDevice = NULL;
    super::free();
}

IOService * IOBluetoothHostControllerUSBTransport::probe(IOService * provider, SInt32 * score)
{
    IOService * result;
    IOUSBHostDevice * device;
    OSNumber * locationID;
    UInt16 vendorID;
    UInt16 productID;
    IORegistryEntry * dtPlaneEntry;
    OSNumber * bDeviceClass;

    result = super::probe(provider, score);
    device = OSDynamicCast(IOUSBHostDevice, provider);
    if ( !device )
        return NULL;
    
    locationID = OSDynamicCast(OSNumber, provider->getProperty("locationID"));
    if ( !device->getDeviceDescriptor() )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][probe] -- device->getDeviceDescriptor() returned NULL -- exiting -- returning %p -- this = 0x%04X ****\n\n", NULL, ConvertAddressToUInt32(this));
        return NULL;
    }
    
    vendorID = device->getDeviceDescriptor()->idVendor;
    productID = device->getDeviceDescriptor()->idProduct;
    
    if ( locationID )
        mLocationID = locationID->unsigned32BitValue();
    
    if ( !(device->getPortStatus() & 2) )
    {
        mBuiltIn = false;
        
        // Skip the DisableInternalBluetoothModule check as it is not built in.
        goto SKIP_DISABLE_INTERNAL_BLUETOOTH_MODULE;
    }
    
    mBuiltIn = true;
    
    dtPlaneEntry = IORegistryEntry::fromPath("/options", gIODTPlane);
    if ( dtPlaneEntry )
    {
        OSSafeReleaseNULL(dtPlaneEntry);
        if ( OSDynamicCast(OSData, dtPlaneEntry->getProperty("DisableInternalBluetoothModule")) )
        {
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][probe] -- Disable internal Bluetooth module ****\n");
            return NULL;
        }
    }
    
SKIP_DISABLE_INTERNAL_BLUETOOTH_MODULE:
    if ( vendorID != 1452 && mSwitchBehavior == 2 )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][probe] -- vid != kAppleBluetoothUSBDeviceVID AND Switch behavior is NEVER, ignoring this Bluetooth HCI Controller -- 0x%04x ****\n", ConvertAddressToUInt32(this));
    }
    if ( vendorID == 8888 && (productID == 2357) | (productID == 2209)
      || vendorID == 2830 && productID == 2309
      || vendorID == 1151 && productID == 2
      || vendorID == 8047
      || vendorID == 2830 )
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][probe] -- Ignoring incorrectly identified Bluetooth HCI Controller -- 0x%04x ****\n", ConvertAddressToUInt32(this));
    
    if ( vendorID == 2652 && productID == 8243 )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][probe] -- Ignoring unsupported Broadcom 2033 Bluetooth HCI Controller -- 0x%04x ****\n", ConvertAddressToUInt32(this));
        return NULL;
    }
    
    if ( vendorID != 1293 || productID != 23 )
    {
        if ( vendorID != 2652 || productID != 8703 )
        {
            if ( vendorID != 5843 )
                return false;
            
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][probe] -- Ignoring Frontline Bluetooth Protocol Analyzer ****\n");
            return NULL;
        }
        if ( mBuiltIn )
        {
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][probe] -- Broadcom Bluetooth HCI Controller PID 0x21FF -- Booting from ROM -- 0x%04x ****\n", ConvertAddressToUInt32(this));
            *(_BYTE *)(this + 296) = 1;
        }
    }
    else
    {
        bDeviceClass = OSDynamicCast(OSNumber, provider->getProperty("bDeviceClass"));
        if ( bDeviceClass && bDeviceClass->unsigned32BitValue() != 224 )
        {
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][probe] -- Ignoring unsupported matching and reused VID/PID Belkin USB Hub, because not actually a Bluetooth HCI Controller -- 0x%04x ****\n", ConvertAddressToUInt32(this));
            return NULL;
        }
    }
    return result;
}

bool IOBluetoothHostControllerUSBTransport::start(IOService * provider)
{
    const StandardUSB::DeviceDescriptor * descriptor;
    OSNumber * locationID;
    IOService * usbDeviceProvider;
    OSString * ioClass;
    OSNumber * bcdDevice;
    const char * vendorName;
  
    mBluetoothUSBHostDevice = OSDynamicCast(IOUSBHostDevice, provider);
    if ( mBluetoothUSBHostDevice && mBluetoothUSBHostDevice == provider )
    {
        setProperty("InterfaceMatched", false);
        mMatchedOnInterface = 0;
    }
    else if ( OSDynamicCast(IOUSBHostInterface, provider) )
    {
        setProperty("InterfaceMatched", true);
        mMatchedOnInterface = 1;
        mBluetoothUSBHostDevice = OSDynamicCast(IOUSBHostDevice, provider->getProvider());
    }
    if ( !super::start(provider) )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][start] -- super::start() failed -- result = %s -- 0x%04x ****\n", "FALSE", ConvertAddressToUInt32(this));
        return false;
    }

    descriptor = mBluetoothUSBHostDevice->getDeviceDescriptor();
    if ( !descriptor )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][start] -- exiting -- getDeviceDescriptor() failed -- result = %s -- 0x%04x ****\n", "FALSE", ConvertAddressToUInt32(this));
        return false;
    }
    
    mCurrentInternalPowerState = 4294967297LL; //*(_QWORD *)(this + 216) = 4294967297LL;
    
    locationID = OSDynamicCast(OSNumber, provider->getProperty("locationID"));
    mVendorID = descriptor->idVendor;
    mProductID = descriptor->idProduct;
    usbDeviceProvider = OSDynamicCast(IOService, mBluetoothUSBHostDevice->getProvider());
    if ( usbDeviceProvider )
    {
        if ( usbDeviceProvider->getProperty("IOClass") )
        {
            ioClass = OSDynamicCast(OSString, usbDeviceProvider->getProperty("IOClass"));
            if ( ioClass )
            {
                if ( ioClass->isEqualTo("AppleUSBXHCIPCI") )
                    mIOClassIsAppleUSBXHCIPCI = 1;
            }
        }
    }
    if ( mVendorID > 2577 )
    {
        if ( mVendorID == 2652 )
        {
            mPowerMask = 4;
            vendorName = "Broadcom";
        }
        else
        {
            if ( mVendorID != 2578 )
            {
UNKNOWN_VENDOR:
                mPowerMask = 1;
                vendorName = "Unknown";
                goto SET_VENDOR_NAME;
            }
            bcdDevice = OSDynamicCast(OSNumber, provider->getProperty("bcdDevice"));
            if ( !bcdDevice || bcdDevice->unsigned16BitValue() <= 0x442u )
            {
                mPowerMask = 5;
                setProperty("ActiveBluetoothControllerVendor", "CSR - Older Firmware");
                setProperty("OlderFirmware", true);
                goto SET_NAME_COMPLETE;
            }
            mPowerMask = 5;
            vendorName = "CSR";
        }
    }
    else
    {
        if ( mVendorID != 1293 )
        {
            if ( mVendorID == 1452 )
                goto SET_NAME_COMPLETE;
            goto UNKNOWN_VENDOR;
        }
        mPowerMask = 4;
        vendorName = "Belkin";
    }
SET_VENDOR_NAME:
    setProperty("ActiveBluetoothControllerVendor", vendorName);
SET_NAME_COMPLETE:
    if ( locationID )
    {
        mLocationID = locationID->unsigned32BitValue();
        setProperty("LocationID", mLocationID, 32);
    }
    if ( mBluetoothUSBHostDevice->getPortStatus() & 2 )
    {
        mBuiltIn = true;
        setProperty("Built-In", true);
    }
    else
    {
        mBuiltIn = false;
        setProperty("Built-In", false);
    }
    
    if ( !isServiceRegistered )
    {
        isServiceRegistered = true;
        registerService();
    }
    
    os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][start] -- completed -- result = %s -- 0x%04x ****\n", "TRUE", ConvertAddressToUInt32(this));
    return true;
}

void IOBluetoothHostControllerUSBTransport::stop(IOService * provider)
{
    super::stop(provider);
   
    if ( mBluetoothUSBHostDevice && mHostDeviceStarted )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][stop] -- calling release() on mBluetoothUSBHostDevice -- Matched on %s ****\n", mMatchedOnInterface ? "interface" : "device");
        OSSafeReleaseNULL(mBluetoothUSBHostDevice);
        mHostDeviceStarted = false;
    }
    if ( *(_QWORD *)(this + 336) )
    {
        if ( *(_BYTE *)(this + 1727) )
        {
            OSSafeReleaseNULL((_QWORD *)(this + 336));
            *(_BYTE *)(this + 1727) = 0;
        }
    }
}

bool IOBluetoothHostControllerUSBTransport::terminateWL(IOOptionBits options)
{
    bool result;

    os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][terminateWL] -- entering -- mCurrentInternalPowerState = %s -- this = 0x%04x ****\n", gInternalPowerStateString[mCurrentInternalPowerState], ConvertAddressToUInt32(this));
    mBluetoothFamilyLogPacket(248, "terminateWL +");
    
    if ( mBluetoothController )
        mBluetoothController->TransportTerminating(this);
    
    if ( mSupportWoBT && mBluetoothController && *(mBluetoothController + 966) )
        AbortPipesAndClose(true, true);
    else
    {
        mBluetoothFamily->messageClients(3758227465);
        if ( mBluetoothController )
            mBluetoothController->CleanUpBeforeTransportTerminate(this);
        AbortPipesAndClose(true, true);
        mCommandGate->commandWakeup(&mCurrentInternalPowerState);
    }
    result = super::terminateWL(options);
    
    os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][terminateWL] -- exiting -- (matched on %s) -- this = 0x%04x ****\n\n", mMatchedOnInterface ? "Interface" : "Device", ConvertAddressToUInt32(this));
    mBluetoothFamilyLogPacket(248, "terminateWL -");
    
    return result;
}

IOReturn IOBluetoothHostControllerUSBTransport::ProcessG3StandByWake()
{
    return kIOReturnSuccess;
}

bool IOBluetoothHostControllerUSBTransport::InitializeTransportWL(IOService * provider)
{
    UInt8 cnt;

    if (!provider || isInactive() || !mBluetoothUSBHostDevice)
        return false;
    
    if ( !mBluetoothUSBHostDevice->getProperty("USB Product Name") )
        mBluetoothUSBHostDevice->setProperty("USB Product Name", "Bluetooth USB Host Controller");
    
    if ( !mBluetoothUSBHostDevice->getProperty("USB Vendor Name") && mVendorID == 1452 )
        provider->setProperty("USB Vendor Name", "Apple Inc.");

    *(_QWORD *)(this + 1688) = IOMalloc(1021LL);
    mInterruptDataLength = 0;
    mInterruptDataPosition = 0;
    
    cnt = 1;
    while ( !mBluetoothUSBHostDevice->open(this) )
    {
        IOSleep(100LL);
        if ( cnt > 9 )
        {
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][InitializeTransportWL] -- Even with retries, open() on the device failed or device was not an IOUSBHostDevice.\n");
            return false;
        }
        ++cnt;
    }

    if ( mVendorID != 2652 || mProductID != 8249 && mProductID != 8243 )
    {
        if ( ConfigureDevice() && FindInterfaces() )
        {
            if ( *(_BYTE *)(mBluetoothFamily + 416) )
                mBluetoothFamily->StartFullWakeTimer();
            if ( StartInterruptPipeRead() )
            {
                if ( !mBluetoothController || !*(_BYTE *)(mBluetoothController + 966) || StartBulkPipeRead() )
                {
                    if ( mTerminateState )
                    {
                        mUSBControllerSupportsSuspend = USBControllerSupportsSuspend();
                        mMessageReceiverNotifier = mBluetoothUSBHostDevice->registerInterest(gIOGeneralInterest, IOBluetoothHostControllerUSBTransport::MessageReceiver, this);
                        return super::InitializeTransportWL(provider);
                    }
                }
            }
        }
    }
    os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][InitializeTransportWL] -- BROADCOM DEVICE %x - NO FIRMWARE PRESENT. Bailing now...\n", mProductID);
    
    if ( *(_BYTE *)(mBluetoothFamily + 416) )
        mBluetoothFamily->CancelFullWakeTimer();
    AbortPipesAndClose(true, true);
    os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][InitializeTransportWL] -- failed -- calling DoDeviceReset (kBluetoothControllerReEnumerateModule) -- 0x%04x ****\n", ConvertAddressToUInt32(this));
    DoDeviceReset(1);
    return false;
}

void IOBluetoothHostControllerUSBTransport::AbortPipesAndClose(bool stop, bool release)
{
    if (mAbortPipesAndCloseCalled)
        return;
    
    mAbortPipesAndCloseCalled = 1;
    if ( stop )
    {
        if ( mInterruptPipe && mInterruptPipeStarted )
            StopInterruptPipeRead();
        if ( mBulkInPipe && mBulkInPipeStarted )
            StopBulkPipeRead();
          if ( mBulkOutPipe )
              mBulkOutPipe->abort();
          if ( mIsochInPipe && mIsochInReadsSucceeded )
              StopIsochPipeRead();
    }
    if ( release )
    {
        OSSafeReleaseNULL(mBulkInPipe);
        OSSafeReleaseNULL(mBulkOutPipe);
        OSSafeReleaseNULL(mInterruptPipe);
      
        if ( mInterface )
        {
            mInterface->close(this);
            if ( mInterfaceFound )
            {
                mInterfaceFound = false;
                mInterface->release();
            }
            mInterface = NULL;
        }
        if ( mIsochInterface )
        {
            OSSafeReleaseNULL(mIsochInPipe);
            OSSafeReleaseNULL(mIsochOutPipe);
            
            if ( mBluetoothUSBHostDevice && !mBluetoothUSBHostDevice->isInactive() )
                mIsochInterface->selectAlternateSetting(0);
            mIsochInterface->close(this);
            
            if ( mIsochInterfaceFound )
            {
                mIsochInterfaceFound = 0;
                mIsochInterface->release();
            }
            mIsochInterface = NULL;
        }
        if ( mBluetoothUSBHostDevice )
            mBluetoothUSBHostDevice->close(this);
        OSSafeReleaseNULL(mInterruptReadDataBuffer);
        OSSafeReleaseNULL(mBulkInReadDataBuffer);
    }
}

unsigned long IOBluetoothHostControllerUSBTransport::maxCapabilityForDomainState(IOPMPowerFlags domainState)
{
    unsigned long result;
    
    result = super::maxCapabilityForDomainState(domainState);
    if ( domainState == &loc_10000 && !mCurrentInternalPowerState )
        return 0;
    return result;
}

bool IOBluetoothHostControllerUSBTransport::NeedToTurnOnUSBDebug()
{
    IOService * rootNode;
    char * log;
    OSObject * propertyValue;
    OSData * boardID;
    
    log = (char *)IOMalloc(511);
        if ( !log )
          return false;

    rootNode = IOService::getPlatform()->getProvider();
    if ( !rootNode )
    {
        snprintf(log, 511, "**** [IOBluetoothHostControllerUSBTransport][NeedToTurnOnUSBDebug] -- Cannot check machine ID because rootNode is NULL");
        os_log(mInternalOSLogObject, "%s", log);
        
LOG_FAILED:
        if ( mBluetoothFamily )
            mBluetoothFamily->LogPacket(250, (void *) log, strlen(log));
        IOFree(log, 511);
        return false;
    }
    
    propertyValue = rootNode->getProperty("board-id");
    if ( !propertyValue )
    {
        snprintf(log, 511, "**** [IOBluetoothHostControllerUSBTransport][NeedToTurnOnUSBDebug] -- Cannot check machine ID because propertyValue is NULL");
        os_log(mInternalOSLogObject, "%s", log);
        goto LOG_FAILED;
    }
    
    boardID = OSDynamicCast(OSData, propertyValue);
    if ( !boardID )
    {
        snprintf(log, 511, "**** [IOBluetoothHostControllerUSBTransport][NeedToTurnOnUSBDebug] -- Cannot check machine ID because boardID is NULL");
        os_log(mInternalOSLogObject, "%s", log);
        goto LOG_FAILED;
    }
    
    if ( boardID->getBytesNoCopy() &&  boardID->getLength() >= 0x14)
    {
        if (    (  !strncmp((const char *)boardID->getBytesNoCopy(), "Mac-189A3D4F975D5FFC", 0x14uLL)
                || !strncmp((const char *)boardID->getBytesNoCopy(), "Mac-3CBD00234E554E41", 0x14uLL)
                || !strncmp((const char *)boardID->getBytesNoCopy(), "Mac-2BD1B31983FE1663", 0x14uLL)
                || !strncmp((const char *)boardID->getBytesNoCopy(), "Mac-E43C1C25D4880AD6", 0x14uLL)
                || !strncmp((const char *)boardID->getBytesNoCopy(), "Mac-06F11FD93F0323C5", 0x14uLL)
                || !strncmp((const char *)boardID->getBytesNoCopy(), "Mac-06F11F11946D27C5", 0x14uLL)) )
        {
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][NeedToTurnOnUSBDebug] -- Needs to turn on USB debug logging due to excessive USB issues with the internal Bluetooth module ****\n");
            return true;
        }
    }
    return false;
}

unsigned long IOBluetoothHostControllerUSBTransport::initialPowerStateForDomainState()
{
    return 2;
}

void IOBluetoothHostControllerUSBTransport::ResetBluetoothDevice()
{
    if ( mBluetoothUSBHostDevice )
        mBluetoothUSBHostDevice->reset();
}

UInt16 IOBluetoothHostControllerUSBTransport::GetControllerVendorID()
{
    return mVendorID;
}

UInt16 IOBluetoothHostControllerUSBTransport::GetControllerProductID()
{
    return mProductID;
}

BluetoothHCIPowerState IOBluetoothHostControllerUSBTransport::GetRadioPowerState()
{
    return kBluetoothHCIPowerStateOFF;
}

void IOBluetoothHostControllerUSBTransport::SetRadioPowerState(BluetoothHCIPowerState inState)
{
    return;
}

bool IOBluetoothHostControllerUSBTransport::ControllerSupportWoBT()
{
    return false;
}

bool IOBluetoothHostControllerUSBTransport::StartLMPLogging()
{
    return true;
}

bool IOBluetoothHostControllerUSBTransport::StartLMPLoggingBulkPipeRead()
{
    return true;
}

IOReturn IOBluetoothHostControllerUSBTransport::ToggleLMPLogging(UInt8 *)
{
    return kIOReturnSuccess;
}

IOReturn IOBluetoothHostControllerUSBTransport::TransportLMPLoggingBulkOutWrite(UInt8, UInt8)
{
    return kIOReturnSuccess;
}

bool IOBluetoothHostControllerUSBTransport::SystemWakeCausedByBluetooth()
{
    IOPMrootDomain * rootDomain;
    OSString * reason;
    char str[128];

    rootDomain = IOService::getPMRootDomain();
    if ( !this )
        return 0;
    
    OSDynamicCast(OSString, rootDomain->getProperty("Wake Type"));
    
    reason = OSDynamicCast(OSString, rootDomain->getProperty("Wake Reason"));
    if ( !reason )
        return 0;
    
    strlcat(str, reason->getCStringNoCopy(), 128);
    if ( reason->isEqualTo("XHC1") || reason->isEqualTo("EHC1") )
        return 1;
    return 0;
}

void IOBluetoothHostControllerUSBTransport::systemWillShutdownWL(IOOptionBits options, void *)
{
    mBluetoothController->CallCancelTimeoutForIdleTimer();
    if ( mIsControllerActive )
        mBluetoothFamily->UpdateNVRAMControllerInfo();
    mSystemOnTheWayToSleep = 1;
    *(_BYTE *)(this + 183) = 1;
    
    if ( mCurrentInternalPowerState == 12884901891 )
    {
        CallPowerManagerChangePowerStateTo(2, "IOBluetoothHostControllerUSBTransport::systemWillShutdownWL()");
        WaitForControllerPowerStateWithTimeout(kIOBluetoothHCIControllerInternalPowerStateOn, 5000000, "IOBluetoothHostControllerUSBTransport::systemWillShutdownWL()", false);
    }
    if ( options == -536870320 )
    {
        if ( mCurrentInternalPowerState != kIOBluetoothHCIControllerInternalPowerStateOn )
            return;
        mBluetoothController->PerformTaskForPowerManagementCalls(3758096976);
    }
    else if (options == -536870128)
    {
        mBluetoothController->DisableScan();
        mBluetoothController->PerformTaskForPowerManagementCalls(3758097168);
    }
}

bool IOBluetoothHostControllerUSBTransport::ConfigureDevice()
{
    const StandardUSB::DeviceDescriptor* descriptor;
    const StandardUSB::ConfigurationDescriptor * configDescriptor;
    IOReturn result;
    char * errStringLong;
    char * errStringShort;

    if ( !mBluetoothUSBHostDevice )
      return false;
    
    descriptor = mBluetoothUSBHostDevice->getDeviceDescriptor();
    if ( !descriptor || descriptor->bNumConfigurations == 0 )
        return false;
    
    for (UInt8 i = 0; i < descriptor->bNumConfigurations - 1; ++i)
    {
        configDescriptor = mBluetoothUSBHostDevice->getConfigurationDescriptor(i);
        if ( configDescriptor && StandardUSB::getNextInterfaceDescriptor(configDescriptor, NULL) )
            break;
    }
    
    result = mBluetoothUSBHostDevice->setConfiguration(configDescriptor->bConfigurationValue, 0);
    if ( result )
    {
        if ( mBluetoothFamily )
            mBluetoothFamily->ConvertErrorCodeToString(result, errStringLong, errStringShort);
        return false;
    }
    return true;
}

IOReturn IOBluetoothHostControllerUSBTransport::SetIdlePolicyValue(uint32_t idleTimeoutMs)
{
    IOReturn result;
    char * errStringLong;
    char * errStringShort;

    if ( !mInterruptPipe )
    {
        result = -536870212;
        goto OVER;
    }
    result = mInterruptPipe->setIdlePolicy(idleTimeoutMs);
    if ( mBluetoothFamily )
        mBluetoothFamily->ConvertErrorCodeToString(result, errStringLong, errStringShort);
    
    if ( result )
    {
        mBluetoothFamilyLogPacket(248, "Failed setIdlePolicy (%d ms) Interrupt Pipe", idleTimeoutMs);
        goto OVER;
    }
    
    mBluetoothFamilyLogPacket(248, "Success setIdlePolicy (%d ms) Interrupt Pipe", idleTimeoutMs);

    if ( mBulkInPipe )
    {
        result = mBulkInPipe->setIdlePolicy(idleTimeoutMs);
        if ( mBluetoothFamily )
            mBluetoothFamily->ConvertErrorCodeToString(result, errStringLong, errStringShort);
        
        mBluetoothFamilyLogPacket(248, result ? "Failed setIdlePolicy (%d ms) Bulk Pipe" : "Success setIdlePolicy (%d ms) Bulk Pipe", idleTimeoutMs);
    }
OVER:
    if ( idleTimeoutMs )
        changePowerStateTo(1);
    else
    {
        changePowerStateTo(2);
        mCurrentPMMethod = 6;
    }
    if ( mBluetoothFamily )
        mBluetoothFamily->ConvertErrorCodeToString(result, errStringLong, errStringShort);
    return result;
}

bool IOBluetoothHostControllerUSBTransport::SetIdlePolicyValueAction(OSObject * owner, void * arg0, void * arg1, void * arg2, void * arg3)
{
    IOBluetoothHostControllerUSBTransport * transport;
    
    transport = OSDynamicCast(IOBluetoothHostControllerUSBTransport, owner);
    if ( transport && !transport->isInactive() )
        return transport->mBluetoothController->SetTransportIdlePolicyValue();
    return false;
}

IOReturn IOBluetoothHostControllerUSBTransport::ReConfigure()
{
    if ( mInterface )
        mInterface->close(this);
    if ( mIsochInterface )
        mIsochInterface->close(this);
    
    mBluetoothUSBHostDevice->close(this);
    
    if ( mBluetoothUSBHostDevice->open(this) && ConfigureDevice() && FindInterfaces() && StartInterruptPipeRead() && StartBulkPipeRead() )
    {
        if ( mBluetoothController )
            mBluetoothFamily->WakeUpDisplay();
        return kIOReturnSuccess;
    }
    
    //*(_BYTE *)(*(_QWORD *)(this + 144) + 1298LL) = 0;
    return -536870212;
}

IOReturn IOBluetoothHostControllerUSBTransport::HardReset()
{
    if ( mBluetoothController && !*(_BYTE *)(mBluetoothController + 897) )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][HardReset] -- Power state is changing, prohibiting Bluetooth hard reset error recovery ****\n");
        mBluetoothFamilyLogPacket(251, "**** [IOBluetoothHostControllerUSBTransport][HardReset] -- Power state is changing, prohibiting Bluetooth hard reset error recovery ****\n");
        return 3758097122;
    }
    
    if ( mBluetoothUSBHostDevice )
    {
        mBluetoothUSBHostDevice->retain();
        *(_QWORD *)(*(_QWORD *)(this + 136) + 472LL) = *(_QWORD *)(this + 328);
    }
    
    if ( mBluetoothUSBHub )
    {
        mBluetoothUSBHub->retain();
        *(_QWORD *)(*(_QWORD *)(this + 136) + 480LL) = *(_QWORD *)(this + 336);
    }
    
    retain();
    *(_QWORD *)(*(_QWORD *)(this + 136) + 464) = this;
    
    if ( mBluetoothController )
        mBluetoothController->BroadcastNotification(13, kIOBluetoothHCIControllerConfigStateUninitialized, kIOBluetoothHCIControllerConfigStateUninitialized);
    
    return mBluetoothFamily->USBHardReset();
}

void IOBluetoothHostControllerUSBTransport::LogData(void * data, UInt64, IOByteCount size)
{
    os_log(mInternalOSLogObject, "      ");
    
    for (UInt16 i = 1; i <= size; ++i)
    {
        os_log(mInternalOSLogObject, "%02X", ((UInt8 *) data)[i - 1]);
        if ( !(i & 3) )
            os_log(mInternalOSLogObject, " ");
        if ( !(i & 0xF) )
            os_log(mInternalOSLogObject, "\n");
    }
    
    os_log(mInternalOSLogObject, "\n");
}

bool IOBluetoothHostControllerUSBTransport::HostSupportsSleepOnUSB()
{
    IOService * provider;

    provider = getProvider();
    if ( !provider )
        return 1;
    
    while (provider->getProvider())
        provider = provider->getProvider();
    
    if ( strncmp(provider->getName(), "PowerBook1,1", 0xC) )
        return 1;
    return 0;
}

UInt8 IOBluetoothHostControllerUSBTransport::GetInterfaceNumber(IOUSBHostInterface * interface)
{
    if (!interface)
        return -1;
    
    if (interface->getInterfaceDescriptor())
        return interface->getInterfaceDescriptor()->bInterfaceNumber;
     
    os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][GetInterfaceNumber] -- interfaceDescriptor is NULL -- this = 0x%04X ****\n", ConvertAddressToUInt32(this));
    return -1;
}

bool IOBluetoothHostControllerUSBTransport::StartInterruptPipeRead()
{
    IOReturn err;
    char * errStringShort;
    char * errStringLong;
    
    if ( !mInterruptPipe )
        return false;
    if ( mInterruptPipeStarted )
        goto SUCCESS;
    err = mInterruptPipe->clearStall(1);
    if ( err )
    {
        mBluetoothFamily->ConvertErrorCodeToString(err, errStringLong, errStringShort);
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][StartInterruptPipeRead] -- mInterruptPipe->clearStall (true) failed with error 0x%04X (%s) -- 0x%04x ****\n", err, errStringLong, ConvertAddressToUInt32(this));
        return false;
    }
    
    mInterruptCompletion.owner = this;
    mInterruptCompletion.action = IOBluetoothHostControllerUSBTransport::InterruptReadHandler;
    mInterruptCompletion.parameter = NULL;
    
    RetainTransport("IOBluetoothHostControllerUSBTransport::StartInterruptPipeRead()");
    if ( mInterruptSleepMs >= 0xFB )
        IOSleep(mInterruptSleepMs);
    bzero((void *) mInterruptReadDataBuffer->getBytesNoCopy(), 0x3FD);
    err = mInterruptPipe->io(mInterruptReadDataBuffer, 1021, &mInterruptCompletion);
    if ( !err )
    {
        ++mInterruptPipeOutstandingIOCount;
        mInterruptPipeStarted = true;
SUCCESS:
        mHardwareStatus = mHardwareStatus & 0xFFFFFFF3 | 4;
        return true;
    }
    mBluetoothFamily->ConvertErrorCodeToString(err, errStringLong, errStringShort);
    os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][StartInterruptPipeRead] -- failed to read on the interrupt pipe: 0x%04X (%s) -- 0x%04x ****\n", err, errStringLong, ConvertAddressToUInt32(this));
    ReleaseTransport("IOBluetoothHostControllerUSBTransport::StartInterruptPipeRead()");
    return false;
}

bool IOBluetoothHostControllerUSBTransport::StopInterruptPipeRead()
{
    IOReturn err;
    char * errStringShort;
    char * errStringLong;
    
    if ( mInterruptPipe && mInterruptPipeStarted )
    {
        ++mStopInterruptPipeReadCounter;
        error = mInterruptPipe->abort();
        if ( mBluetoothUSBHostDevice )
        {
            mBluetoothFamily->ConvertErrorCodeToString(err, errStringLong, errStringShort);
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][StopInterruptPipeRead] -- mInterruptPipe->abort() returned 0x%04X (%s) ****\n", err, errStringLong);
        }
    }
    mInterruptPipeStarted = false;
    mHardwareStatus = mHardwareStatus & 0xFFFFFFF3 | 8;
    return true;
}

bool IOBluetoothHostControllerUSBTransport::StartBulkPipeRead()
{
    IOReturn err;
    char * errStringShort;
    char * errStringLong;
    
    if ( !mBulkInPipe )
        return false;
  
    if ( mBulkInPipeStarted )
        goto SUCCESS;
    
    err = mBulkInPipe->clearStall(true);
    if ( err )
    {
        mBluetoothFamily->ConvertErrorCodeToString(err, errStringLong, errStringShort);
        
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][StartBulkPipeRead] -- mBulkInPipe->clearStall (true) failed with error 0x%04X (%s) -- 0x%04x ****\n", err, errStringLong, ConvertAddressToUInt32(this));
        
        return false;
    }

    mBulkInCompletion.owner = this;
    mBulkInCompletion.action = IOBluetoothHostControllerUSBTransport::BulkInReadHandler;
    mBulkInCompletion.parameter = NULL;
    RetainTransport("IOBluetoothHostControllerUSBTransport::StartBulkPipeRead()");
    
    err = mBulkInPipe->io(mBulkInReadDataBuffer, 1088, &mBulkInCompletion);
    if ( !err )
    {
        ++mBulkInPipeOutstandingIOCount;
        mBulkInPipeStarted = true;
SUCCESS:
        mHardwareStatus = mHardwareStatus & 0xFFFFFFCF | 0x10;
        return true;
    }
    mBluetoothFamily->ConvertErrorCodeToString(err, errStringLong, errStringShort);
    ReleaseTransport("IOBluetoothHostControllerUSBTransport::StartBulkPipeRead()");

    return false;
}

bool IOBluetoothHostControllerUSBTransport::StopBulkPipeRead()
{
    IOReturn err;
    char * errStringShort;
    char * errStringLong;
    
    if ( mBulkInPipe && mBulkInPipeStarted )
    {
        err = mBulkInPipe->abort();
        mBluetoothFamily->ConvertErrorCodeToString(err, errStringLong, errStringShort);
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][StopBulkPipeRead] -- mBulkInPipe->abort() returned 0x%04X (%s) ****\n", err, errStringLong);
    }
    mHardwareStatus = mHardwareStatus & 0xFFFFFFCF | 0x20;
    return true;
}

bool IOBluetoothHostControllerUSBTransport::StartIsochPipeRead()
{
    IOReturn err;
    UInt8 i;
    bool isochInCompletionRoutineListIsNew;
    IOBufferMemoryDescriptor * memoryDescriptor;
    StandardUSB::Descriptor desc;
    char * errStringShort;
    char * errStringLong;
    
    if (mIsochInPipeNumReads)
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][StartIsochPipeRead] -- Read already pending -- 0x%04x ****\n", ConvertAddressToUInt32(this));
        goto SUCCESS;
    }
    
    if (!mBluetoothUSBHostDevice)
    {
        RequireFailureLog("( mBluetoothUSBHostDevice != NULL )");
FAILURE:
        OSSafeReleaseNULL(mIsochInPipe);
        OSSafeReleaseNULL(mIsochOutPipe);
        return false;
    }
  
    if (!mIsochInterface)
    {
        RequireFailureLog("( mIsochInterface != NULL )");
        goto FAILURE;
    }
          
    if ( mIsochInPipe )
        CheckFailureLog("mIsochInPipe == NULL");
    
    mIsochInPipe = FindNextPipe(mIsochInterface, 1, 1, &desc);
    if (!mIsochInPipe)
    {
        RequireFailureLog("( mIsochInPipe != NULL )");
        goto FAILURE;
    }
    
    mIsochOutPipe = FindNextPipe(mIsochInterface, 1, 0, &desc);
    if (!mIsochOutPipe)
    {
        RequireFailureLog("( mIsochOutPipe != NULL )");
        goto FAILURE;
    }
    
    if ( !mIsochInReadDataBufferList )
    {
        mIsochInReadDataBufferList = IONew(IOBufferMemoryDescriptor *, 2);
        if ( !mIsochInReadDataBufferList )
        {
            RequireFailureLog("( mIsochInReadDataBufferList != NULL )");
            goto FAILURE;
        }
        bzero(mIsochInReadDataBufferList, 16);
    }
    
    if (!mIsochInCompletionRoutineList)
    {
        isochInCompletionRoutineListIsNew = 1;
        if (!(mIsochInCompletionRoutineList = IONew(IOUSBHostIsochronousCompletion, 2)))
        {
            RequireFailureLog("( mIsochInCompletionRoutineList != NULL )");
            goto FAILURE;
        }
    }
    else
        isochInCompletionRoutineListIsNew = 0;
    
    if (!mIsochInFrames)
    {
        if (!(mIsochInFrames = IONew(IOUSBHostIsochronousFrame, 2 * mIsochInPipeNumFrames)))
        {
            RequireFailureLog("( mIsochInFrames != NULL )");
            goto FAILURE;
        }
    }
    
    ResetIsocFrames(mIsochInFrames, 2 * mIsochInPipeNumFrames);
    mIsochInFrameNumber = mIsochInterface->getFrameNumber() + 4;
    
    for (i = 0; i <= 1; ++i)
    {
        if ( !mIsochInReadDataBufferList[i] )
        {
            //IOBufferMemoryDescriptor::withOptions((&dword_10 + 3), mIsochInReadDataBufferLength, page_size);
            memoryDescriptor = IOBufferMemoryDescriptor::withOptions(kIODirectionOutIn, mIsochInReadDataBufferLength, page_size); //kIODirectionOutIn is guessed
            if ( !memoryDescriptor )
            {
                RequireFailureLog("( memoryDescriptor != NULL )");
                goto FAILURE;
            }
            memoryDescriptor->setLength(mIsochInReadDataBufferLength);
            memoryDescriptor->prepare();
            mIsochInReadDataBufferStates[i] = 1;
            mIsochInReadDataBufferList[i] = memoryDescriptor;
            bzero(memoryDescriptor->getBytesNoCopy(), mIsochInReadDataBufferLength);
        }
        if ( isochInCompletionRoutineListIsNew )
        {
            mIsochInCompletionRoutineList[i].owner = this;
            mIsochInCompletionRoutineList[i].action = IOBluetoothHostControllerUSBTransport::IsochInReadHandler;
            mIsochInCompletionRoutineList[i].parameter = (void *) i;
        }
        ++mIsochInPipeNumReads;
        RetainTransport("IOBluetoothHostControllerUSBTransport::StartIsochPipeRead()");
        err = mIsochInPipe->io(mIsochInReadDataBufferList[i], &mIsochInFrames[i], mIsochInPipeNumFrames, mIsochInFrameNumber, &mIsochInCompletionRoutineList[i]);
        if ( err )
        {
            --mIsochInPipeNumReads;
            ReleaseTransport("IOBluetoothHostControllerUSBTransport::StartIsochPipeRead()");
            
            mBluetoothFamily->ConvertErrorCodeToString(err, errStringLong, errStringShort);
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][StartIsochPipeRead] -- failed to read on the isoch in pipe: 0x%04X (%s)\n", err, errStringLong);
            goto FAILURE;
        }
        mIsochInReadDataBufferStates[i] = 2;
        mIsochInFrameNumber += mIsochInPipeNumFrames;
    }
    
    mIsochInReadsComplete = true;
      
SUCCESS:
    //*(_BYTE *)(*(_QWORD *)(this + 144) + 933LL) = 0;
    mHardwareStatus = mHardwareStatus & 0xFFFFFF3F | 0x40;
    return true;
}

bool IOBluetoothHostControllerUSBTransport::StopIsochPipeRead()
{
    if ( mIsochInPipe )
    {
        if ( mIsochInPipeNumReads )
            mIsochInPipe->abort();
        mIsochInPipe = NULL;
    }
    if ( mIsochOutPipe )
        mIsochOutPipe = NULL;
    //*(_BYTE *)(*(_QWORD *)(this + 144) + 933LL) = 1;
    mIsochInReadsComplete = false;
    mHardwareStatus = mHardwareStatus & 0xFFFFFF3F | 0x80;
    return true;
}

void IOBluetoothHostControllerUSBTransport::ResetIsocFrames(IOUSBHostIsochronousFrame * isocFrames, UInt32 numberOfFrames)
{
    if (!isocFrames)
    {
        RequireFailureLog("( isocFrames != NULL )");
        return;
    }
  
    for (int i = 0; i < numberOfFrames; ++i)
    {
        isocFrames[i].status = 0;
        isocFrames[i].requestCount = mIsochInFrameNumRequestedBytes;
        isocFrames[i].completeCount = 0;
        isocFrames[i].timeStamp = 0;
    }
}

bool IOBluetoothHostControllerUSBTransport::StopAllPipes()
{
    mPipesStarted = true;
    StopIsochPipeRead();
    StopBulkPipeRead();
    return StopInterruptPipeRead();
}

bool IOBluetoothHostControllerUSBTransport::StartAllPipes()
{
    mPipesStarted = false;
    // I wonder why it does not call StartIsochPipeRead(). Maybe it is called elsewhere.
    StartBulkPipeRead();
    return StartInterruptPipeRead();
}

void IOBluetoothHostControllerUSBTransport::WaitForAllIOsToBeAborted()
{
    for (UInt8 i = 0; i <= 4; ++i)
    {
        if (!mBulkInPipeOutstandingIOCount && !mInterruptPipeOutstandingIOCount && !*(UInt16 *)(this + 344))
            break;
        TransportCommandSleep(&mInterruptPipeOutstandingIOCount, 100, "IOBluetoothHostControllerUSBTransport::WaitForAllIOsToBeAborted()", 0);
    }
}

IOReturn IOBluetoothHostControllerUSBTransport::TransportBulkOutWrite(void * buffer)
{
    return BulkOutWrite((IOMemoryDescriptor *) buffer);
}

IOReturn IOBluetoothHostControllerUSBTransport::SetRemoteWakeUp(bool enable)
{
    IOReturn error;
    char * errStringShort;
    char * errStringLong;
    StandardUSB::DeviceRequest request;
    UInt32 bytesTransferred;
    
    if ( isInactive() || !mBluetoothUSBHostDevice || TerminateCalled() || mBluetoothUSBHostDevice->isInactive())
    {
        mBluetoothFamily->ConvertErrorCodeToString(65283, errStringLong, errStringShort);
        return 65283;
    }
    
    request.bmRequestType   = 0;
    request.bRequest        = 0;
    request.wValue          = 0;
    request.wIndex          = 0;
    request.wLength         = 0;
    
    /*
    LOBYTE(v12) = 0;
    v13 = 1;
    v14 = 0;
    HIBYTE(v12) = 2 * enable | 1;
    */
    
    error = -536870212;
    if ( !TerminateCalled() )
    {
        error = mBluetoothUSBHostDevice->deviceRequest(this, request, (IOBufferMemoryDescriptor *) NULL, bytesTransferred);
        if (!error)
        {
            setProperty("Remote-Wake", enable);
            return kIOReturnSuccess;
        }
        mBluetoothFamily->ConvertErrorCodeToString(error, errStringLong, errStringShort);
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][SetRemoteWakeUp] -- deviceRequest() failed: 0x%04X (%s)\n", error, errStringLong);
        mBluetoothFamilyLogPacket(250, "deviceRequest() 0x%04X %s", error, errStringShort);
    }
    return error;
}

IOReturn IOBluetoothHostControllerUSBTransport::PrepareControllerForSleep()
{
    IOReturn error;
    IOReturn result;
    char * errStringShort;
    char * errStringLong;
    IOBluetoothHCIControllerInternalPowerState state;
    
    WaitForSystemReadyForSleep("IOBluetoothHostControllerUSBTransport::PrepareControllerForSleep()");
    mPendingInternalPowerState = kIOBluetoothHCIControllerInternalPowerStateSleep;
    
    error = mBluetoothController->CleanupForPowerChangeFromOnToSleep(*(this + 249), &result);
    if ( error )
    {
        mBluetoothFamilyLogPacket(250, "Power state transition to SLEEP failed\n");
        *(_BYTE *)(this + 312) = 1;
        SetIdlePolicyValue(0);
    }
    mBluetoothFamily->ConvertErrorCodeToString(error, errStringLong, errStringShort);
    mBluetoothController->PerformTaskForPowerManagementCalls(3758097024);
    *(_BYTE *)(this + 248) = 0;

    if ( !result )
    {
        *(_BYTE *)(this + 1708) = 0;
        *(_QWORD *)(this + 216) = 8589934594LL;
        if ( mIsControllerActive )
        {
            mBluetoothController->UpdatePowerStateProperty(kIOBluetoothHCIControllerInternalPowerStateSleep, true);
            state = mCurrentInternalPowerState;
        }
        else
            state = kIOBluetoothHCIControllerInternalPowerStateSleep;
        
        *(_BYTE *)(this + 1711) = 0;
        mBluetoothController->UpdatePowerReports(state);
        if ( mBluetoothController )
        {
            mBluetoothController->SetChangingPowerState(false);
            if ( mSupportNewIdlePolicy )
                mBluetoothController->ChangeIdleTimerTime("IOBluetoothHostControllerUSBTransport::PrepareControllerForSleep()", *(unsigned int *)(*(_QWORD *)(this + 144) + 1264));
        }
    }
    return result;
}

IOReturn IOBluetoothHostControllerUSBTransport::PrepareControllerWakeFromSleep()
{
    IOReturn error;
    IOReturn result;
    char * errStringShort;
    char * errStringLong;
    
    mPendingInternalPowerState = kIOBluetoothHCIControllerInternalPowerStateOn;
    mSystemOnTheWayToSleep = false;
    *(_BYTE *)(mBluetoothController + 967) = 1;
    *(_BYTE *)(this + 1708) = 1;
    *(_BYTE *)(this + 1711) = 0;
    if ( mBluetoothController )
    {
        *(_BYTE *)(mBluetoothController + 1282) = 0;
        mBluetoothController->KillAllPendingRequests(false, false);
        error = mBluetoothUSBHostDevice->abortDeviceRequests(this, 0, 3758097131);
        mBluetoothFamily->ConvertErrorCodeToString(error, errStringLong, errStringShort);
    }
    error = SetRemoteWakeUp(true);
    if ( error )
    {
        mBluetoothFamily->ConvertErrorCodeToString(error, errStringLong, errStringShort);
        if ( TransportWillReEnumerate() && (error == kIOReturnNoPower || error == kIOReturnNoDevice) )
        {
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][PrepareControllerWakeFromSleep] -- SLEEP -> ON -- SetRemoteWakeUp(TRUE) returned kIOReturnNoPower or kIOReturnNoDevice and waking up from either hibernate or G3Standby (ErP) -- no need to retry -- this = 0x%04x ****\n", ConvertAddressToUInt32(this));
            goto PM_SETUP;
        }
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][PrepareControllerWakeFromSleep] -- SLEEP -> ON -- SetRemoteWakeUp(TRUE) returned error 0x%04X (%s) -- this = 0x%04x ****\n", error, errStringLong, ConvertAddressToUInt32(this));
        mBluetoothFamilyLogPacket(250, "SLEEP->ON - SetRemoteWakeup(True) 0x%04X ", error);
    }
    
PM_SETUP:
    *(_BYTE *)(this + 248) = 0;
    *(_QWORD *)(this + 216) = 4294967297LL;
    *(_BYTE *)(this + 1708) = 0;
    mBluetoothController->PerformTaskForPowerManagementCalls(3758097184);
    if ( !mBluetoothController->ControllerSetupIsComplete() )
        mBluetoothController->SetupController(NULL);
    result = mBluetoothController->CleanUpForCompletePowerChangeFromSleepToOn();
    *(_BYTE *)(this + 182) = 0;
    mBluetoothController->CallUpdateTimerForIdleTimer();
    *(_BYTE *)(mBluetoothController + 912) = 1;
    *(_BYTE *)(mBluetoothController + 967) = 0;
    
    os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][PrepareControllerWakeFromSleep] -- complete: ON -- calling mBluetoothController->UpdatePowerReports() -- this = 0x%04x ****\n", ConvertAddressToUInt32(this));
    mBluetoothController->UpdatePowerReports(mCurrentInternalPowerState);
    
    if ( mBluetoothController )
    {
        mBluetoothController->SetChangingPowerState(false);
        if ( mSupportNewIdlePolicy )
            mBluetoothController->ChangeIdleTimerTime("IOBluetoothHostControllerUSBTransport::PrepareControllerWakeFromSleep()", *(unsigned int *)(*(_QWORD *)(this + 144) + 1264LL));
    }
    if ( mIsControllerActive )
        mBluetoothController->UpdatePowerStateProperty(mPendingInternalPowerState, true);
    
    if ( mBluetoothController && mCurrentInternalPowerState == kIOBluetoothHCIControllerInternalPowerStateOn )
        mBluetoothController->CompleteInitializeHostControllerVariables();
    
    if ( !*(_BYTE *)(mBluetoothController + 912) )
        *(_BYTE *)(mBluetoothController + 912) = 1;
    mCommandGate->commandWakeup(mBluetoothController + 967);
    
    if ( mBluetoothController )
    {
        *(_BYTE *)(mBluetoothController + 1282) = 1;
        mBluetoothController->ProcessWaitingRequests(false);
    }
    return result;
}

bool IOBluetoothHostControllerUSBTransport::ConfigurePM(IOService * policyMaker)
{
    OSNumber * number;
    OSDictionary * dict;
    IOService * provider;
    IOUSBHostDevice * device;
    int i;
    bool needWakeupNotification;
    
    if ( !mBluetoothUSBHostDevice )
      goto CONFIG_PM;
    
    provider = mBluetoothUSBHostDevice->getProvider();
    if ( !provider )
        return false;
    
    if ( NeedToTurnOnUSBDebug() )
    {
        number = OSNumber::withNumber(0x20, 32);
        dict = OSDictionary::withCapacity(0x20);
        dict->setObject("kUSBDebugOptions", number);
        OSSafeReleaseNULL(number);
        mBluetoothUSBHostDevice->setProperties(dict);
        
        device = OSDynamicCast(IOUSBHostDevice, provider);
        if ( device )
            device->setProperties(dict);
        
        provider = provider->getProvider();
        device = OSDynamicCast(IOUSBHostDevice, provider);
        if ( device )
            device->setProperties(dict);

        provider = provider->getProvider();
        if ( provider )
        {
            device = OSDynamicCast(IOUSBHostDevice, provider);
            mBluetoothUSBHub = device;
            if ( device )
            {
                device->retain();
                mProviderDeviceStarted = true;
                mBluetoothUSBHub->setProperties(dict);
            }
        }
        OSSafeReleaseNULL(dict);
        goto CONFIG_PM;
    }
    
    if ( provider->getProvider()->getProvider() )
        mBluetoothUSBHub = OSDynamicCast(IOUSBHostDevice, provider->getProvider()->getProvider());
        
CONFIG_PM:
    if ( !pm_vars )
    {
        PMinit();
        policyMaker->joinPMtree(this);
        if ( !pm_vars )
            return false;
    }
    
    mSupportPowerOff = mPowerMask < 6;
    if ( mPowerMask > 5 )
    {
        registerPowerDriver(this, &powerStateArray[kPowerStateOn], 1);
        setProperty("SupportPowerOff", false);
    }
    else
    {
        registerPowerDriver(this, powerStateArray, kPowerStateCount);
        setProperty("SupportPowerOff", true);
    }
    
    if ( mBluetoothFamily )
    {
        mBluetoothFamily->setProperty("TransportType", "USB");
        mConrollerTransportType = kBluetoothTransportTypeUSB;
    }
  
    if ( !mConfiguredPM && mPowerMask <= 5 && mCommandGate ) //SupportPowerOff
    {
        needWakeupNotification = true;
        
        for (i = 0; i <= 0x63; ++i)
        {
            if ( TransportCommandSleep(&mConfiguredPM, 300, "IOBluetoothHostControllerUSBTransport::ConfigurePM()", true) )
            {
                if ( mConfiguredPM )
                    needWakeupNotification = false;
                if ( !mBuiltIn )
                    mConfiguredPM = true;
            }
            else
                goto OVER;
            if ( !needWakeupNotification )
                goto OVER;
            if ( isInactive() )
                goto OVER;
            if ( mConfiguredPM )
                goto OVER;
        }
        if ( needWakeupNotification )
        {
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][ConfigurePM] -- ERROR -- waited 30 seconds and still did not get the commandWakeup() notification -- 0x%04x ****\n", ConvertAddressToUInt32(this));
            mConfiguredPM = true;
        }
    }
    
OVER:
    mBluetoothFamilyLogPacket(251, "USB Low Power");
    changePowerStateTo(1);
    ReadyToGo(mConfiguredPM);
    return true;
}

IOReturn IOBluetoothHostControllerUSBTransport::RequestTransportPowerStateChange(int newPowerState, char * name) //need to clean up the logic
{
    IOReturn result;
    IOReturn error;
    char * errStringShort;
    char * errStringLong;
    unsigned long ordinal;
    
    os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][RequestTransportPowerStateChange] -- entering -- %s -> %s -- this = 0x%04x ****\n", gInternalPowerStateString[mCurrentInternalPowerState], gInternalPowerStateString[newPowerState], ConvertAddressToUInt32(this));
    mBluetoothFamilyLogPacket(251, "%s->%s+Req", gInternalPowerStateString[mCurrentInternalPowerState], gInternalPowerStateString[newPowerState]);
    
    if ( mCurrentInternalPowerState != mPendingInternalPowerState )
    {
        if ( mPendingInternalPowerState != 3 )
        {
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][RequestTransportPowerStateChange] -- exiting -- Power state change in progress -- cannot perform RequestTransportPowerStateChange() -- %s -> %s -- this = 0x%04x ****\n\n", gInternalPowerStateString[mCurrentInternalPowerState], gInternalPowerStateString[newPowerState], ConvertAddressToUInt32(this));
            return -536870184;
        }
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][RequestTransportPowerStateChange] -- Power state change in progress -- %s -> IDLE -- wait for it to be completed ****\n", gInternalPowerStateString[mCurrentInternalPowerState]);
        
        if ( WaitForControllerPowerStateWithTimeout(kIOBluetoothHCIControllerInternalPowerStateIdle, 5000000, name, false) )
        {
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][RequestTransportPowerStateChange] -- exiting -- Power state change from %s -> IDLE failed ****\n\n", gInternalPowerStateString[mCurrentInternalPowerState]);
            return -536870184;
        }
    }
    
    result = kIOReturnSuccess;
    if ( mCurrentInternalPowerState != newPowerState )
    {
        if ( !newPowerState )
        {
            if ( mCurrentInternalPowerState == 2 || !(mBluetoothController->mControllerPowerOptions & 1) )
            {
                os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][RequestTransportPowerStateChange] -- exiting -- Controller does not support power OFF or already in SLEEP power state -- returned kIOReturnUnsupported -- %s -> %s -- this = 0x%04x ****\n\n", gInternalPowerStateString[mCurrentInternalPowerState], "OFF", ConvertAddressToUInt32(this));
                return kIOReturnUnsupported;
            }
        }
        if ( newPowerState != 1 && mCurrentInternalPowerState == 3 )
        {
            CallPowerManagerChangePowerStateTo(2, name);
            error = WaitForControllerPowerStateWithTimeout(kIOBluetoothHCIControllerInternalPowerStateOn, 5000000, name, false);
            if ( error )
            {
                RequireNoErrFailureLog(error);
LABEL_74:
                mPendingInternalPowerState = mCurrentInternalPowerState;
                result = error;
                goto LABEL_75;
            }
        }
        if ( newPowerState == 2 )
        {
            if ( !(mBluetoothController->mControllerPowerOptions & 6) || !mCurrentInternalPowerState || mBluetoothController->BluetoothRemoteWakeEnabled() ^ 1 )
                return kIOReturnUnsupported;
        }
        else if ( newPowerState )
        {
            if ( newPowerState == 1 )
            {
                *(_BYTE *)(this + 182) = 0;
                os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][RequestTransportPowerStateChange] -- calling PrepareControllerForPowerOn () -- called by %s -- %s -> %s -- this = 0x%04x ****\n\n", name, gInternalPowerStateString[mCurrentInternalPowerState], "ON", ConvertAddressToUInt32(this));
                error = PrepareControllerForPowerOn();
                mBluetoothFamilyLogPacket(251, "%s->%s-Req", gInternalPowerStateString[mCurrentInternalPowerState], "ON");
                mBluetoothFamilyLogPacket(251, "ON Complete");
                mBluetoothFamilyLogPacket(251, "USB Low Power");
                ordinal = 1;
                goto LABEL_73;
            }
        }
        else
        {
            *(_BYTE *)(this + 182) = 1;
            *(_BYTE *)(this + 248) = 1;
            RetainTransport("IOBluetoothHostControllerUSBTransport::RequestTransportPowerStateChange()");
            if ( mIsControllerActive )
                error = mBluetoothController->CleanUpForPoweringOff();
            else
                error = kIOReturnSuccess;
            ReleaseTransport("IOBluetoothHostControllerUSBTransport::RequestTransportPowerStateChange()");
            if ( error )
                mBluetoothFamily->ConvertErrorCodeToString(error, errStringLong, errStringShort);
            
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][RequestTransportPowerStateChange] -- newPowerState is OFF -- %s -> %s -- this = 0x%04x ****\n\n", gInternalPowerStateString[mCurrentInternalPowerState], "OFF", ConvertAddressToUInt32(this));
        }
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][RequestTransportPowerStateChange] -- calling PrepareControllerForPowerOff () -- called by %s -- %s -> %s -- this = 0x%04x ****\n\n", name, gInternalPowerStateString[mCurrentInternalPowerState], gInternalPowerStateString[newPowerState], ConvertAddressToUInt32(this));
        error = PrepareControllerForPowerOff(false);
        mBluetoothFamilyLogPacket(251, "OFF Complete");
        mBluetoothFamilyLogPacket(251, "Power Off");
    
        ordinal = 0;
        if ( mIsControllerActive )
        {
            mBluetoothController->UpdatePowerStateProperty(kIOBluetoothHCIControllerInternalPowerStateOff, true);
            if ( *(_BYTE *)(this + 248) )
                *(_BYTE *)(this + 251) = 0;
        }
        
LABEL_73:
        changePowerStateTo(ordinal);
        if ( error )
            goto LABEL_74;
    }
LABEL_75:
    *(_WORD *)(this + 1724) = 0;
    os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUSBTransport][RequestTransportPowerStateChange] -- exiting -- result = %s -- this = 0x%04x ****\n\n", result ? "FAILED" : "SUCCESS", ConvertAddressToUInt32(this));
  
LABEL_82:
    return result;
}

void IOBluetoothHostControllerUSBTransport::CompletePowerStateChange(char * name)
{
    char * errStringShort;
    char * errStringLong;
    
    if ( mCurrentInternalPowerState != mPendingInternalPowerState )
    {
        mCurrentInternalPowerState = mPendingInternalPowerState;
        if ( *(_BYTE *)(this + 1708) )
        {
            *(_BYTE *)(this + 1708) = 0;
            mBluetoothFamily->ConvertErrorCodeToString(mBluetoothUSBHostDevice->abortDeviceRequests(), errStringLong, errStringShort);
            StopAllPipes();
            WaitForAllIOsToBeAborted();
            acknowledgeSetPowerState();
        }
        if ( mIsControllerActive )
            mBluetoothController->UpdatePowerStateProperty(mPendingInternalPowerState, true);
        *(_BYTE *)(this + 1711) = 0;
        mBluetoothController->UpdatePowerReports(mCurrentInternalPowerState);
        mBluetoothFamilyLogPacket(251, "%s Complete", gInternalPowerStateString[mCurrentInternalPowerState]);
        
        mCurrentPMMethod = 3;
        if ( mBluetoothController )
        {
            mBluetoothController->SetChangingPowerState(false);
            if ( mSupportNewIdlePolicy )
                mBluetoothController->ChangeIdleTimerTime("IOBluetoothHostControllerUSBTransport::CompletePowerStateChange()", *(unsigned int *)(*(_QWORD *)(this + 144) + 1264LL));
        }
        mCommandGate->commandWakeup(this);
    }
    CancelBluetoothSleepTimer();
}

IOReturn IOBluetoothHostControllerUSBTransport::ProcessPowerStateChangeAfterResumed(char * name)
{
    if ( mCurrentInternalPowerState != kIOBluetoothHCIControllerInternalPowerStateIdle && mCurrentInternalPowerState )
        return kIOReturnSuccess;
    if ( StartInterruptPipeRead() && !mBluetoothController->mActiveConnections )
        return kIOReturnSuccess;
    if ( StartBulkPipeRead() )
        return kIOReturnSuccess;
    return -536870212;
}

IOReturn IOBluetoothHostControllerUSBTransport::powerStateWillChangeTo(IOPMPowerFlags capabilities, unsigned long stateNumber, IOService * whatDevice)
{
    if ( (IOService *) IOService::getPMRootDomain() == whatDevice && !isInactive() )
    {
        RetainTransport("IOBluetoothHostControllerUSBTransport::powerStateWillChangeTo()");
        super::powerStateWillChangeTo(capabilities, stateNumber, whatDevice);
        if ( capabilities & 2 )
        {
            *(_BYTE *)(this + 1725) = 1;
            mSystemOnTheWayToSleep = false;
            *(_BYTE *)(mBluetoothController + 967) = 1;
            if ( !mBluetoothController->mNumberOfCommandsAllowedByHardware )
                mBluetoothController->mNumberOfCommandsAllowedByHardware = 1;
        }
        ReleaseTransport("IOBluetoothHostControllerUSBTransport::powerStateWillChangeTo()");
    }
    return kIOReturnSuccess;
}

IOReturn IOBluetoothHostControllerUSBTransport::powerStateWillChangeToWL(IOOptionBits options, void *)
{
    if ( *(_BYTE *)(this + 248) )
        mBluetoothFamilyLogPacket(251, "OFF Already");
    
    if ( options == -536870112 )
    {
        *(_BYTE *)(this + 1725) = 1;
        mSystemOnTheWayToSleep = false;
        *(_BYTE *)(mBluetoothController + 967) = 1;
        if ( !mBluetoothController->mNumberOfCommandsAllowedByHardware )
            mBluetoothController->mNumberOfCommandsAllowedByHardware = 1;
    }
    else if ( options == -536870272 )
    {
        *(_BYTE *)(this + 1724) = 1;
        mCurrentPMMethod = 2;
        mBluetoothFamilyLogPacket(251, "Power On");
        changePowerStateTo(2);
        TransportCommandSleep(this + 1724, 5000, "IOBluetoothHostControllerUSBTransport::powerStateWillChangeToWL()", true);
    }
    return kIOReturnSuccess;
}

IOReturn IOBluetoothHostControllerUSBTransport::PrepareControllerForPowerOff(bool a2) //clean up logic
{
    IOReturn result;
    IOReturn err1;
    IOReturn err2;
    char * errStringShort;
    char * errStringLong;
    
    result = kIOReturnSuccess;
    mBluetoothFamilyLogPacket(251, "%s->OFF +", gInternalPowerStateString[mCurrentInternalPowerState]);
  
    mPendingInternalPowerState = kIOBluetoothHCIControllerInternalPowerStateOff;
    if ( mIsControllerActive )
    {
        err1 = mBluetoothController->CallBluetoothHCIReset(false, "IOBluetoothHostControllerUSBTransport::PrepareControllerForPowerOff()");
        mBluetoothFamily->ConvertErrorCodeToString(err1, errStringLong, errStringShort);
        
        if (mIsControllerActive)
        {
            if ( !*(_BYTE *)(this + 248) )
            {
LABEL_14:
                err2 = mBluetoothController->CallPowerRadio(false);
                mBluetoothFamily->ConvertErrorCodeToString(err2, errStringLong, errStringShort);
                
                if ( err1 || err2 )
                {
                    mBluetoothFamilyLogPacket(250, "Power state transition to OFF failed\n");
                    *(_BYTE *)(this + 312) = 1;
                    SetIdlePolicyValue(0);
                    goto LABEL_X;
                }
            }
            result = mBluetoothController->CleanUpForPoweringOff();
            
            if (mIsControllerActive)
            {
                if (*(_BYTE *)(this + 248))
                    mBluetoothController->PerformTaskForPowerManagementCalls(3758096976);
                goto LABEL_14;
            }
        }
    }
    
LABEL_X:
    if ( !mBluetoothController->BluetoothRemoteWakeEnabled() )
        SetRemoteWakeUp(false);
    StopAllPipes();
    *(_BYTE *)(this + 1708) = 0;
    if ( a2 )
    {
        mCurrentInternalPowerState = kIOBluetoothHCIControllerInternalPowerStateOff;
        if ( *(_BYTE *)(this + 240) )
            mBluetoothController->UpdatePowerStateProperty(kIOBluetoothHCIControllerInternalPowerStateOff, true);
    }
    *(_BYTE *)(this + 1711) = 0;
    mBluetoothController->UpdatePowerReports(mCurrentInternalPowerState);
    if ( mBluetoothController )
    {
        mBluetoothController->SetChangingPowerState(false);
        if ( *(_BYTE *)(this + 226) )
            mBluetoothController->ChangeIdleTimerTime("IOBluetoothHostControllerUSBTransport::PrepareControllerForPowerOff()", *(unsigned int *)(*(_QWORD *)(this + 144) + 1264LL));
    }
    mBluetoothFamilyLogPacket(251, "%s->OFF -", gInternalPowerStateString[mCurrentInternalPowerState]);
    return result;
}

bool IOBluetoothHostControllerUSBTransport::SystemGoingToSleep()
{
    IOReturn result;
    char * errStringShort;
    char * errStringLong;
    bool v6;
    
    if ( !mCurrentInternalPowerState )
        return kIOReturnSuccess;
    
    if ( mBluetoothController )
    {
        mBluetoothController->SetChangingPowerState(true);
        if ( mSupportNewIdlePolicy )
            mBluetoothController->ChangeIdleTimerTime("IOBluetoothHostControllerUSBTransport::SystemGoingToSleep()", *(unsigned int *)(*(_QWORD *)(this + 144) + 1264LL));
    }
    mCurrentPMMethod = 4;
    
    if ( mBluetoothFamily->mCurrentBluetoothHardware && mBluetoothFamily->mCurrentBluetoothHardware->mBluetoothTransport != this || *(_BYTE *)(mBluetoothFamily + 383) || !mIsControllerActive )
        v6 = 1;
    else
        v6 = 0;
    
    v6 |= !!(*(_BYTE *)(this + 248));
    if ( (mBluetoothController->mControllerPowerOptions & 6) != 6 || mBluetoothController->BluetoothRemoteWakeEnabled() ^ 1 || v6 )
    {
        *(_DWORD *)(this + 220) = 0;
        *(_BYTE *)(v5 + 432) = 1;
        *(_BYTE *)(mBluetoothController + 966) = 0;
        *(_BYTE *)(this + 1708) = 1;
        *(_BYTE *)(this + 1711) = 1;
        PrepareControllerForPowerOff(true);
        return kIOReturnSuccess;
    }
    
    *(_DWORD *)(this + 220) = 2;
    *(_BYTE *)(this + 244) |= 3;
    *(_BYTE *)(this + 1708) = 1;
    *(_BYTE *)(this + 1711) = 1;
    result = PrepareControllerForSleep();
    if ( !result )
    {
        v9 = mBluetoothUSBHostDevice->abortDeviceRequests(this);
        mBluetoothFamily->ConvertErrorCodeToString(v9, errStringLong, errStringShort);
        StopAllPipes();
        WaitForAllIOsToBeAborted();
    }
    return result;
}

OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 1)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 2)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 3)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 4)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 5)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 6)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 7)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 8)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 9)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 10)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 11)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 12)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 13)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 14)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 15)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 16)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 17)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 18)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 19)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 20)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 21)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 22)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerUSBTransport, 23)
