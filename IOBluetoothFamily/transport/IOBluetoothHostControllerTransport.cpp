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

#include <IOKit/bluetooth/transport/IOBluetoothHostControllerTransport.h>

#define super IOService
OSDefineMetaClassAndAbstractStructors(IOBluetoothHostControllerTransport, super)

bool IOBluetoothHostControllerTransport::init(OSDictionary * dictionary)
{
    CreateOSLogObject();
    mProvider = NULL;
    mPowerMask = 0;
    mWorkLoop = NULL;
    mCommandGate = NULL;
    mBluetoothFamily = NULL;
    mLMPLoggingEnabled = false;
    mConfiguredPM = false;
    mSwitchBehavior = 0;
    mHardwareInitialized = false;
    mTerminateCounter = 0;
    mLMPLoggingAvailable = false;
    mRefCon = NULL;
    mCurrentInternalPowerState = kIOBluetoothHCIControllerInternalPowerStateOff;
    mPendingInternalPowerState = kIOBluetoothHCIControllerInternalPowerStateOff;
    mConrollerTransportType = 0;
    mBuiltIn = false;
    mLocationID = 0;
    mSupportPowerOff = true;
    mSleepType = 0;
    mIsControllerActive = false;
    *(UInt8 *) (this + 248) = 0;
    mSystemOnTheWayToSleep = false;
    *(UInt8 *) (this + 249) = 0;
    mCurrentPMMethod = 1;
    mTransportCounter = 0;
    mTransportOutstandingCalls = 0;
    *(UInt8 *) (this + 266) = true;
    mBluetoothSleepTimerStarted = false;
    mBluetoothSleepTimerEventSource = NULL;
    *(UInt8 *)(this + 296) = 0;
    mUARTProductID = 0;
    mACPIMethods = NULL;
    *(UInt8 *)(this + 312) = 0;
    GetNVRAMSettingForSwitchBehavior();
    mExpansionData = IONewZero(ExpansionData, 1);
    if ( mExpansionData )
    {
        mExpansionData->reserved = 0;
        return super::init(dictionary);
    }
    return false;
}

void IOBluetoothHostControllerTransport::free()
{
    OSSafeReleaseNULL(mACPIMethods);
    IOSafeDeleteNULL(mExpansionData, ExpansionData, 1);
  
    if ( mBluetoothSleepTimerEventSource )
    {
        if ( mWorkLoop )
            mWorkLoop->removeEventSource(mBluetoothSleepTimerEventSource);
        OSSafeReleaseNULL(mBluetoothSleepTimerEventSource);
    }
    if ( mCommandGate )
    {
        if ( mWorkLoop )
            mWorkLoop->removeEventSource(mCommandGate);
        OSSafeReleaseNULL(mCommandGate);
    }
    OSSafeReleaseNULL(mWorkLoop);
    super::free();
}

IOService * IOBluetoothHostControllerTransport::probe(IOService * provider, SInt32 * score)
{
    IOService * result;
    IORegistryEntry * dtOptions;
    OSData * data;
    
    result = super::probe(provider, score);
  
    dtOptions = IORegistryEntry::fromPath("/options", gIODTPlane);
    if ( dtOptions )
    {
        data = OSDynamicCast(OSData, dtOptions->getProperty("SkipIOBluetoothHostControllerUSBTransport"));
        
        OSSafeReleaseNULL(dtOptions);
        if ( data )
        {
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerTransport][probe] -- skip Bluetooth USB Transport ****\n");
            return NULL;
        }
    }
    return result;
}

bool IOBluetoothHostControllerTransport::start(IOService * provider)
{
    mProvider = provider;
    
    if ( super::start(provider) )
    {
        CheckACPIMethodsAvailabilities();
        return true;
    }
    return false;
}

void IOBluetoothHostControllerTransport::stop(IOService * provider)
{
    ((IOService *) IOService::getPMRootDomain())->deRegisterInterestedDriver(this);
    PMstop();
    super::stop(provider);
}

IOWorkLoop * IOBluetoothHostControllerTransport::getWorkLoop() const
{
    return mWorkLoop;
}

IOCommandGate * IOBluetoothHostControllerTransport::getCommandGate() const
{
    return mCommandGate;
}

void BluetoothSleepTimeOutOccurred(OSObject * owner, IOTimerEventSource * sender)
{
    IOBluetoothHostControllerTransport * transport;
    
    transport = OSDynamicCast(IOBluetoothHostControllerTransport, owner);
    transport->CompletePowerStateChange((char *) "IOBluetoothHostControllerTransport::BluetoothSleepTimeOutOccurred()");
}

bool IOBluetoothHostControllerTransport::setTransportWorkLoop(void * refCon, IOWorkLoop * inWorkLoop)
{
    if ( !refCon || !inWorkLoop )
        return false;
    
    mRefCon = refCon;
    mWorkLoop = inWorkLoop;
    mWorkLoop->retain();
    
    mCommandGate = IOCommandGate::commandGate(this);;
    if ( !mCommandGate )
    {
FAIL_RELEASE_WORKLOOP:
        OSSafeReleaseNULL(mWorkLoop);
        return false;
    }
    
    if ( mWorkLoop->addEventSource(mCommandGate) )
    {
FAIL_RELEASE_CMDGATE:
        OSSafeReleaseNULL(mCommandGate);
        goto FAIL_RELEASE_WORKLOOP;
    }
    
    mBluetoothSleepTimerEventSource = IOTimerEventSource::timerEventSource(this, BluetoothSleepTimeOutOccurred);
    if ( !mBluetoothSleepTimerEventSource )
    {
FAIL_REMOVE_CMDGATE:
        mWorkLoop->removeEventSource(mCommandGate);
        goto FAIL_RELEASE_CMDGATE;
    }
    
    if ( mWorkLoop->addEventSource(mBluetoothSleepTimerEventSource) )
    {
FAIL_RELEASE_TIMER:
        OSSafeReleaseNULL(mBluetoothSleepTimerEventSource);
        goto FAIL_REMOVE_CMDGATE;
    }
    
    if ( CallConfigPM() )
    {
        mWorkLoop->removeEventSource(mBluetoothSleepTimerEventSource);
        goto FAIL_RELEASE_TIMER;
    }
    return true;
}

bool IOBluetoothHostControllerTransport::terminate(IOOptionBits options)
{
    IOReturn result;
    
    if ( TerminateCalled() )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerTransport][terminate] is being called more than once -- this = 0x%04X ****\n", ConvertAddressToUInt32(this));
        return super::terminate(options);
    }
    
    mTerminateState = 1;
    
    OSAddAtomic(1, &mTerminateCounter);
    if ( mBluetoothFamily )
        mBluetoothFamily->SetBluetoothTransportTerminateState(this, 1);
    
    if ( mBluetoothController )
        mBluetoothController->TransportIsGoingAway();
    
    if ( mCommandGate )
        mCommandGate->runAction(IOBluetoothHostControllerTransport::terminateAction, &options);
    
    result = super::terminate(options);
    if ( mBluetoothFamily )
        mBluetoothFamily->SetBluetoothTransportTerminateState(this, 2);
    return result;
}

IOReturn IOBluetoothHostControllerTransport::terminateAction(OSObject * owner, void * arg1, void * arg2, void * arg3, void * arg4)
{
    IOBluetoothHostControllerTransport * transport;
    
    if ( owner )
    {
        transport = OSDynamicCast(IOBluetoothHostControllerTransport, owner);
        if ( transport )
            return transport->terminateWL(*(IOOptionBits *) arg1);
    }
    return -536870206;
}

bool IOBluetoothHostControllerTransport::terminateWL(IOOptionBits options)
{
    mTerminateState = 2;
  
    if ( mCommandGate )
        mCommandGate->commandWakeup(&mTerminateState);
    
    if ( mBluetoothSleepTimerEventSource )
    {
        if ( mBluetoothSleepTimerStarted )
        {
            mBluetoothSleepTimerEventSource->cancelTimeout();
            mBluetoothSleepTimerStarted = false;
        }
        
        if ( mWorkLoop )
            mWorkLoop->removeEventSource(mBluetoothSleepTimerEventSource);
        OSSafeReleaseNULL(mBluetoothSleepTimerEventSource);
    }
    return true;
}

bool IOBluetoothHostControllerTransport::InitializeTransport()
{
    if ( mCommandGate )
        return !mCommandGate->runAction(IOBluetoothHostControllerTransport::InitializeTransportAction);
    return false;
}

IOReturn IOBluetoothHostControllerTransport::InitializeTransportAction(OSObject * owner, void * arg1, void * arg2, void * arg3, void * arg4)
{
    IOBluetoothHostControllerTransport * transport;

    transport = OSDynamicCast(IOBluetoothHostControllerTransport, owner);
    if ( transport && !transport->isInactive() && transport->InitializeTransportWL(transport->mProvider) )
        return kIOReturnSuccess;
    return -536870212;
}

bool IOBluetoothHostControllerTransport::InitializeTransportWL(IOService * provider)
{
    OSBoolean * familyLMPEnabled;
    OSBoolean * lmpEnabled;

    if ( getProperty("LMPLoggingEnabled") )
    {
        familyLMPEnabled = OSDynamicCast(OSBoolean, mBluetoothFamily->getProperty("LMPLoggingEnabled"));
        lmpEnabled = OSDynamicCast(OSBoolean, getProperty("LMPLoggingEnabled"));
        
        mLMPLoggingAvailable = true;
        setProperty("LMPLoggingAvailable", true);
        if ( lmpEnabled && (familyLMPEnabled == NULL || (familyLMPEnabled && familyLMPEnabled->getValue())) )
        {
            mLMPLoggingEnabled = lmpEnabled->getValue();
            goto OVER;
        }
    }
    else
    {
        mLMPLoggingAvailable = false;
        setProperty("LMPLoggingAvailable", false);
    }
    mLMPLoggingEnabled = false;
    setProperty("LMPLoggingEnabled", false);
OVER:
    mBluetoothFamily->setProperty("LMPLoggingEnabled", mLMPLoggingEnabled);
    if ( mLMPLoggingEnabled )
        StartLMPLogging();
    return true;
}

OSObject * IOBluetoothHostControllerTransport::getPropertyFromTransport(const OSSymbol * aKey)
{
    return getProperty(aKey);
}

OSObject * IOBluetoothHostControllerTransport::getPropertyFromTransport(const OSString * aKey)
{
    return getProperty(aKey);
}

OSObject * IOBluetoothHostControllerTransport::getPropertyFromTransport(const char * aKey)
{
    return getProperty(aKey);
}

IOReturn IOBluetoothHostControllerTransport::SetRemoteWakeUp(bool enable)
{
    return kIOReturnSuccess;
}

IOReturn IOBluetoothHostControllerTransport::DoDeviceReset(UInt16)
{
    return kIOReturnSuccess;
}

void IOBluetoothHostControllerTransport::AbortPipesAndClose(bool, bool)
{
}

bool IOBluetoothHostControllerTransport::HostSupportsSleepOnUSB()
{
    return true;
}

bool IOBluetoothHostControllerTransport::StartLMPLogging()
{
    return true;
}

bool IOBluetoothHostControllerTransport::StartLMPLoggingBulkPipeRead()
{
    return true;
}

bool IOBluetoothHostControllerTransport::StartInterruptPipeRead()
{
    return true;
}

bool IOBluetoothHostControllerTransport::StopInterruptPipeRead()
{
    return true;
}

bool IOBluetoothHostControllerTransport::StartBulkPipeRead()
{
    return true;
}

bool IOBluetoothHostControllerTransport::StopBulkPipeRead()
{
    return true;
}

IOReturn IOBluetoothHostControllerTransport::TransportBulkOutWrite(void *)
{
    return kIOReturnSuccess;
}

IOReturn IOBluetoothHostControllerTransport::TransportIsochOutWrite(void * memDescriptor, void *, IOOptionBits)
{
    return kIOReturnSuccess;
}

IOReturn IOBluetoothHostControllerTransport::TransportSendSCOData(void *)
{
    return kIOReturnSuccess;
}

IOReturn IOBluetoothHostControllerTransport::TransportLMPLoggingBulkOutWrite(UInt8, UInt8)
{
    return kIOReturnSuccess;
}

IOReturn IOBluetoothHostControllerTransport::SendHCIRequest(UInt8 * buffer, IOByteCount size)
{
    return kIOReturnSuccess;
}

void IOBluetoothHostControllerTransport::UpdateSCOConnections(UInt8, UInt32)
{
}

IOReturn IOBluetoothHostControllerTransport::ToggleLMPLogging(UInt8 *)
{
    return kIOReturnSuccess;
}

IOReturn IOBluetoothHostControllerTransport::CallConfigPM()
{
    if ( mACPIMethods )
        mACPIMethods->SetBluetoothFamily(mBluetoothFamily);
    else
    {
        for (UInt8 i = 0; i < 5; ++i)
        {
            if ( (mPowerMask & 0xFFFE) != 6 || mACPIMethods )
                break;
            CheckACPIMethodsAvailabilities();
            
            if ( mACPIMethods )
                mACPIMethods->SetBluetoothFamily(mBluetoothFamily);
            else
                IOSleep(2);
        }
    }
    
    if ( ConfigurePM(mProvider) )
    {
        SupportNewIdlePolicy();
        if ( mBluetoothController )
            mBluetoothController->SetIdlePolicy(mSupportNewIdlePolicy);
        ((IOService *) IOService::getPMRootDomain())->registerInterestedDriver(this);
        return kIOReturnSuccess;
    }
    return -536870212; 
}

bool IOBluetoothHostControllerTransport::ConfigurePM(IOService * policyMaker)
{
    return true;
}

unsigned long IOBluetoothHostControllerTransport::maxCapabilityForDomainState(IOPMPowerFlags domainState)
{
    return super::maxCapabilityForDomainState(domainState);
}

unsigned long IOBluetoothHostControllerTransport::initialPowerStateForDomainState(IOPMPowerFlags domainState)
{
    return super::initialPowerStateForDomainState(domainState);
}

IOReturn IOBluetoothHostControllerTransport::setPowerState(unsigned long powerStateOrdinal, IOService * whatDevice)
{
    // The setPowerState function group returns booleans, but due to inheritancy compatability issues they seem to be IOReturns
    IOService::getPMRootDomain();
    if ( isInactive() || !mCommandGate )
        return false;
    
    if ( mCurrentPMMethod )
    {
        mCurrentPMMethod = 0;
        return mCommandGate->runAction(IOBluetoothHostControllerTransport::setPowerStateAction, &powerStateOrdinal, &whatDevice);
    }
    if ( *(UInt8 *)(mBluetoothFamily + 432) )
    {
        mCurrentPMMethod = 0;
        powerStateOrdinal = 2;
        return mCommandGate->runAction(IOBluetoothHostControllerTransport::setPowerStateAction, &powerStateOrdinal, &whatDevice);
    }
    if ( (mPowerMask & 0xFFFE) != 6 )
    {
        if ( powerStateOrdinal == 1 )
        {
            BluetoothFamilyLogPacket(mBluetoothFamily, 251, "Low Power done");
            return false;
        }
        else if ( powerStateOrdinal == kIOBluetoothHCIControllerInternalPowerStateOff )
        {
            mCurrentInternalPowerState = kIOBluetoothHCIControllerInternalPowerStateOff;
            if ( mIsControllerActive )
                mBluetoothController->UpdatePowerStateProperty(kIOBluetoothHCIControllerInternalPowerStateOff, true);
            WakeupSleepingPowerStateThread();
        }
    }
    BluetoothFamilyLogPacket(mBluetoothFamily, 251, "%s done", gOrdinalPowerStateString[powerStateOrdinal]);
    return false;
}

IOReturn IOBluetoothHostControllerTransport::setPowerStateAction(OSObject * owner, void * arg1, void * arg2, void * arg3, void * arg4)
{
    IOBluetoothHostControllerTransport * transport;
    IOReturn result;
    
    if ( !owner )
        return false;
    
    transport = OSDynamicCast(IOBluetoothHostControllerTransport, owner);
    if ( !transport || transport->isInactive() )
        return false;
    
    transport->RetainTransport((char *) "IOBluetoothHostControllerTransport::setPowerStateAction()");
    result = transport->setPowerStateWL(*(unsigned long *) arg1, *(IOService **) arg2);
    if ( result )
        transport->StartBluetoothSleepTimer();
    transport->ReleaseTransport((char *) "IOBluetoothHostControllerTransport::setPowerStateAction()");
    
    return result;
}

IOReturn IOBluetoothHostControllerTransport::setPowerStateWL(unsigned long powerStateOrdinal, IOService * whatDevice)
{
    return false;
}

IOReturn IOBluetoothHostControllerTransport::RequestTransportPowerStateChange(IOBluetoothHCIControllerInternalPowerState powerState, char * name)
{
    return kIOReturnSuccess;
}

IOReturn IOBluetoothHostControllerTransport::WaitForControllerPowerState(IOBluetoothHCIControllerInternalPowerState powerState, char * name)
{
    int counter;

    counter = 0;
    while ( !isInactive() )
    {
        if ( mCurrentInternalPowerState == powerState || mTerminateState )
            return kIOReturnSuccess;
        
        // the command sleep loop should only run once
        if ( counter )
            continue;
        while ( 1 )
        {
            ++counter;
            if ( !TransportCommandSleep(&mCurrentInternalPowerState, 500, name, false) )
                break;
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerTransport][WaitForControllerPowerState] -- commandSleep(&mCurrentInternalPowerState) failed -- counter = %d ****\n\n", counter);
        }
    }
    return -536870208;
}

IOReturn IOBluetoothHostControllerTransport::WaitForControllerPowerStateWithTimeout(IOBluetoothHCIControllerInternalPowerState newPowerState, uint32_t timeout, char * name, bool didTimeout)
{
    UInt32 ms;
    IOReturn result;
    char * errStringShort;
    char * errStringLong;
    
    mBluetoothFamily->GetCurrentTime();
    ms = timeout / 0x3E8 + 1000;
    
    if ( isInactive() )
        result = -536870911;
    else if ( mCurrentInternalPowerState == newPowerState )
        result = 0;
    else
        result = TransportCommandSleep(&mCurrentInternalPowerState, ms, name, true);
    
    if ( result )
    {
        if ( didTimeout || result == THREAD_TIMED_OUT )
        {
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerTransport][WaitForControllerPowerStateWithTimeout] -- TransportCommandSleep(&mCurrentInternalPowerState, %ld milliseconds) timeout occurred! -- Called by function %s waiting for power state to change to %s -- mCurrentInternalPowerState = %s -- this = 0x%04x ****\n", (unsigned long) ms, name, gInternalPowerStateString[newPowerState], gInternalPowerStateString[mCurrentInternalPowerState], ConvertAddressToUInt32(this));
            BluetoothFamilyLogPacket(mBluetoothFamily, 250, "%s -> %s timed out", gInternalPowerStateString[mCurrentInternalPowerState], gInternalPowerStateString[newPowerState]);
        }
        else
        {
            errStringLong = (char *) IOMalloc(0x64);
            errStringShort = (char *) IOMalloc(0x32);
            mBluetoothFamily->ConvertErrorCodeToString(result, errStringLong, errStringShort); //It is needed to allocate the memory outside the function
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerTransport][WaitForControllerPowerStateWithTimeout] -- TransportCommandSleep(&mCurrentInternalPowerState, %ld milliseconds) returned error 0x%04X (%s) -- Called by function %s waiting for power state to change to %s -- mCurrentInternalPowerState = %s -- this = 0x%04x ****\n", (unsigned long) ms, result, errStringLong, name, gInternalPowerStateString[newPowerState], gInternalPowerStateString[mCurrentInternalPowerState], ConvertAddressToUInt32(this));
            BluetoothFamilyLogPacket(mBluetoothFamily, 250, "%s -> %s unknown error 0x%04X", gInternalPowerStateString[mCurrentInternalPowerState], gInternalPowerStateString[newPowerState], result);
            IOFree(errStringLong, 0x64);
            IOFree(errStringShort, 0x32);
        }
        mCurrentPMMethod = 0;
    }
    
    if ( isInactive() )
        return -536870208;
    if ( mCurrentInternalPowerState == newPowerState )
        return kIOReturnSuccess;
    return -536870212;
}

void IOBluetoothHostControllerTransport::CompletePowerStateChange(char * name)
{
}

IOReturn IOBluetoothHostControllerTransport::ProcessPowerStateChangeAfterResumed(char * name)
{
    return kIOReturnSuccess;
}

IOReturn IOBluetoothHostControllerTransport::setAggressiveness(unsigned long type, unsigned long newLevel)
{
    if ( type == 6 )
        mCommandGate->runAction(IOBluetoothHostControllerTransport::setAggressivenessAction, &type, &newLevel);
    
    return super::setAggressiveness(type, newLevel);
}

IOReturn IOBluetoothHostControllerTransport::setAggressivenessAction(OSObject * owner, void * arg1, void * arg2, void * arg3, void * arg4)
{
    IOReturn result;
    IOBluetoothHostControllerTransport * transport;
    
    result = -536870212;
    if ( owner )
    {
        transport = OSDynamicCast(IOBluetoothHostControllerTransport, owner);
        if ( transport && !transport->isInactive() )
        {
            transport->RetainTransport((char *) "IOBluetoothHostControllerTransport::setPowerStateAction()");
            result = transport->setAggressivenessWL(*(unsigned long *) arg1, *(unsigned long*) arg2);
            transport->ReleaseTransport((char *) "IOBluetoothHostControllerTransport::setPowerStateAction()");
        }
    }
    return result;
}

bool IOBluetoothHostControllerTransport::setAggressivenessWL(unsigned long type, unsigned long newLevel)
{
    if ( type != 6 || !mBluetoothController )
        return false;
    
    if ( newLevel == 1 )
    {
        mBluetoothController->mControllerPowerOptions |= 0x10;
        mBluetoothController->CallResetTimerForIdleTimer();
    }
    else if ( newLevel == 2 )
        mBluetoothController->mControllerPowerOptions &= 0xEF;
    
    if ( !isInactive() && !mTerminateState && mCommandGate )
        return mBluetoothController->SetTransportIdlePolicyValue();
    
    return false;
}

IOReturn IOBluetoothHostControllerTransport::powerStateWillChangeTo(IOPMPowerFlags capabilities, unsigned long stateNumber, IOService * whatDevice) //variables
{
    IOReturn result;
    OSNumber * sleepType;
    
    result = kIOReturnSuccess;
    if ( (IOService *) IOService::getPMRootDomain() != whatDevice || isInactive() )
        return result;
    
    sleepType = OSDynamicCast(OSNumber, whatDevice->getProperty("IOPMSystemSleepType"));
    if ( sleepType != NULL )
    {
        mSleepType = sleepType->unsigned32BitValue();
        if ( mSleepType > 7 )
        {
            os_log(mInternalOSLogObject, "\n[IOBluetoothHostControllerUSBTransport][powerStateWillChangeTo] -- Error!! IOPMSystemSleepType has wrong value (0x%x) -- treating it as Normal Sleep -- Sleep/Wake may not work properly!!\n", mSleepType);
            BluetoothFamilyLogPacket(mBluetoothFamily, 250, "SleepType is wrong, set to NormalSleep");
            mSleepType = 2;
        }
        BluetoothFamilyLogPacket(mBluetoothFamily, 248, "%s", gPowerManagerSleepTypeString[mSleepType]);
        if ( mBluetoothController )
            mBluetoothController->mTransportSleepType = mSleepType;
    }
    else
        BluetoothFamilyLogPacket(mBluetoothFamily, 248, "Unknown Sleep Type");
    
    RetainTransport((char *) "IOBluetoothHostControllerTransport::powerStateWillChangeTo()");
    if ( capabilities & 4 )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerTransport][powerStateWillChangeTo] -- System Sleep -- SleepType is %s ****\n", gPowerManagerSleepTypeString[mSleepType]);
        BluetoothFamilyLogPacket(mBluetoothFamily, 251, "System Sleep");
        *(UInt8 *)(mBluetoothController + 897) = 0;
        if ( *(UInt8 *)(mBluetoothFamily + 454) )
        {
            *(UInt8 *)(mBluetoothFamily + 455) = 1;
            *(UInt8 *)(mBluetoothController + 889) = *(UInt8 *)(mBluetoothController + 1307);
        }
        mSystemOnTheWayToSleep = 1;
        *(UInt8 *)(mBluetoothController + 967) = 0;
        mBluetoothController->PerformTaskForPowerManagementCalls(-536870272);
        if ( *(UInt32 *)(mBluetoothController + 1276LL) )
            result = mCommandGate->runAction(IOBluetoothHostControllerTransport::powerStateWillChangeToAction, (void *) -536870272);
        else
            mCurrentPMMethod = 2;
    }
    else
    {
        if ( capabilities & 2 )
        {
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerTransport][powerStateWillChangeTo] -- System Wake -- SleepType is %s ****\n", gPowerManagerSleepTypeString[mSleepType]);
            BluetoothFamilyLogPacket(mBluetoothFamily, 251, "System Power On");
            if ( *(UInt8 *)(mBluetoothFamily + 454) )
            {
                *(UInt8 *)(mBluetoothFamily + 455) = 0;
                *(UInt8 *)(mBluetoothController + 889) = 0;
            }
            if ( *(UInt8 *)(mBluetoothFamily + 457) )
                *(UInt8 *)(mBluetoothFamily + 456) = 1;
            *(UInt8 *)(mBluetoothController + 897LL) = 1;
            *(UInt32 *)(mBluetoothFamily + 404) = mBluetoothFamily->GetCurrentTime();
            *(UInt8 *)(mBluetoothController + 967) = 1;
            *(UInt8 *)(mBluetoothController + 1297) = 0;
            mBluetoothController->PerformTaskForPowerManagementCalls(-536870112);
            if ( (mConrollerTransportType & 0xFFFE) == 2 )
                mCommandGate->runAction(IOBluetoothHostControllerTransport::powerStateWillChangeToAction, (void *) -536870112);
        }
    }
    ReleaseTransport((char *) "IOBluetoothHostControllerTransport::powerStateWillChangeTo()");
    return result;
}

IOReturn IOBluetoothHostControllerTransport::powerStateWillChangeToAction(OSObject * owner, void * arg1, void * arg2, void * arg3, void * arg4)
{
    IOBluetoothHostControllerTransport * transport;
    
    transport = OSDynamicCast(IOBluetoothHostControllerTransport, owner);
    if ( !transport || transport->isInactive() )
        return 0;
  
    transport->mCurrentPMMethod = 2;
    return transport->powerStateWillChangeToWL(*(IOOptionBits *) arg1, arg2);
}

IOReturn IOBluetoothHostControllerTransport::powerStateWillChangeToWL(IOOptionBits options, void *)
{
    //"IOBluetoothHostControllerTransport::powerStateWillChangeToWL()"
    //This is in the string list, but it seems to be unused. Put it here for now.
    
    if ( options != -536870112 )
    {
        if ( options == -536870272 )
        {
            BluetoothFamilyLogPacket(mBluetoothFamily, 248, "System Sleep");
            mBluetoothController->CallCancelTimeoutForIdleTimer();
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerTransport][powerStateWillChangeToWL] -- SleepType is %s -- this = 0x%04x ****\n", gPowerManagerSleepTypeString[mSleepType], ConvertAddressToUInt32(this));
            BluetoothFamilyLogPacket(mBluetoothFamily, 248, "%s", gPowerManagerSleepTypeShortString[mSleepType]);
            mSystemOnTheWayToSleep = true;
        }
        return 0;
    }
    
    BluetoothFamilyLogPacket(mBluetoothFamily, 248, "System PowerOn");
    mSystemOnTheWayToSleep = false;
    *(UInt8 *)(mBluetoothController + 967) = 1;
    
    if ( (mPowerMask & 0xFFFE) != 6 )
    {
        if ( ((mSleepType == 5 && mBluetoothFamily->isSystemPortable()) || mSleepType == 4 || mSleepType == 6) && mBluetoothController )
            mBluetoothController->TransportTerminating(this);
    }
    
    if ( !mBluetoothController->mNumberOfCommandsAllowedByHardware )
        mBluetoothController->mNumberOfCommandsAllowedByHardware = 1;
    return 0;
}

void IOBluetoothHostControllerTransport::systemWillShutdown(IOOptionBits specifier)
{
    if ( specifier == -536870128 )
        mSystemOnTheWayToSleep = 1;
    else
    {
        if ( specifier != -536870320 )
            goto OVER;
        mSystemOnTheWayToSleep = 1;
    }
    mCommandGate->runAction(IOBluetoothHostControllerTransport::systemWillShutdownAction, reinterpret_cast<void *>(specifier));
    
OVER:
    super::systemWillShutdown(specifier);
}

IOReturn IOBluetoothHostControllerTransport::systemWillShutdownAction(OSObject * owner, void * arg1, void * arg2, void * arg3, void * arg4)
{
    //In this function, the return value doesn't matter as the other two functions of this group are void...
    IOBluetoothHostControllerTransport * transport;
    
    transport = OSDynamicCast(IOBluetoothHostControllerTransport, owner);
    if ( !transport || transport->isInactive() )
        return 0;
    
    *(UInt32 *)(transport->mBluetoothFamily + 396) = -536870128;
    transport->mBluetoothController->CallKillAllPendingRequests(false, false);
    transport->mCurrentInternalPowerState = kIOBluetoothHCIControllerInternalPowerStateOff;
    transport->mCurrentPMMethod = 5;
    transport->systemWillShutdownWL(*(IOOptionBits *) arg1, arg2);
    return 0;
}

void IOBluetoothHostControllerTransport::systemWillShutdownWL(IOOptionBits options, void *)
{
}

IOBluetoothHCIControllerInternalPowerState IOBluetoothHostControllerTransport::GetCurrentPowerState()
{
    return mCurrentInternalPowerState;
}

IOBluetoothHCIControllerInternalPowerState IOBluetoothHostControllerTransport::GetPendingPowerState()
{
    return mPendingInternalPowerState;
}

IOReturn IOBluetoothHostControllerTransport::ChangeTransportPowerStateFromIdleToOn(char * name)
{
    IOReturn result;
    
    result = -536870212;
    if (isInactive())
        return result;

    RetainTransport((char *) "IOBluetoothHostControllerTransport::ChangeTransportPowerStateFromIdleToOn()");
    if ( mCurrentInternalPowerState != kIOBluetoothHCIControllerInternalPowerStateIdle )
    {
        result = kIOReturnSuccess;
        goto OVER;
    }
    if ( mPendingInternalPowerState == kIOBluetoothHCIControllerInternalPowerStateIdle )
        CallPowerManagerChangePowerStateTo(2, name);
    if ( !WaitForControllerPowerStateChange(kIOBluetoothHCIControllerInternalPowerStateOn, name) )
        result = kIOReturnSuccess;
    
OVER:
    ReleaseTransport((char *) "IOBluetoothHostControllerTransport::ChangeTransportPowerStateFromIdleToOn()");
    return result;
}

IOReturn IOBluetoothHostControllerTransport::ChangeTransportPowerState(unsigned long ordinal, bool willWait, IOBluetoothHCIControllerInternalPowerState powerState, char * name)
{
    IOReturn result;
  
    if ( mSystemOnTheWayToSleep )
        return -536870212;
  
    result = CallPowerManagerChangePowerStateTo(ordinal, name);
    if ( result && !willWait )
        return result;
    
    mBluetoothFamily->GetCurrentTime();
    result = WaitForControllerPowerStateWithTimeout(powerState, 5000000, name, false);
    if ( result )
        mBluetoothController->SetChangingPowerState(false);
    
    return result;
}

IOReturn IOBluetoothHostControllerTransport::WaitForControllerPowerStateChange(IOBluetoothHCIControllerInternalPowerState powerState, char * name)
{
    mBluetoothFamily->GetCurrentTime();
    return WaitForControllerPowerStateWithTimeout(powerState, 5000000, name, false);
}

IOReturn IOBluetoothHostControllerTransport::WakeupSleepingPowerStateThread()
{
    if ( mCommandGate )
    {
        mCommandGate->commandWakeup(&mCurrentInternalPowerState, false);
        return kIOReturnSuccess;
    }
    return -536870212;
}

bool IOBluetoothHostControllerTransport::ControllerSupportWoBT()
{
    if ( (mConrollerTransportType & 0xFFFE) != 2 )
        return false;
    mSupportWoBT = true;
    return true;
}

UInt16 IOBluetoothHostControllerTransport::GetControllerVendorID()
{
    return 0;
}

UInt16 IOBluetoothHostControllerTransport::GetControllerProductID()
{
    return 0;
}

BluetoothHCIPowerState IOBluetoothHostControllerTransport::GetRadioPowerState()
{
    return kBluetoothHCIPowerStateOFF;
}

void IOBluetoothHostControllerTransport::SetRadioPowerState(BluetoothHCIPowerState powerState)
{
}

bool IOBluetoothHostControllerTransport::GetNVRAMSettingForSwitchBehavior()
{
    IORegistryEntry * entry;
    OSData * data;
  
    entry = IORegistryEntry::fromPath("/options", gIODTPlane);
    if ( !entry )
        return false;
    
    data = OSDynamicCast(OSData, entry->getProperty("bluetoothHostControllerSwitchBehavior"));
    if ( !data )
    {
        OSSafeReleaseNULL(entry);
        return false;
    }
        
    if ( !strncmp("always", (const char *) data->getBytesNoCopy(), data->getLength()) )
        mSwitchBehavior = 1;
    if ( !strncmp("never", (const char *) data->getBytesNoCopy(), data->getLength()) )
        mSwitchBehavior = 2;
    
    OSSafeReleaseNULL(entry);
    return true;
}

UInt32 IOBluetoothHostControllerTransport::GetControllerLocationID()
{
    return mLocationID;
}

bool IOBluetoothHostControllerTransport::GetBuiltIn()
{
    if ( (mConrollerTransportType & 0xFFFE) != 2 )
        return mBuiltIn;
    mBuiltIn = true;
    return true;
}

bool IOBluetoothHostControllerTransport::GetSupportPowerOff()
{
    return mSupportPowerOff;
}

bool IOBluetoothHostControllerTransport::IsHardwareInitialized()
{
    return mHardwareInitialized;
}

UInt32 IOBluetoothHostControllerTransport::GetHardwareStatus()
{
    return mHardwareStatus;
}

void IOBluetoothHostControllerTransport::ResetHardwareStatus()
{
    mHardwareStatus = 0;
}

UInt32 IOBluetoothHostControllerTransport::ConvertAddressToUInt32(void * address)
{
    if ( !address )
        return 0;
    return *(UInt32 *) address;
}

void IOBluetoothHostControllerTransport::SetActiveController(bool isActive)
{
    mIsControllerActive = isActive;
}

void IOBluetoothHostControllerTransport::ResetBluetoothDevice()
{
}

IOReturn IOBluetoothHostControllerTransport::TransportCommandSleep(void * event, uint32_t interval, char * name, bool logPanic)
{
    IOReturn result;
    AbsoluteTime deadline;
    char * errStringShort;
    char * errStringLong;
    
    if ( !event || !interval )
        return -536870206;
    if ( !mCommandGate )
        return -536870212;
    
    clock_interval_to_deadline(interval, 1000000, &deadline);
    OSAddAtomic16(1, &mTransportOutstandingCalls);
    result = mCommandGate->commandSleep(event, deadline, THREAD_UNINT);
    OSAddAtomic16(0xFFFFFFFF, &mTransportOutstandingCalls);
    if ( result == -536870174 )
    {
        if ( !logPanic )
            return result;
        panic("\"IOBluetoothHostControllerTransport::TransportCommandSleep() -- commandSleep not called in the workloop for function %s\\n\"@/System/Volumes/Data/SWE/macOS/BuildRoots/2288acc43c/Library/Caches/com.apple.xbs/Sources/IOBluetoothFamily_kexts/IOBluetoothFamily-8004.1.18.3/Core/Family/HCI/Transports/IOBluetoothHostControllerTransport/IOBluetoothHostControllerTransport.cpp:2602", name);
        return -536870212;
    }
    if ( result >= 2 && mBluetoothFamily )
    {
        errStringLong = (char *) IOMalloc(0x64);
        errStringShort = (char *) IOMalloc(0x32);
        mBluetoothFamily->ConvertErrorCodeToString(result, errStringLong, errStringShort);
        IOFree(errStringLong, 0x64);
        IOFree(errStringLong, 0x32);
    }
    return result;
}

void IOBluetoothHostControllerTransport::ReadyToGo(bool oneThread)
{
    if ( mBluetoothController )
        mBluetoothController->TransportIsReady(oneThread);
}

bool IOBluetoothHostControllerTransport::TerminateCalled()
{
    return OSCompareAndSwap(0, 0, &mTerminateCounter) ^ 1;
}

void IOBluetoothHostControllerTransport::GetInfo(void * info)
{
    if ( !info )
        return;
    
    BluetoothTransportInfo * transportInfo = (BluetoothTransportInfo *) info;
    transportInfo->productID = 0;
    transportInfo->vendorID = 0;
    transportInfo->type = 0;
    snprintf(transportInfo->productName, 35, "Unknown");
    snprintf(transportInfo->vendorName, 35, "Unknown");
}

IOReturn IOBluetoothHostControllerTransport::CallPowerManagerChangePowerStateTo(unsigned long ordinal, char * name)
{
    if ( mBluetoothController )
    {
        mBluetoothController->SetChangingPowerState(true);
        if ( mSupportNewIdlePolicy )
            mBluetoothController->ChangeIdleTimerTime((char *) "IOBluetoothHostControllerUSBTransport::CallPowerManagerChangePowerStateTo()", *(UInt32 *)(mBluetoothController + 1264));
    }
    mCurrentPMMethod = 4;
    if ( (mConrollerTransportType & 0xFFFE) != 2 )
        return changePowerStateTo(ordinal);
    
    // Here, the greatest ordinal allowed is 1.
    if ( ordinal )
    {
        if ( ordinal != 2 )
            return 0;
        ordinal = 1;
    }
    return changePowerStateToPriv(ordinal);
}

UInt16 IOBluetoothHostControllerTransport::GetControllerTransportType()
{
    return mConrollerTransportType;
}

bool IOBluetoothHostControllerTransport::SupportNewIdlePolicy()
{
    mSupportNewIdlePolicy = false;
    
    if ( GetControllerVendorID() == 1452 && ((GetControllerTransportType() == 1 && GetControllerProductID() > 0x821C) || (GetControllerTransportType() == 2) || (GetControllerTransportType() == 3)) )
    {
        mSupportNewIdlePolicy = true;
        return setProperty("SupportNewIdlePolicy", true);
    }
    return setProperty("SupportNewIdlePolicy", false);
}

void IOBluetoothHostControllerTransport::CheckACPIMethodsAvailabilities()
{
    mACPIMethods = new IOBluetoothACPIMethods;
    if ( !mACPIMethods )
        return;
    
    if ( mACPIMethods->init(this) && mACPIMethods->attach(this) )
    {
        if ( mACPIMethods->start(this) )
        {
            mACPIMethods->CheckSpecialGPIO();
            return;
        }
        mACPIMethods->detach(this);
    }
    OSSafeReleaseNULL(mACPIMethods);
}

IOReturn IOBluetoothHostControllerTransport::SetBTRS()
{
    if ( mACPIMethods )
        return mACPIMethods->SetBTRS();
    return -536870212;
}

IOReturn IOBluetoothHostControllerTransport::SetBTPU()
{
    if ( mACPIMethods )
        return mACPIMethods->SetBTPU();
    return -536870212;
}

IOReturn IOBluetoothHostControllerTransport::SetBTPD()
{
    if ( mACPIMethods )
        return mACPIMethods->SetBTPD();
    return -536870212;
}

IOReturn IOBluetoothHostControllerTransport::SetBTRB(bool arg)
{
    if ( mACPIMethods )
        return mACPIMethods->SetBTRB(arg, NULL);
    return -536870212;
}

IOReturn IOBluetoothHostControllerTransport::SetBTLP(bool arg)
{
    if ( mACPIMethods )
        return mACPIMethods->SetBTLP(arg);
    return -536870212;
}

void IOBluetoothHostControllerTransport::NewSCOConnection()
{
}

void IOBluetoothHostControllerTransport::retain() const
{
    super::retain();
}

void IOBluetoothHostControllerTransport::release() const
{
    super::release();
}

void IOBluetoothHostControllerTransport::RetainTransport(char * name)
{
    retain();
    ++mTransportCounter;
}

void IOBluetoothHostControllerTransport::ReleaseTransport(char * name)
{
    --mTransportCounter;
    release();
}

IOReturn IOBluetoothHostControllerTransport::SetIdlePolicyValue(uint32_t idleTimeoutMs)
{
    return kIOReturnSuccess;
}

bool IOBluetoothHostControllerTransport::TransportWillReEnumerate()
{
    return false;
}

void IOBluetoothHostControllerTransport::ConvertPowerFlagsToString(IOPMPowerFlags flags, char * outString)
{
#define GENSTR(num, condition, content)         \
    char * str ## num = (char *) "";            \
    if ( (condition) )                          \
        str ## num = (char *) content;          \

    GENSTR(1,  flags & kIOPMPowerOn,                "kIOPMPowerOn")
    GENSTR(2,  flags & kIOPMDeviceUsable,           "kIOPMDeviceUsable")
    GENSTR(3,  flags & kIOPMLowPower,               "kIOPMLowPower")
    GENSTR(4,  flags & kIOPMPreventIdleSleep,       "kIOPMPreventIdleSleep")
    GENSTR(5,  flags & kIOPMSleepCapability,        "kIOPMSleepCapability")
    GENSTR(6,  flags & kIOPMRestartCapability,      "kIOPMRestartCapability")
    GENSTR(7,  flags & kIOPMRestart,                "kIOPMRestart")
    GENSTR(8,  flags & kIOPMSleep,                  "kIOPMSleep")
    GENSTR(9,  flags & kIOPMInitialDeviceState,     "kIOPMInitialDeviceState")
    GENSTR(10, flags & kIOPMRootDomainState,        "kIOPMRootDomainState")
    snprintf(outString, 200, "%s %s %s %s %s %s %s %s %s %s", str1, str2, str3, str4, str5, str6, str7, str8, str9, str10);
    
#undef GENSTR
}

void IOBluetoothHostControllerTransport::SetupTransportSCOParameters()
{
}

void IOBluetoothHostControllerTransport::DestroyTransportSCOParameters()
{
}

bool IOBluetoothHostControllerTransport::WaitForSystemReadyForSleep(char * name)
{
    UInt16 i;
    
    if ( mBluetoothController->SystemReadyForSleep() )
        return true;
    
    for (i = 0; i < 4; ++i)
    {
        TransportCommandSleep(this, 1000, name, true);
        if ( mBluetoothController->SystemReadyForSleep() )
            return true;
    }
    
    if ( mBluetoothController->SystemReadyForSleep() )
        return true;
    *(UInt8 *)(mBluetoothController + 1286) = 1;
    return false;
}

IOReturn IOBluetoothHostControllerTransport::StartBluetoothSleepTimer()
{
    IOReturn result;
    char * errStringShort;
    char * errStringLong;

    if ( mBluetoothSleepTimerEventSource )
    {
        result = mBluetoothSleepTimerEventSource->setTimeoutMS(17000);
        mBluetoothSleepTimerStarted = 1;
    }
    else
        result = -536870212;
  
    errStringLong = (char *) IOMalloc(0x64);
    errStringShort = (char *) IOMalloc(0x32);
    mBluetoothFamily->ConvertErrorCodeToString(result, errStringLong, errStringShort);
    IOFree(errStringLong, 0x64);
    IOFree(errStringShort, 0x32);
    
    return result;
}

void IOBluetoothHostControllerTransport::CancelBluetoothSleepTimer()
{
    if ( mBluetoothSleepTimerEventSource && mBluetoothSleepTimerStarted )
    {
        mBluetoothSleepTimerEventSource->cancelTimeout();
        mBluetoothSleepTimerStarted = false;
    }
}

os_log_t IOBluetoothHostControllerTransport::CreateOSLogObject()
{
    if ( !mInternalOSLogObject )
    {
        mInternalOSLogObject = os_log_create("com.apple.bluetooth", "KernelBluetoothTransport");
        if ( !mInternalOSLogObject )
            mInternalOSLogObject = OS_LOG_DEFAULT;
    }
    return mInternalOSLogObject;
}

IOReturn IOBluetoothHostControllerTransport::setProperties(OSObject * properties)
{
    if ( isInactive() || !mCommandGate )
        return -536870212;
    return mCommandGate->runAction(IOBluetoothHostControllerTransport::setPropertiesAction, (void *) properties);
}

IOReturn IOBluetoothHostControllerTransport::setPropertiesAction(OSObject * owner, void * arg1, void * arg2, void * arg3, void * arg4) //needs working
{
    IOBluetoothHostControllerTransport * transport;
    
    if ( !owner )
        return -536870206;
  
    transport = OSDynamicCast(IOBluetoothHostControllerTransport, owner);
    if ( transport->isInactive() )
        return 0;
    if ( transport )
        return transport->setPropertiesWL((OSObject *) arg1);
    
    return -536870206;
}

IOReturn IOBluetoothHostControllerTransport::setPropertiesWL(OSObject * properties)
{
    IOReturn result;
    IOReturn propResult;
    OSDictionary * dict;
    OSCollectionIterator * iter;
    OSSymbol * symbol;
    OSBoolean * boolean;
    OSNumber * number;
    
    result = -536870212;
    
    dict = OSDynamicCast(OSDictionary, properties);
    if ( !dict )
        return result;
    
    iter = OSCollectionIterator::withCollection(dict);
    if ( !iter )
    {
        OSSafeReleaseNULL(iter);
        return result;
    }
    
    while ( 1 )
    {
NEXT_OBJ:
        symbol = (OSSymbol *) iter->getNextObject();
        if ( !symbol )
        {
            OSSafeReleaseNULL(iter);
            return result;
        }
        
        propResult = -536870206;
        
        if (symbol->isEqualTo("SetBTRS"))
        {
            if ( mACPIMethods )
                propResult = mACPIMethods->SetBTRS();
SET_PROP:
            setProperty("SetBTGPIOResult", propResult, 64);
            result = 0;
            goto NEXT_OBJ;
        }
        
        if ( symbol->isEqualTo("SetBTPD") )
        {
            if ( mACPIMethods )
                propResult = mACPIMethods->SetBTPD();
            goto SET_PROP;
        }
        
        if ( symbol->isEqualTo("SetBTPU") )
        {
            if ( mACPIMethods )
                propResult = mACPIMethods->SetBTPU();
            goto SET_PROP;
        }
        
        if ( symbol->isEqualTo("SetBTRB") )
        {
            boolean = OSDynamicCast(OSBoolean, dict->getObject(symbol));
            if ( boolean && mACPIMethods )
                propResult = mACPIMethods->SetBTRB(boolean->getValue(), NULL);
            goto SET_PROP;
        }
        
        if ( symbol->isEqualTo("SetBTLP") )
        {
            boolean = OSDynamicCast(OSBoolean, dict->getObject(symbol));
            if ( boolean && mACPIMethods )
                propResult = mACPIMethods->SetBTLP(boolean->getValue());
            goto SET_PROP;
        }
        
        if ( !symbol->isEqualTo("SetBTPower") )
        {
            BluetoothFamilyLogPacket(mBluetoothFamily, 250, "**** [IOBluetoothHostControllerTransport][setPropertiesWL] -- Invalid Property Name -- %s ****\n", symbol->getCStringNoCopy());
            result = -536870212;
            goto NEXT_OBJ;
        }
        
        result = 0;
        if ( mACPIMethods && mACPIMethods->mROMBootMethodAvailable && (mPowerMask & 0xFFFE) == 6 )
        {
            number = OSDynamicCast(OSNumber, dict->getObject(symbol));
            if ( number && mBluetoothController )
            {
                if ( !number->unsigned8BitValue() )
                {
                    if ( !mBluetoothController->RequestPowerStateChange(kIOBluetoothHCIControllerInternalPowerStateOff, (char *) "IOBluetoothHostControllerTransport::setPropertiesWL()") || WaitForControllerPowerStateChange(kIOBluetoothHCIControllerInternalPowerStateOff, (char *) "IOBluetoothHostControllerTransport::setPropertiesWL()") )
                        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerTransport][setPropertiesWL] -- WaitForControllerPowerStateChange (OFF) failed -- this = %p ****\n", this);
                }
                else if ( number->unsigned8BitValue() == 1 )
                {
                    mBluetoothController->RequestPowerStateChange(kIOBluetoothHCIControllerInternalPowerStateOn, (char *) "IOBluetoothHostControllerTransport::setPropertiesWL()");
                    if ( !WaitForControllerPowerStateChange(kIOBluetoothHCIControllerInternalPowerStateOn, (char *) "IOBluetoothHostControllerTransport::setPropertiesWL()") && !mBluetoothFamily->mRegisterServiceCalled )
                            mBluetoothFamily->CallRegisterService();
                    else
                        os_log(mInternalOSLogObject, "**** [IOBluetoothHostControllerTransport][setPropertiesWL] -- WaitForControllerPowerStateChange (ON) failed -- this = %p ****\n", this);
                }
            }
        }
    }
}

IOReturn IOBluetoothHostControllerTransport::HardReset()
{
    return -536870212;
}

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_VERSION_11_0
void IOBluetoothHostControllerTransport::DumpTransportProviderState()
{
}
#endif

OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 0)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 1)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 2)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 3)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 4)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 5)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 6)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 7)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 8)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 9)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 10)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 11)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 12)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 13)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 14)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 15)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 16)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 17)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 18)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 19)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 20)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 21)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 22)
OSMetaClassDefineReservedUnused(IOBluetoothHostControllerTransport, 23)
