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

#include <IOKit/bluetooth/transport/IOBluetoothHostControllerUARTTransport.h>
#include <IOKit/IOBufferMemoryDescriptor.h>

#define super IOBluetoothHostControllerTransport

enum
{
    kPowerStateOff = 0,
    kPowerStateOn,
    kPowerStateCount
};

static IOPMPowerState powerStateArray[kPowerStateCount] =
{
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, kIOPMDeviceUsable, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
};

OSDefineMetaClassAndAbstractStructors(IOBluetoothHostControllerUARTTransport, super)

IOService * IOBluetoothHostControllerUARTTransport::probe(IOService * provider, SInt32 * score)
{
    IOService * result;
    IORegistryEntry * entry;

    result = super::probe(provider, score);
    entry = IORegistryEntry::fromPath("/options", gIODTPlane);
    if ( entry )
    {
        if ( OSDynamicCast(OSData, entry->getProperty("SkipIOBluetoothHostControllerUARTTransport")) )
        {
            result = NULL;
            os_log(mInternalOSLogObject, "IOBluetoothHostControllerUARTTransport::probe skip Bluetooth UART Transport");
        }
        
        if ( OSDynamicCast(OSData, entry->getProperty("SkipBluetoothFirmwareBoot")) )
        {
            mSkipBluetoothFirmwareBoot = true;
            os_log(mInternalOSLogObject, "IOBluetoothHostControllerUARTTransport::probe Bluetooth ROM firmware boot");
        }
        OSSafeReleaseNULL(entry);
    }
    if ( !OSDynamicCast(IOBluetoothSerialClientSerialStreamSync, provider) )
        return result;
    return NULL;
}

bool IOBluetoothHostControllerUARTTransport::start(IOService * provider)
{
    bool result;

    result = super::start(provider);
    mPowerMask = 6;
    mProvider = OSDynamicCast(IORS232SerialStreamSync, provider);
    mUARTTransportWorkLoop = IOWorkLoop::workLoop();
    if ( !mUARTTransportWorkLoop )
    {
        os_log(mInternalOSLogObject, "[IOBluetoothHostControllerUARTTransport][start] -- failed to create local workloop");
        return false;
    }
    mUARTTransportTimerWorkLoop = IOWorkLoop::workLoop();
    if ( !mUARTTransportTimerWorkLoop )
    {
        os_log(mInternalOSLogObject, "[IOBluetoothHostControllerUARTTransport][start] -- failed to create local timer workloop");
FAIL1:
        OSSafeReleaseNULL(mUARTTransportWorkLoop);
        return false;
    }
    mUARTTransportTimer = IOTimerEventSource::timerEventSource(this, TimeOutHandler);
    if ( !mUARTTransportTimer )
    {
        os_log(mInternalOSLogObject, "[IOBluetoothHostControllerUARTTransport][start] -- Cannot obtain timerEventSource");
FAIL2:
        OSSafeReleaseNULL(mUARTTransportTimerWorkLoop);
        goto FAIL1;
    }
    mUARTTransportTimerWorkLoop->addEventSource(mUARTTransportTimer);
    mUARTTransportTimerCommandGate = IOCommandGate::commandGate(this, NULL);
    if ( !mUARTTransportTimerCommandGate )
    {
        os_log(mInternalOSLogObject, "[IOBluetoothHostControllerUARTTransport][start] -- failed to obtain timer commandGate");
FAIL3:
        mUARTTransportTimerWorkLoop->removeEventSource(mUARTTransportTimer);
        OSSafeReleaseNULL(mUARTTransportTimer);
        goto FAIL2;
    }
    mUARTTransportTimerWorkLoop->addEventSource(mUARTTransportTimerCommandGate);
    mDequeueDataInterruptAction = OSMemberFunctionCast(IOInterruptEventSource::Action, this, &IOBluetoothHostControllerUARTTransport::DequeueDataInterruptEventGated);
    mDequeueDataInterrupt = IOInterruptEventSource::interruptEventSource(this, mDequeueDataInterruptAction);
    if ( !mDequeueDataInterrupt )
    {
        os_log(mInternalOSLogObject, "[IOBluetoothHostControllerUARTTransport][start] -- failed to get interrupt event source");
        mUARTTransportTimerWorkLoop->removeEventSource(mUARTTransportTimerCommandGate);
        OSSafeReleaseNULL(mUARTTransportTimerCommandGate);
        goto FAIL3;
    }
    mUARTTransportWorkLoop->addEventSource(mDequeueDataInterrupt);
    registerService();
    
    return result;
}

void IOBluetoothHostControllerUARTTransport::stop(IOService * provider)
{
    super::stop(provider);
}

bool IOBluetoothHostControllerUARTTransport::terminateWL(IOOptionBits options)
{
    bool result;

    if ( mBluetoothFamily )
        mBluetoothFamily->messageClients(-536739831);
        
    if ( mBluetoothController )
        mBluetoothController->CleanUpBeforeTransportTerminate(this);
    
    result = super::terminateWL(options);
    if ( mUARTTransportTimerHasTimeout )
    {
        mUARTTransportTimer->cancelTimeout();
        mUARTTransportTimerHasTimeout = false;
    }
    return result;
}

bool IOBluetoothHostControllerUARTTransport::ConfigurePM(IOService * policyMaker)
{
    PMinit();
    policyMaker->joinPMtree(this);
    registerPowerDriver(this, powerStateArray, kPowerStateCount);
    if ( mBluetoothFamily )
    {
        mBluetoothFamily->setProperty("TransportType", "UART");
        mConrollerTransportType = 2;
    }
    return true;
}

void IOBluetoothHostControllerUARTTransport::CompletePowerStateChange(char * name)
{
    if ( mCurrentInternalPowerState == kIOBluetoothHCIControllerInternalPowerStateOff )
        BluetoothFamilyLogPacket(mBluetoothFamily, 251, "OFF -- Complete");
    else if ( mCurrentInternalPowerState == kIOBluetoothHCIControllerInternalPowerStateOn )
        BluetoothFamilyLogPacket(mBluetoothFamily, 251, "ON -- Complete");
    else if ( mCurrentInternalPowerState == kIOBluetoothHCIControllerInternalPowerStateSleep )
    {
        if ( mHardwareInitialized )
        {
            ReleasePort();
            SetBTLP(true);
            mHardwareInitialized = false;
        }
        ReadyToGo(false);
        BluetoothFamilyLogPacket(mBluetoothFamily, 251, "SLEEP -- Complete");
    }
    acknowledgeSetPowerState();
    CancelBluetoothSleepTimer();
    mPendingInternalPowerState = mCurrentInternalPowerState;
}

IOReturn IOBluetoothHostControllerUARTTransport::powerStateWillChangeToWL(IOOptionBits options, void * refCon)
{
    IOReturn result;

    result = super::powerStateWillChangeToWL(options, refCon);
    if ( options == -536870112 && !mCurrentInternalPowerState )
    {
        ReleasePort();
        SetBTLP(false);
        SetBTRB(mSkipBluetoothFirmwareBoot ? true : *(bool *)(mBluetoothFamily + 434));
        SetBTPD();
    }
    return result;
}

IOReturn IOBluetoothHostControllerUARTTransport::RequestTransportPowerStateChange(IOBluetoothHCIControllerInternalPowerState powerState, char * name)
{
    if ( *(bool *)(this + 368) )
        return -536870212;
  
    if ( powerState == 0 )
    {
        mCurrentPMMethod = 4;
        *(UInt8 *)(this + 248) = 1;
        mBluetoothController->CleanUpForPoweringOff();
        changePowerStateToPriv(0);
    }
    else if ( powerState == 1 )
    {
        mCurrentPMMethod = 4;
        changePowerStateToPriv(1);
    }
    return kIOReturnSuccess;
}

void IOBluetoothHostControllerUARTTransport::systemWillShutdownWL(IOOptionBits options, void *)
{
    if ( options == -536870128 && mCurrentInternalPowerState )
        mBluetoothController->CallSetHIDEmulation();
}

UInt16 IOBluetoothHostControllerUARTTransport::GetControllerVendorID()
{
    return 1452;
}

UInt16 IOBluetoothHostControllerUARTTransport::GetControllerProductID()
{
    return mUARTProductID;
}

IOReturn IOBluetoothHostControllerUARTTransport::SendHCIRequest(UInt8 * buffer, IOByteCount size)
{
    mPacket->type = 1;
    memcpy(mPacket->data, buffer, size);
    return SendUART((UInt8 *) mPacket, (UInt32) size + 1);
}

IOReturn IOBluetoothHostControllerUARTTransport::TransportBulkOutWrite(void * buffer)
{
    IOReturn result;
    IOBufferMemoryDescriptor * memDescriptor;
    
    memDescriptor = (IOBufferMemoryDescriptor *) buffer;
    mPacket->type = 2;
    memDescriptor->readBytes(0, mPacket->data, memDescriptor->getLength());
    
    result = SendUART((UInt8 *) mPacket, (UInt32) memDescriptor->getLength() + 1);
    if ( !result )
        mBluetoothController->setUnackQueueCompletionCalled(buffer);
    return result;
}

IOReturn IOBluetoothHostControllerUARTTransport::TransportIsochOutWrite(void * buffer, void * arg3, IOOptionBits)
{
    IOReturn result;
    IOBufferMemoryDescriptor * memDescriptor;
    
    memDescriptor = (IOBufferMemoryDescriptor *) buffer;
    mPacket->type = 3;
    memDescriptor->readBytes(0, mPacket->data, memDescriptor->getLength());
    result = SendUART((UInt8 *) mPacket, (UInt32) memDescriptor->getLength() + 1);
    if ( !result )
    {
        *(void **)(this + 352) = arg3;
        OSAddAtomic(1, this + 364);
    }
    return result;
}

IOReturn IOBluetoothHostControllerUARTTransport::TransportSendSCOData(void * buffer)
{
    IOBufferMemoryDescriptor * memDescriptor;

    memDescriptor = (IOBufferMemoryDescriptor *) buffer;
    mPacket->type = 3;
    memDescriptor->readBytes(0, mPacket->data, memDescriptor->getLength());
    return SendUART((UInt8 *) mPacket, (UInt32) memDescriptor->getLength() + 1);
}

IOReturn IOBluetoothHostControllerUARTTransport::SendUART(UInt8 * buffer, UInt32 size)
{
    UInt32 time1;
    UInt32 time;
    IOReturn result;
    UInt32 count;

    if ( !mUARTTransportTimerHasTimeout && !mUARTTransportTimer->setTimeoutMS(5000) )
        mUARTTransportTimerHasTimeout = true;
    
    time1 = mBluetoothFamily->GetCurrentTime();
    result = mProvider->enqueueData(buffer, size, &count, true);
    time = mBluetoothFamily->GetCurrentTime() - time1;
    if ( time >= 0x4A39 )
    {
        if ( time > mLongestEnqueueDataCallMicroseconds )
        {
            mLongestEnqueueDataCallMicroseconds = time;
            mBluetoothFamily->setProperty("LongestEnqueueDataCallMicroseconds", mLongestEnqueueDataCallMicroseconds, 32);
        }
        mBluetoothFamily->setProperty("SlowEnqueueData", ++mSlowEnqueueData, 32);
        mBluetoothController->BroadcastNotification(15, kIOBluetoothHCIControllerConfigStateUninitialized, kIOBluetoothHCIControllerConfigStateUninitialized);
    }
    if ( mUARTTransportTimerHasTimeout )
    {
        mUARTTransportTimer->cancelTimeout();
        mUARTTransportTimerHasTimeout = false;
    }
    if ( result )
        os_log(mInternalOSLogObject, "IOBluetoothHostControllerUARTTransport::SendUART mProvider->enqueueData() failed error=0x%08x", result);
    return result;
}

IOReturn IOBluetoothHostControllerUARTTransport::StaticProcessACLSCOEventData(void * owner, int)
{
    return ((IOBluetoothHostControllerUARTTransport *) owner)->ProcessACLSCOEventData();
}

void IOBluetoothHostControllerUARTTransport::GetInfo(void * info)
{
    if ( !info )
        return;
    
    BluetoothTransportInfo * transportInfo = (BluetoothTransportInfo *) info;
    transportInfo->type = 4;
    transportInfo->vendorID = GetControllerVendorID();
    transportInfo->productID = GetControllerProductID();
    snprintf(transportInfo->productName, 35, "Bluetooth UART Host Controller");
    snprintf(transportInfo->vendorName, 35, "Apple Inc.");
}

IOReturn IOBluetoothHostControllerUARTTransport::DequeueDataInterruptEventGated(IOInterruptEventSource * sender, int count)
{
    ProcessACLSCOEventData();
    return -536870212;
}

IOReturn IOBluetoothHostControllerUARTTransport::SetLMPLogging()
{
    IOReturn result;

    mPacket->type = 7;
    mPacket->data[0] = -16;
    mPacket->data[1] = mLMPLoggingEnabled;
    
    result = SendUART((UInt8 *) mPacket, 3);
    if ( result )
    {
        BluetoothFamilyLogPacket(mBluetoothFamily, 251, "Failed to turn %s LMPLogging --SendUART() returned error (0x%04X)", mLMPLoggingEnabled ? "On" : "Off", result);
        return result;
    }
    setProperty("LMPLoggingEnabled", mLMPLoggingEnabled);
    mBluetoothFamily->setProperty("LMPLoggingEnabled", mLMPLoggingEnabled);
    
    mBluetoothFamily->LogPacket(10, mPacket->data, 2);
    return kIOReturnSuccess;
}

bool IOBluetoothHostControllerUARTTransport::StartLMPLogging()
{
    bool oldVal;

    oldVal = mLMPLoggingEnabled;
    mLMPLoggingEnabled = true;
    if ( !SetLMPLogging() )
        return true;
    mLMPLoggingEnabled = oldVal;
    return false;
}

bool IOBluetoothHostControllerUARTTransport::StopLMPLogging()
{
    bool oldVal;

    oldVal = mLMPLoggingEnabled;
    mLMPLoggingEnabled = false;
    if ( !SetLMPLogging() )
        return true;
    mLMPLoggingEnabled = oldVal;
    return false;
}

IOReturn IOBluetoothHostControllerUARTTransport::ToggleLMPLogging(UInt8 * buffer)
{
    IOReturn result;
    bool oldVal;

    if ( buffer )
        return kIOReturnSuccess;
    
    oldVal = mLMPLoggingEnabled;
    mLMPLoggingEnabled ^= 1;
    result = SetLMPLogging();
    if ( result )
    {
        mLMPLoggingEnabled = oldVal;
        return result;
    }
    
    return kIOReturnSuccess;
}

IOReturn IOBluetoothHostControllerUARTTransport::DoDeviceReset(UInt16)
{
    IOReturn result;

    if ( !mBluetoothController )
        return -536870212;
    
    if ( !mBluetoothController->mCanDoHardReset )
    {
        os_log(mInternalOSLogObject, "[IOBluetoothHostControllerUARTTransport][DoDeviceReset] -- mCanDoHardReset is false, did not call BTRS()");
        return -536870212;
    }

    mBluetoothController->mCanDoHardReset = false;
    ReleasePort();
    SetBTLP(false);
    SetBTRB(mSkipBluetoothFirmwareBoot ? true : *(bool *)(mBluetoothFamily + 434));
    SetBTRS();
    result = AcquirePort(false);
    if ( result )
        return result;
    
    mHardwareInitialized = true;
    IOSleep(1000);
    if ( mBluetoothController )
    {
        mBluetoothController->KillAllPendingRequests(true, true);
        if ( !mBluetoothController->mNumberOfCommandsAllowedByHardware )
            mBluetoothController->mNumberOfCommandsAllowedByHardware = 1;
        mBluetoothController->ResetHCICommandTimeOutCounter();
        os_log(mInternalOSLogObject, "IOBluetoothHostControllerUARTTransport::DoDeviceReset -- calling SetupController()");
        result = mBluetoothController->SetupController(NULL);
        StartLMPLogging();
    }
    return result;
}

IOReturn IOBluetoothHostControllerUARTTransport::WaitForReceiveToBeReady(bool sleep)
{
    IOReturn result;
    AbsoluteTime deadline;
    
    *(bool *)(this + 360) = 0;
  
    if ( !mDequeueDataInterrupt )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUARTTransport][WaitForReceiveToBeReady] -- mDequeueDataInterruptEvent is NULL ****");
        return -536870212;
    }
    mDequeueDataInterrupt->interruptOccurred(NULL, NULL, 0);
    clock_interval_to_deadline(500, 1000000, &deadline);
    
    result = -536870212;
    for (UInt8 i = 0; i < 9; ++i)
    {
            
        if ( !sleep )
        {
            result = TransportCommandSleep(this + 360, 500, (char *) "IOBluetoothHostControllerUARTTransport::WaitForReceiveToBeReady()", true);
            continue;
        }
     
        if ( *(bool *)(this + 360) ) //TransportCommandSleep might still change it in a subclass of uart transport
            return -536870212;
        
        if ( !mUARTTransportTimerCommandGate )
        {
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerUARTTransport][WaitForReceiveToBeReady] -- mUARTTransportTimerCommandGate is NULL ****");
            return -536870212;
        }
        
        result = mUARTTransportTimerCommandGate->commandSleep(&mUARTTransportTimerHasTimeout, deadline, THREAD_UNINT);
        continue;
    }
    return result;
}

void IOBluetoothHostControllerUARTTransport::NewSCOConnection()
{
    *(UInt32 *)(this + 364) = 0;
}

IOReturn IOBluetoothHostControllerUARTTransport::ReleasePort()
{
    IOReturn error;

    error = mProvider->executeEvent(5, 0);
    if ( !error )
        return mProvider->releasePort();
    os_log(mInternalOSLogObject, "IOBluetoothHostControllerUARTTransport::ReleasePort mProvider->executeEvent() failed result=0x%08x", error);
    return error;
}

IOReturn IOBluetoothHostControllerUARTTransport::AcquirePort(bool sleep)
{
    IOReturn error;
    char aBool;

    error = mProvider->acquirePort(0);
    if ( error )
    {
        os_log(mInternalOSLogObject, "IOBluetoothHostControllerUARTTransport::AcquirePort mProvider->acquirePort() failed result=0x%08x", error);
        return error;
    }
  
    error = mProvider->executeEvent(5, 1);
    if ( error )
        goto FAILED;
    
    if ( mSkipBluetoothFirmwareBoot || *(UInt8 *)(mBluetoothFamily + 434) )
    {
        error = mProvider->executeEvent(51, 230400);
        aBool = 1;
    }
    else
    {
        error = mProvider->executeEvent(51, 6000000);
        aBool = 0;
    }
    *(UInt8 *)(this + 296) = aBool;
    
    if (error
    || (error = mProvider->executeEvent(59, 16))
    || (error = mProvider->executeEvent(67, 1))
    || (error = mProvider->executeEvent(243, 2))
    || (error = mProvider->executeEvent(4, 4))
    || (error = mProvider->executeEvent(83, 36))
    || (error = mProvider->executeEvent(0x20000000, 0x20000000))
    || (error = mProvider->executeEvent(0x400000, 0x400000)))
    {
FAILED:
        os_log(mInternalOSLogObject, "IOBluetoothHostControllerUARTTransport::AcquirePort mProvider->executeEvent() failed result=0x%08x", error);
        mProvider->releasePort();
        return error;
    }
    WaitForReceiveToBeReady(sleep);
    return 0;
}

IOReturn IOBluetoothHostControllerUARTTransport::ProcessACLSCOEventData()
{
    IOReturn result;
    UInt32 eventType;
    UInt32 event;
    UInt32 data;
    UInt8 dataType;
    UInt32 count;
    ACLDataPacketHeader * aclPacket;
    SCODataPacketHeader * scoPacket;
    EventDataPacketHeader * eventPacket;

    mCommandGate->commandWakeup(this + 360);
    
    if ( mUARTTransportTimerCommandGate )
        mUARTTransportTimerCommandGate->commandWakeup(&mUARTTransportTimerHasTimeout);
    
    *(bool *)(this + 360) = 1;
    while ( 1 )
    {
        while ( 1 )
        {
            eventType = mProvider->nextEvent() & 0xFFFFFFFC;
            if ( eventType == 84 || !eventType )
                break;
            mProvider->dequeueEvent(&event, &data, false);
        }
        
        result = mProvider->dequeueData(&dataType, 1, &count, 1);
        if ( result )
            return result;
        
        switch ( dataType )
        {
            case 2:
                mProvider->dequeueData(mDataPacket, sizeof(ACLDataPacketHeader), &count, sizeof(ACLDataPacketHeader));
                aclPacket = (ACLDataPacketHeader *) mDataPacket;
                mProvider->dequeueData(mDataPacket + sizeof(ACLDataPacketHeader), aclPacket->dataSize, &count, aclPacket->dataSize);
                count = aclPacket->dataSize + sizeof(ACLDataPacketHeader);
                mBluetoothController->ProcessACLData(mDataPacket, count);
                break;
                
            case 3:
                mProvider->dequeueData(mDataPacket, sizeof(SCODataPacketHeader), &count, sizeof(SCODataPacketHeader));
                scoPacket = (SCODataPacketHeader *) mDataPacket;
                mProvider->dequeueData(mDataPacket + sizeof(SCODataPacketHeader), scoPacket->dataSize, &count, scoPacket->dataSize);
                count = scoPacket->dataSize + sizeof(SCODataPacketHeader);
                mBluetoothController->ProcessSCOData(mDataPacket, count, 0, 0, 1);
                break;
                
            case 4:
                mProvider->dequeueData(mDataPacket, sizeof(EventDataPacketHeader), &count, sizeof(EventDataPacketHeader));
                eventPacket = (EventDataPacketHeader *) mDataPacket;
                mProvider->dequeueData(mDataPacket + sizeof(EventDataPacketHeader), eventPacket->dataSize, &count, eventPacket->dataSize);
                count = eventPacket->dataSize + sizeof(EventDataPacketHeader);
                mBluetoothController->ProcessEventData(mDataPacket, count);
                break;
                
            case 7:
                mProvider->dequeueData(mDataPacket, 63, &count, 63);
                mBluetoothController->ProcessLMPData(mDataPacket, count);
                break;
                
            default:
                continue;
        }
    }
}

