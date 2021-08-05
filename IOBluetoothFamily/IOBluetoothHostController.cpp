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

#include <IOKit/bluetooth/IOBluetoothHostController.h>
#include <IOKit/bluetooth/IOBluetoothHCIController.h>
#include <IOKit/bluetooth/IOBluetoothHCIRequest.h>
#include <IOKit/bluetooth/transport/IOBluetoothHostControllerTransport.h>
#include <sys/proc.h>

#define super IOService
OSDefineMetaClassAndAbstractStructors(IOBluetoothHostControllerTransport, super)

IOReturn IOBluetoothHostController::SendRawHCICommand(BluetoothHCIRequestID inID, UInt8 * inBuffer, IOByteCount inBufferSize, UInt8 * outBuffer, IOByteCount outBufferSize)
{
    IOReturn err;
    BluetoothHCICommandPacket * cmdPacket;
    IOBluetoothHCIRequest * request;
    IOBluetoothHCIControllerInternalPowerState powerState;
    char opStr;
    char processName;
    
    cmdPacket = (BluetoothHCICommandPacket *) inBuffer;
    if ( inBufferSize >= kMaxHCIBufferLength )
    {
        os_log(OS_LOG_DEFAULT, "REQUIRE failure: %s - file: %s:%d", "(bufferSize < kMaxHCIBufferLength)",
             "/System/Volumes/Data/SWE/macOS/BuildRoots/2288acc43c/Library/Caches/com.apple.xbs/Sources/IOBluetoothFamily_kexts/IOBluetoothFamily-8004.1.18.3/Core/Family/HCI/HostControllers/IOBluetoothHostController.cpp", __LINE__);
        err = -536870206;
        goto OVER;
    }
    if ( inBufferSize < 3 )
    {
        BluetoothFamilyLogPacket(mBluetoothFamily, 250, "**** [IOBluetoothHostController][SendRawHCICommand] -- Error -- buffer size (%llu) < 3 -- inID = %d \n", inBufferSize, inID);
        err = -536870206;
        goto OVER;
    }
    if ( cmdPacket->dataSize + 3 != inBufferSize )
    {
        BluetoothFamilyLogPacket(mBluetoothFamily, 250, "**** [IOBluetoothHostController][SendRawHCICommand] -- Error -- buffer size (%llu) != payload size (%d) + 3 -- inID = %d \n", inBufferSize, cmdPacket->dataSize, inID);
        err = -536870206;
        goto OVER;
    }
    
    err = LookupRequest(inID, &request);
    if ( err || !request )
    {
        os_log(OS_LOG_DEFAULT, "[IOBluetoothHostController][SendRawHCICommand] ### ERROR: request could not be found!");
        goto OVER;
    }
    
    request->RetainRequest("IOBluetoothHostController::SendRawHCICommand -- at the beginning");
    err = PrepareRequestForNewCommand(inID, NULL, 0xFFFF);
    if ( err )
    {
        os_log(OS_LOG_DEFAULT, "REQUIRE_NO_ERR failure: 0x%x - file: %s:%d", "/System/Volumes/Data/SWE/macOS/BuildRoots/2288acc43c/Library/Caches/com.apple.xbs/Sources/IOBluetoothFamily_kexts/IOBluetoothFamily-8004.1.18.3/Core/Family/HCI/HostControllers/IOBluetoothHostController.cpp", err, __LINE__);
        goto LABEL_34;
    }
    
    memcpy(request->mCommandBuffer, inBuffer, inBufferSize);
    request->mCommandBufferSize = inBufferSize;
    request->mOpCode = cmdPacket->opCode;
    
    mBluetoothFamily->ConvertOpCodeToString(cmdPacket->opCode, &opStr);
    
    if ( mUpdatingFirmware
      //&& ((unsigned __int16)(cmdPacket->opCode + 978) > 0x20u || (v23 = 5368709121LL, !_bittest64(&v23, (unsigned __int16)(cmdPacket->opCode + 978))))
      && cmdPacket->opCode != 65374 )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostController][SendRawHCICommand] -- mUpdatingFirmware is TRUE -- Not sending -- opCode = 0x%04x (%s) \n", cmdPacket->opCode, &opStr);
        err = -536870212;
        goto LABEL_34;
    }
    
    if (cmdPacket->opCode == 8200 || cmdPacket->opCode == 64843)
        UpdateLESetAdvertisingDataReporter(request);
    
    if ( mIdlePolicyIsOneByte )
        ChangeIdleTimerTime("IOBluetoothHostController::SendRawHCICommand()", *(unsigned int *)(this + 1264));
    
    if ( !mBluetoothTransport || mBluetoothTransport->isInactive() )
    {
        memset(&processName, 170, 0x100uLL);
        proc_name(request->mPID, &processName, 0x100);
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostController][SendRawHCICommand] -- Transport is inactive -- opCode = 0x%04X (%s), From: %s (%d), mNumberOfCommandsAllowedByHardware is %d, requestPtr = 0x%04x -- this = 0x%04x\n", cmdPacket->opCode, &opStr, &processName, request->mPID, mNumberOfCommandsAllowedByHardware, ConvertAddressToUInt32(request), ConvertAddressToUInt32(this));
        err = -536870212;
        goto LABEL_34;
    }
    err = 0;
    
    if ( TransportRadioPowerOff(cmdPacket->opCode, "Unknown", 255, request) )
        goto LABEL_34;
    
    request->SetResultsBufferPtrAndSize(outBuffer, outBufferSize);
    err = -536870212;
    if ( !mBluetoothTransport || mBluetoothTransport->isInactive() || mTransportIsGoingAway )
        goto LABEL_34;
    
    SetHCIRequestRequireEvents(cmdPacket->opCode, request);
    
    if ( GetTransportCurrentPowerState(&powerState) )
    {
        if ( mBluetoothTransport && mBluetoothTransport->mSystemOnTheWayToSleep )
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostController][SendRawHCICommand] -- Returned Error -- System is going to sleep -- mNumberOfCommandsAllowedByHardware is %d, requestPtr = 0x%04x, opCode = 0x%04x (%s) \n", mNumberOfCommandsAllowedByHardware, ConvertAddressToUInt32(request), cmdPacket->opCode, &opStr);
        else
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostController][SendRawHCICommand] -- Returned Error -- Current power state is SLEEP and Not PowerStateChange and Synchronous request -- mNumberOfCommandsAllowedByHardware is %d, requestPtr = 0x%04x, opCode = 0x%04x (%s) \n", mNumberOfCommandsAllowedByHardware, ConvertAddressToUInt32(request), cmdPacket->opCode, &opStr);
        goto LABEL_34;
    }
    
    if ( (!mBluetoothTransport || !mBluetoothTransport->mSystemOnTheWayToSleep) && (powerState != 2 || Power_State_Change_In_Progress() || request->mAsyncNotify) )
    {
        if ( cmdPacket->opCode == 8217 )
            request->mConnectionHandle = *(BluetoothConnectionHandle *) cmdPacket->data;
        
        err = EnqueueRequest(request);
        if ( err == 99 )
        {
LABEL_69:
            request->Start();
            err = request->mStatus;
            if ( request->mStatus > 65282 )
                goto LABEL_34;
        
            if ( request->mState == 1 )
            {
                AbortRequestAndSetTime(request);
                IncrementHCICommandTimeOutCounter(request->GetCommandOpCode());
                goto LABEL_34;
            }
            goto LABEL_34;
        }
        if ( !err )
        {
            err = EnqueueRequestForController(request);
            if ( err )
            {
                AbortRequestAndSetTime(request);
                os_log(OS_LOG_DEFAULT, "[IOBluetoothHostController][SendRawHCICommand] ### ERROR: EnqueueRequestForController failed (err=%x)", err);
                goto LABEL_34;
            }
            goto LABEL_69;
        }
      
        os_log(OS_LOG_DEFAULT, "[IOBluetoothHostController][SendRawHCICommand] ### ERROR: EnqueueRequest failed (err=%x)", err);
    }
  
LABEL_34:
    if ( request )
        request->ReleaseRequest("IOBluetoothHostController::SendRawHCICommand -- before exiting");
    
OVER:
    if ( mIdlePolicyIsOneByte )
        ChangeIdleTimerTime("IOBluetoothHostController::SendRawHCICommand()", *(unsigned int *)(this + 1264));
    
    return err;
}

IOReturn IOBluetoothHostController::HardResetController(UInt16 arg2)
{
    IOReturn err;
    IORegistryEntry * entry;
    
    if ( !mPowerStateNotChanging )
    {
        BluetoothFamilyLogPacketWithOSLog(mInternalOSLogObject, mBluetoothFamily, 251, "**** [IOBluetoothHostController][HardResetController] -- Power state is changing, prohibiting Bluetooth hard reset error recovery ****\n");
        return -536870174;
    }
    
    *(_BYTE *)(*(_QWORD *)(this + 824) + 453LL) = 0;
    
    entry = IORegistryEntry::fromPath("/options", gIODTPlane);
    if ( entry && OSDynamicCast(OSData, entry->getProperty("SkipBluetoothHardResetErrorRecovery")) )
    {
        OSSafeReleaseNULL(entry);
        os_log(mInternalOSLogObject, "IOBluetoothHostController::HardResetController -- SkipBluetoothHardResetErrorRecovery is set in nvram prohibiting Bluetooth hard reset error recovery");
        return -536870174;
    }
    OSSafeReleaseNULL(entry);
    
    if ( arg2 > 5 )
        return -536870206;
    
    if ( (*(_WORD *)(this + 880) & 0xFFFE) != 6 )
    {
        if ( mBluetoothTransport )
        {
            mBluetoothTransport->RetainTransport("IOBluetoothHostController::HardResetController()");
            err = mBluetoothTransport->DoDeviceReset(2 - (arg2 == 1));
            if ( !err && mBluetoothFamily->mSearchForTransportEventTimer )
            {
                mBluetoothFamily->mReceivedTransportNotification = 0;
                mBluetoothFamily->ConvertErrorCodeToString(mBluetoothFamily->mSearchForTransportEventTimer->setTimeoutMS(20000), &v49, &v42);
                mBluetoothFamily->mSearchForTransportEventTimerHasTimeout = 1;
            }
            mBluetoothTransport->ReleaseTransport("IOBluetoothHostController::HardResetController()");
            return err;
        }
        return -536870212;
    }
    
    if ( *(_WORD *)(this + 880) != 7 || mCanDoHardReset )
    {
        mCanDoHardReset = 1;
        if ( mBluetoothFamily->ReachHardResetLimit(this) )
        {
            BluetoothFamilyLogPacketWithOSLog(mInternalOSLogObject, mBluetoothFamily, 249, "**** [IOBluetoothHostController][HardResetController] -- cannot call BluetoothResetController() because hard reset counter (%d) reached the limit (%d) ****\n", mBluetoothFamily->GetHardResetCounter(this), 5);
            *(bool *)(mBluetoothFamily + 453LL) = 0;
            return -536870212;
        }
        
        if ( mBluetoothTransport )
        {
            mBluetoothTransport->RetainTransport("IOBluetoothHostController::HardResetController()");
            mBluetoothFamily->IncrementHardResetCounter(this);
            BluetoothFamilyLogPacketWithOSLog(mInternalOSLogObject, mBluetoothFamily, 249, "**** [IOBluetoothHostController][HardResetController] -- incremented hard reset counter (%d) -- calling mBluetoothTransport->DoDeviceReset() ****\n", mBluetoothFamily->GetHardResetCounter(this));
            err = mBluetoothTransport->DoDeviceReset(arg2);
            if ( !err )
            {
                BroadcastNotification(13, kIOBluetoothHCIControllerConfigStateUninitialized, kIOBluetoothHCIControllerConfigStateUninitialized);
                mBluetoothFamily->ResetHardResetCounter(this);
            }
            mBluetoothTransport->ReleaseTransport("IOBluetoothHostController::HardResetController()");
            return err;
        }
        return -536870212;
    }
    
    BluetoothFamilyLogPacketWithOSLog(mInternalOSLogObject, mBluetoothFamily, 249, "**** [IOBluetoothHostController][HardResetController] -- cannot call BluetoothResetController() because hard reset is being performed ****\n");
    return -536870212;
}

IOReturn IOBluetoothHostController::PerformTaskForPowerManagementCalls(IOOptionBits options)
{
    if ( options == -536870128 || options == -536870112 || options == -536870320 )
        *(_BYTE *)(this + 959) = 0;
    
    else if ( options == -536870272 )
        *(_BYTE *)(this + 959) = 1;
    
    return 0;
}
