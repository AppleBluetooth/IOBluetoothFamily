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

#include <IOKit/bluetooth/IOBluetoothHCIRequest.h>
#include <IOKit/bluetooth/IOBluetoothHCIController.h>
#include <sys/proc.h>

#define super OSObject
OSDefineMetaClassAndStructors(IOBluetoothHCIRequest, super)

IOBluetoothHCIRequest * IOBluetoothHCIRequest::Create(IOCommandGate * commandGate, IOBluetoothHostController * hostController, bool async, UInt32 timeout, UInt32 controlFlags)
{
    IOBluetoothHCIRequest * me;
    
    me = OSTypeAlloc(IOBluetoothHCIRequest);
    if ( !me || !me->init(commandGate, hostController) )
        return NULL;
    
    me->mTimeout = timeout;
    me->mOriginalTimeout = timeout;
    me->mAsyncNotify = async;
    me->mControlFlags = controlFlags;
    
    if ( timeout < 500 )
    {
        if ( hostController )
            os_log(hostController->mInternalOSLogObject, "[IOBluetoothHCIRequest][Create] -- %s has a too small timeout, a reasonable value would be 5000", me->RequestDescription("CREATE"));
        else
            os_log(OS_LOG_DEFAULT, "[IOBluetoothHCIRequest][Create] -- %s has a too small timeout, a reasonable value would be 5000", me->RequestDescription("CREATE"));
        me->mTimeout = 5000; //18558553690337LL - 4321
    }
    return me;
}

IOReturn IOBluetoothHCIRequest::Dispose(IOBluetoothHCIRequest * request)
{
    char opStr;
    
    if ( !request )
        return -536870206;
    
    if ( request->mState )
    {
        request->mHCIRequestDeleteWasCalled = true;
        request->mOwningTaskID = NULL;
        return -536870187;
    }

    if ( request->mHostController )
        request->mHostController->mBluetoothFamily->ConvertOpCodeToString(request->mOpCode, &opStr);
    OSSafeReleaseNULL(request);
    
    return kIOReturnSuccess;
}

IOReturn IOBluetoothHCIRequest::DisposeRequest()
{
    char opStr;
    
    if ( mState )
    {
        mHCIRequestDeleteWasCalled = 1;
        mOwningTaskID = NULL;
        return -536870187;
    }
    
    if ( mHostController )
        mHostController->mBluetoothFamily->ConvertOpCodeToString(mOpCode, &opStr);
    release();
    
    return kIOReturnSuccess;
}

bool IOBluetoothHCIRequest::init(IOCommandGate * commandGate, IOBluetoothHostController * hostController)
{
    if ( !super::init() || !commandGate || !hostController )
        return false;
    
    mCommandGate = commandGate;
    mCommandGate->retain();
    mHostController = hostController;
    mID = 0xFFFF;
    mPrivateResultsSize = 2048;
    mTransportID = 0;
    mState = 0;
    mAsyncNotify = false;
    mOpCode = 0;
    mCommandBufferSize = 0;
    mResultsPtr = NULL;
    mResultsSize = 0;
    mStatus = 0;
    mTimeout = 0;
    mPID = proc_selfpid();
    mHCIRequestDeleteWasCalled = 0;
    mStartTimeForDelete = 0;
    
    __reserved1 = 0;
    mExpectedEvents = 0;
    mExpectedExplicitCompleteEvents = 0;
    __reserved4 = 0;
    __reserved5 = 0;
    __reserved6 = 0;
    __reserved7 = 0;
    __reserved8 = 0;
    __reserved9 = 0;
    __reserved10 = 0;
    __reserved11 = 0;
    __reserved12 = 0;
    __reserved13 = 0;
    
    return true;
}

void IOBluetoothHCIRequest::InitializeRequest()
{
    char str;

    if ( mHostController )
        mHostController->mBluetoothFamily->ConvertOpCodeToString(mOpCode, &str);
    
    mState = 0;
    mStatus = 0;
    mHCIRequestDeleteWasCalled = 0;
    mStartTimeForDelete = 0;
    
    __reserved1 = 0;
    mExpectedEvents = 0;
    mExpectedExplicitCompleteEvents = 0;
    __reserved4 = 0;
    __reserved5 = 0;
    __reserved6 = 0;
    __reserved7 = 0;
    __reserved8 = 0;
    __reserved9 = 0;
    __reserved10 = 0;
    __reserved11 = 0;
    __reserved12 = 0;
    __reserved13 = 0;
}

void IOBluetoothHCIRequest::free()
{
    if ( mNotificationMessage )
        IOFree(mNotificationMessage, mNotificationMessageSize);
    
    OSSafeReleaseNULL(mCommandGate);
    if ( mTimer )
    {
        mTimer->cancelTimeout();
        mTimer->getWorkLoop()->removeEventSource(mTimer);
        OSSafeReleaseNULL(mTimer);
    }
    
    super::free();
}

void IOBluetoothHCIRequest::retain() const
{
    char opStr;
    
    if ( mHostController )
        mHostController->mBluetoothFamily->ConvertOpCodeToString(mOpCode, &opStr);
  
    super::retain();
}

void IOBluetoothHCIRequest::RetainRequest(char *)
{
    retain();
}

void IOBluetoothHCIRequest::release() const
{
    char opStr;
    
    if ( mHostController )
        mHostController->mBluetoothFamily->ConvertOpCodeToString(mOpCode, &opStr);
  
    super::release();
}

void IOBluetoothHCIRequest::ReleaseRequest(char *)
{
    char opStr;

    if ( mHostController )
        mHostController->mBluetoothFamily->ConvertOpCodeToString(mOpCode, &opStr);

    release();
}

const char * IOBluetoothHCIRequest::RequestDescription(const char * name)
{
    static const char * BluetoothHCIRequestStateStrings[3] = {"IDLE", "WAITING", "BUSY"};
    
    // These three are exported
    char * stateString;
    char * syncRequestString;
    char * descriptionBuffer;
    
    char processName;
    char opStr;
    char errStringShort;
    char errStringLong;
    
    memset(&processName, 170, 0x100);
    snprintf(&processName, 0x100, "Unknown");
    proc_name(mPID, &processName, 0x100u);
    
    stateString = (char *) IOMalloc(0xA);
    if ( mState <= 2 )
        snprintf(stateString, 0xA, "%s", BluetoothHCIRequestStateStrings[mState]);
    
    syncRequestString = (char *) IOMalloc(0x14);
    if ( mAsyncNotify )
        snprintf(syncRequestString, 0x14, "Asynchronous");
    else
        snprintf(syncRequestString, 0x14, "Synchronous");
    
    //Well, here is a bug in apple's code... it did not explicitly handle the case in which mHostController would be NULL... could be exploited! I fixed it myself for now
    if ( !mHostController )
        return "";
    
    mHostController->mBluetoothFamily->ConvertOpCodeToString(GetCommandOpCode(), &opStr);
    mHostController->mBluetoothFamily->ConvertErrorCodeToString(mStatus, &errStringLong, &errStringShort);
    
    descriptionBuffer = (char *) IOMalloc(0x100);
    snprintf(descriptionBuffer, 0x100, "[0x%04X] %s OpCode 0x%04X (%s) from: %s (%d)  %s  status: 0x%02X (%s) state: %d (%s) timeout: %d originalTimeout: %d", mHostController->ConvertAddressToUInt32(this), name, GetCommandOpCode(), &opStr, &processName, mPID, syncRequestString, mStatus, &errStringLong, mState, stateString, mTimeout, mOriginalTimeout);
    return descriptionBuffer;
}

IOReturn IOBluetoothHCIRequest::Start()
{
    IOReturn result;
    UInt64 time;
    char opStr;
    char * log1;
    char * log2;
    
    log1 = (char *) IOMalloc(0x100);
    log2 = (char *) IOMalloc(0x32);
    
    RequestDescription("[IOBluetoothHCIRequest][Start] -- ");
    mStatus = 0;
    if ( mAsyncNotify )
        return kIOReturnSuccess;
    
    if ( mHostController && !mHostController->GetCompleteCodeForCommand(GetCommandOpCode(), NULL) )
    {
        BluetoothFamilyLogPacket(mHostController->mBluetoothFamily, 249, "[%p] IOBluetoothHCIRequest::Start() -- Cannot find completion code for 0x%04X -- Is this a new HCI command? Don't forget to add it to SetHCIRequestRequireEvents() and GetCompleteCodeForCommand() in the kernel", this, GetCommandOpCode());
        BluetoothFamilyLogPacket(mHostController->mBluetoothFamily, 250, "No complete code for 0x%04X new cmd?", GetCommandOpCode());
    }
    
    clock_interval_to_deadline(mTimeout / 0x3E8 + 1, 1000000000, &time);
    OSAddAtomic16(1, &mHostController->mControllerOutstandingCalls);
    result = mCommandGate->commandSleep(this, time, THREAD_UNINT);
    OSAddAtomic16(0xFFFFFFFF, &mHostController->mControllerOutstandingCalls);
    
    switch ( result )
    {
        case 0:
            return kIOReturnSuccess;
            
        case 1:
            mStatus = 65282 - (mState == 2);
            if ( mHostController )
            {
                mHostController->mBluetoothFamily->ConvertOpCodeToString(GetCommandOpCode(), &opStr);
                
                if ( mStatus == 65281 )
                {
                    snprintf(log1, 0x100, "[%p] IOBluetoothHCIRequest::Start() -- commandSleep() timeout occurred for 0x%04X (%s) -- mNumberOfCommandsAllowedByHardware is %d - mStatus = kBluetoothSyncHCIRequestTimedOutAfterSent!", this, GetCommandOpCode(), &opStr, mHostController->mNumberOfCommandsAllowedByHardware);
                    snprintf(log2, 0x32, "HCIRequest::Start 0x%04X timed out Sent", GetCommandOpCode());
                    if ( !mHostController->mNumberOfCommandsAllowedByHardware )
                        mHostController->mNumberOfCommandsAllowedByHardware = 1;
                }
                else
                {
                    snprintf(log1, 0x100, "[%p] IOBluetoothHCIRequest::Start() -- commandSleep() timeout occurred for 0x%04X (%s) -- mNumberOfCommandsAllowedByHardware is %d - mStatus = kBluetoothSyncHCIRequestTimedOutWaitingToBeSent!", this, GetCommandOpCode(), &opStr, mHostController->mNumberOfCommandsAllowedByHardware);
                    snprintf(log2, 0x32, "HCIRequest::Start 0x%04X timed out Wait", GetCommandOpCode());
                }
            }
            else
            {
                if ( mStatus == 65281 )
                {
                    snprintf(log1, 0x100, "[%p] IOBluetoothHCIRequest::Start() -- commandSleep() timeout occurred for 0x%04X -- mStatus = kBluetoothSyncHCIRequestTimedOutAfterSent", this, GetCommandOpCode());
                    snprintf(log2, 0x32, "HCIRequest::Start 0x%04X timed out Sent", GetCommandOpCode());
                }
                else
                {
                    snprintf(log1, 0x100, "[%p] IOBluetoothHCIRequest::Start() -- commandSleep() timeout occurred for 0x%04X -- mStatus = kBluetoothSyncHCIRequestTimedOutWaitingToBeSent", this, GetCommandOpCode());
                    snprintf(log2, 0x32, "HCIRequest::Start 0x%04X timed out Wait", GetCommandOpCode());
                }
            }
            
            if ( mHostController )
            {
                BluetoothFamilyLogPacket(mHostController->mBluetoothFamily, 249, "%s", log1);
                BluetoothFamilyLogPacket(mHostController->mBluetoothFamily, 250, "%s", log2);
            }
            mResultsPtr = NULL;
            mResultsSize = 0;
            return kIOReturnSuccess;
            
        case -536870174:
            panic("\"IOBluetoothHCIRequest::Start() -- commandSleep not called in the workloop\\n\"@/System/Volumes/Data/SWE/macOS/BuildRoots/2288acc43c/Library/Caches/com.apple.xbs/Sources/IOBluetoothFamily_kexts/IOBluetoothFamily-8004.1.18.3/Core/Family/HCI/IOBluetoothHCIRequest.cpp:434");
            break;
    }
    
    os_log(mHostController ? mHostController->mInternalOSLogObject : OS_LOG_DEFAULT, "IOBluetoothHCIRequest::Start() -- commandSleep return %#x for 0x%04X", result, GetCommandOpCode());
    return kIOReturnSuccess;
}

void IOBluetoothHCIRequest::Complete()
{
    if ( mTimer )
        mTimer->cancelTimeout();
    
    if ( !mAsyncNotify )
        mCommandGate->commandWakeup(this);
}

void IOBluetoothHCIRequest::SetState(BluetoothHCIRequestState inState)
{
    if ( mState == inState )
        return;
    
    mState = inState;
    if ( !inState && mTimer )
        mTimer->cancelTimeout();
}

void IOBluetoothHCIRequest::SetCallbackInfo(BluetoothHCIRequestCallbackInfo * inInfo)
{
    if ( !inInfo )
    {
        bzero(&mCallbackInfo, 40);
        return;
    }
    
    mCallbackInfo.userCallback      = inInfo->userCallback;
    mCallbackInfo.userRefCon        = inInfo->userRefCon;
    mCallbackInfo.internalRefCon    = inInfo->internalRefCon;
    mCallbackInfo.asyncIDRefCon     = inInfo->asyncIDRefCon;
    mCallbackInfo.reserved          = inInfo->reserved;
}

void IOBluetoothHCIRequest::SetCallbackInfoToZero()
{
    bzero(&mCallbackInfo, 40);
}

void IOBluetoothHCIRequest::SetResultsDataSize(IOByteCount inCount)
{
    if ( mResultsPtr )
        mResultsSize = inCount;
    else
        mPrivateResultsSize = inCount;
}

void IOBluetoothHCIRequest::SetResultsBufferPtrAndSize(UInt8 * resultsBuffer, IOByteCount inSize)
{
    if ( !resultsBuffer )
        inSize = 0;
    mResultsPtr = resultsBuffer;
    mResultsSize = inSize;
}

void IOBluetoothHCIRequest::CopyDataIntoResultsPtr(UInt8 * inDataPtr, IOByteCount inSize)
{
    if ( !mResultsPtr || mAsyncNotify )
    {
        memcpy(mPrivateResultsBuffer, inDataPtr, inSize);
        mPrivateResultsSize = inSize;
        return;
    }
    memcpy(mResultsPtr, inDataPtr, inSize);
    mResultsSize = inSize;
}

UInt8 * IOBluetoothHCIRequest::GetResultsBuffer()
{
    if ( mResultsPtr && !mAsyncNotify )
        return mResultsPtr;
    return mPrivateResultsBuffer;
}

IOByteCount IOBluetoothHCIRequest::GetResultsBufferSize()
{
    if ( mResultsPtr && !mAsyncNotify )
        return mResultsSize;
    return mPrivateResultsSize;
}

inline BluetoothHCICommandOpCode IOBluetoothHCIRequest::GetCommandOpCode()
{
    if ( mCommandBufferSize )
        return ((BluetoothHCICommandPacket *) mCommandBuffer)->opCode;
    return 0;
}

void IOBluetoothHCIRequest::StartTimer()
{
    if ( mState != 2 )
    {
        if ( mHostController )
        {
            os_log(mHostController->mInternalOSLogObject, "[IOBluetoothHCIRequest][startTimer] -- called on a non busy request %s", RequestDescription("STARTTIMER"));
            BluetoothFamilyLogPacket(mHostController->mBluetoothFamily, 249, "Failed StartTimer (2)");
            BluetoothFamilyLogPacket(mHostController->mBluetoothFamily, 250, "Failed StartTimer (2)");
        }
        else
            os_log(OS_LOG_DEFAULT, "[IOBluetoothHCIRequest][startTimer] -- called on a non busy request %s", RequestDescription("STARTTIMER"));
        return;
    }
    if ( !mTimeout )
    {
        if ( !mHostController )
            os_log(OS_LOG_DEFAULT, "[IOBluetoothHCIRequest][startTimer] -- mOpCode = 0x%04X -- mTimeout is 0", mOpCode);
        else
            BluetoothFamilyLogPacket(mHostController->mBluetoothFamily, 249, "[IOBluetoothHCIRequest][startTimer] -- mOpCode = 0x%04X -- mTimeout is 0", mOpCode);
        return;
    }
    if ( mTimer )
    {
        mTimer->setTimeoutMS(mOriginalTimeout);
        return;
    }
    
    mTimer = IOTimerEventSource::timerEventSource(this, IOBluetoothHCIRequest::timerFired);
    if ( !mTimer )
    {
        if ( mHostController )
            BluetoothFamilyLogPacket(mHostController->mBluetoothFamily, 250, "[IOBluetoothHCIRequest][startTimer] -- mOpCode = 0x%04X -- timerEventSource() failed", mOpCode);
        else
          os_log(OS_LOG_DEFAULT, "[IOBluetoothHCIRequest][startTimer] -- mOpCode = 0x%04X -- timerEventSource() failed", mOpCode);
        return;
    }
    
    if ( mCommandGate->getWorkLoop() )
        mCommandGate->getWorkLoop()->addEventSource(mTimer);
    else
    {
        if ( mHostController )
            os_log(mHostController->mInternalOSLogObject, "[IOBluetoothHCIRequest][startTimer] -- mWorkLoop is NULL");
        else
            os_log(OS_LOG_DEFAULT, "[IOBluetoothHCIRequest][startTimer] -- mWorkLoop is NULL");
    }
    
    mTimer->setTimeoutMS(mOriginalTimeout);
}

void IOBluetoothHCIRequest::timerFired(OSObject * owner, IOTimerEventSource * sender)
{
    IOBluetoothHCIRequest * me;
    
    me = OSDynamicCast(IOBluetoothHCIRequest, owner);
    if (me)
        me->handleTimeout();
}

void IOBluetoothHCIRequest::handleTimeout()
{
    BluetoothHCIEventCode eventCode;
    char str;
    
    if ( !mHostController )
    {
        os_log(OS_LOG_DEFAULT, "REQUIRE failure: %s - file: %s:%d", "mBluetoothController", "/System/Volumes/Data/SWE/macOS/BuildRoots/2288acc43c/Library/Caches/com.apple.xbs/Sources/IOBluetoothFamily_kexts/IOBluetoothFamily-8004.1.18.3/Core/Family/HCI/IOBluetoothHCIRequest.cpp", __LINE__);
        return;
    }
    
    bzero(&str, 0x200);
    if ( mHostController->mNumberOfCommandsAllowedByHardware )
    {
        snprintf(&str, 0x200, "[%s] Bluetooth warning: An HCI Req timeout occurred -- missing completion event.", RequestDescription(NULL));
        
        if ( !*(UInt8 *)(mHostController + 969) )
        {
            os_log(mHostController->mInternalOSLogObject, "%s", &str);
            BluetoothFamilyLogPacket(mHostController->mBluetoothFamily, 249, "%s", &str);
            BluetoothFamilyLogPacket(mHostController->mBluetoothFamily, 250, "An HCI Req 0X%04X timeout occurred -- missing completion event", GetCommandOpCode());
        }
    }
    else
    {
        snprintf(&str, 0x200, "[%s] Bluetooth warning: An HCI Req timeout occurred.", IOBluetoothHCIRequest::RequestDescription(NULL));
        if ( !*(UInt8 *)(mHostController + 969) )
        {
            os_log(mHostController->mInternalOSLogObject, "%s", &str);
            BluetoothFamilyLogPacket(mHostController->mBluetoothFamily, 249, "%s", &str);
            BluetoothFamilyLogPacket(mHostController->mBluetoothFamily, 250, "An HCI Req 0X%04X timeout occurred", GetCommandOpCode());
            mHostController->mNumberOfCommandsAllowedByHardware = 1;
        }
    }
    if ( mState )
    {
        mStatus = 16;
        if ( mHostController->GetCompleteCodeForCommand(GetCommandOpCode(), &eventCode) )
            mHostController->BroadcastEventNotification(mID, eventCode, 16, NULL, 0, GetCommandOpCode(), false, 255);
        
        mHostController->DequeueRequest(this);
        Complete();
        if ( !mHostController->WillResetModule() )
            mHostController->ProcessWaitingRequests(false);
    }
    mHostController->IncrementHCICommandTimeOutCounter(mOpCode);
}

UInt32 IOBluetoothHCIRequest::GetCurrentTime()
{
    UInt64 abstime;
    UInt64 time;

    clock_get_uptime(&abstime);
    absolutetime_to_nanoseconds(abstime, &time);
    // This is really weird... Why not just return time directly? maybe reversing bug
    return (UInt32)(0x20C49BA5E353F7CF * (time >> 3) >> 64) >> 4;
}

void IOBluetoothHCIRequest::SetStartTimeForDelete()
{
    mStartTimeForDelete = GetCurrentTime();
}

bool IOBluetoothHCIRequest::CompareDeviceAddress(const BluetoothDeviceAddress * inDeviceAddress)
{
    bool isInDeviceAddressValid;
    bool ismDeviceAddressValid;

    isInDeviceAddressValid = false;
    if ( inDeviceAddress && (inDeviceAddress->data[0] || inDeviceAddress->data[1] || inDeviceAddress->data[2] || inDeviceAddress->data[3] || inDeviceAddress->data[4] || inDeviceAddress->data[5]) )
        isInDeviceAddressValid = true;
    
    ismDeviceAddressValid = false;
    if ( mDeviceAddress.data[0] || mDeviceAddress.data[1] || mDeviceAddress.data[2] || mDeviceAddress.data[3] || mDeviceAddress.data[4] || mDeviceAddress.data[5] )
        ismDeviceAddressValid = true;
    
    if ( isInDeviceAddressValid && ismDeviceAddressValid )
        return !memcmp(inDeviceAddress, &mDeviceAddress, 6);
    return isInDeviceAddressValid == ismDeviceAddressValid;
}
