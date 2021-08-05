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
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS A SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include <sys/proc.h>
#include <kern/thread.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/bluetooth/IOBluetoothHCIController.h>
#include <IOKit/bluetooth/IOBluetoothHostController.h>
#include <IOKit/bluetooth/IOBluetoothACPIMethods.h>
#include <IOKit/bluetooth/transport/IOBluetoothHostControllerUSBTransport.h>

#define super IOService
OSDefineMetaClassAndStructors(IOBluetoothHCIController, super)

IOWorkLoop * IOBluetoothHCIController::getWorkLoop() const
{
    return mWorkLoop;
}

IOCommandGate * IOBluetoothHCIController::getCommandGate() const
{
    return mCommandGate;
}

IOWorkLoop * IOBluetoothHCIController::getUSBHardResetWorkLoop() const
{
    return mUSBHardResetWorkLoop;
}

IOCommandGate * IOBluetoothHCIController::getUSBHardResetCommandGate() const
{
    return mUSBHardResetCommandGate;
}

void IOBluetoothHCIController::LogPacket(UInt8 packetType, void * packetData, size_t packetSize)
{
    //if ( mPacketLogger )
        //mPacketLogger->LogPacket(packetType, packetData, packetSize);
}

IOReturn IOBluetoothHCIController::AddHCIEventNotification(task_t inOwningTask, mach_port_t inPort, void * refCon)
{
    IOReturn result;
    char * procName;
    int i;
    
    procName = (char *) IOMalloc(0x100);
    snprintf(procName, 0x100, "Unknown");
    proc_name(proc_selfpid(), procName, 0x100);
    
    i = 0;
    while ( mHCIEventListenersList[i].owningTask || mHCIEventListenersList[i].port )
    {
        ++i;
        if ( i == 64 )
        {
            result = -536870210;
            goto OVER;
        }
    }
    mHCIEventListenersList[i].owningTask = inOwningTask;
    mHCIEventListenersList[i].port = inPort;
    mHCIEventListenersList[i].refCon = refCon;
    mHCIEventListenersList[i].unknown = 1;
    
    result = kIOReturnSuccess;
    //if ( !strcmp(procName, "bluetoothd") )
      //  *((_BYTE *)&loc_10208 + this) = i;
OVER:
    if ( !strcmp(procName, "bluetoothd") )
        if ( mActiveBluetoothHardware && mActiveBluetoothHardware->mBluetoothHostController )
            mActiveBluetoothHardware->mBluetoothHostController->FoundBluetoothd();
    return result;
}

IOReturn IOBluetoothHCIController::RemoveHCIEventNotification(task *provider)
{
    char * procName = (char *) IOMalloc(0x100);
    snprintf(procName, 0x100, "Unknown");
    proc_name(proc_selfpid(), procName, 0x100);
    
    if ( mHCIEventListenersList )
    {
        for ( int i = 0; i < 64; i += 1 )
        {
            if ( mHCIEventListenersList[i].owningTask == provider )
            {
                bzero(&mHCIEventListenersList[i], 32);
                return kIOReturnSuccess;
            }
        }
    }
    return -536870208;
}

void IOBluetoothHCIController::ConvertErrorCodeToString(UInt32 errorCode, char * outStringLong, char * outStringShort)
{
    if ( errorCode & 0x80000000 )
    {
        switch ( errorCode + 536870212 )
        {
            case -699:
                snprintf(outStringLong, 0x64, "kIOReturnInvalid");
                snprintf(outStringShort, 0x32, "Invalid");
                return;
            
            case 0:
                snprintf(outStringLong, 0x64, "kIOReturnError");
                snprintf(outStringShort, 0x32, "Error");
                return;
                
            case 1:
                snprintf(outStringLong, 0x64, "kIOReturnNoMemory");
                snprintf(outStringShort, 0x32, "NoMemory");
                return;
                
            case 2:
                snprintf(outStringLong, 0x64, "kIOReturnNoResources");
                snprintf(outStringShort, 0x32, "NoResources");
                return;
                
            case 3:
                snprintf(outStringLong, 0x64, "kIOReturnIPCError");
                snprintf(outStringShort, 0x32, "IPCError");
                return;
                
            case 4:
                snprintf(outStringLong, 0x64, "kIOReturnNoDevice");
                snprintf(outStringShort, 0x32, "NoDevice");
                return;
                
            case 5:
                snprintf(outStringLong, 0x64, "kIOReturnNotPrivileged");
                snprintf(outStringShort, 0x32, "NotPrivileged");
                return;
            
            case 6:
                snprintf(outStringLong, 0x64, "kIOReturnBadArgument");
                snprintf(outStringShort, 0x32, "BadArgument");
                return;
            
            case 7:
                snprintf(outStringLong, 0x64, "kIOReturnLockedRead");
                snprintf(outStringShort, 0x32, "LockedRead");
                return;
            
            case 8:
                snprintf(outStringLong, 0x64, "kIOReturnLockedWrite");
                snprintf(outStringShort, 0x32, "LockedWrite");
                return;
            
            case 9:
                snprintf(outStringLong, 0x64, "kIOReturnExclusiveAccess");
                snprintf(outStringShort, 0x32, "ExclusiveAccess");
                return;
            
            case 10:
                snprintf(outStringLong, 0x64, "kIOReturnBadMessageID");
                snprintf(outStringShort, 0x32, "BadMessageID");
                return;
            
            case 11:
                snprintf(outStringLong, 0x64, "kIOReturnUnsupported");
                snprintf(outStringShort, 0x32, "Unsupported");
                return;
            
            case 12:
                snprintf(outStringLong, 0x64, "kIOReturnVMError");
                snprintf(outStringShort, 0x32, "VMError");
                return;
            
            case 13:
                snprintf(outStringLong, 0x64, "kIOReturnInternalError");
                snprintf(outStringShort, 0x32, "InternalError");
                return;
            
            case 14:
                snprintf(outStringLong, 0x64, "kIOReturnIOError");
                snprintf(outStringShort, 0x32, "IOError");
                return;
            
            case 16:
                snprintf(outStringLong, 0x64, "kIOReturnCannotLock");
                snprintf(outStringShort, 0x32, "CannotLock");
                return;
            
            case 17:
                snprintf(outStringLong, 0x64, "kIOReturnNotOpen");
                snprintf(outStringShort, 0x32, "NotOpen");
                return;
            
            case 18:
                snprintf(outStringLong, 0x64, "kIOReturnNotReadable");
                snprintf(outStringShort, 0x32, "NotReadable");
                return;
            
            case 19:
                snprintf(outStringLong, 0x64, "kIOReturnNotWritable");
                snprintf(outStringShort, 0x32, "NotWritable");
                return;
            
            case 20:
                snprintf(outStringLong, 0x64, "kIOReturnNotAligned");
                snprintf(outStringShort, 0x32, "NotAligned");
                return;
            
            case 21:
                snprintf(outStringLong, 0x64, "kIOReturnBadMedia");
                snprintf(outStringShort, 0x32, "BadMedia");
                return;
            
            case 22:
                snprintf(outStringLong, 0x64, "kIOReturnStillOpen");
                snprintf(outStringShort, 0x32, "StillOpen");
                return;
            
            case 23:
                snprintf(outStringLong, 0x64, "kIOReturnRLDError");
                snprintf(outStringShort, 0x32, "RLDError");
                return;
            
            case 24:
                snprintf(outStringLong, 0x64, "kIOReturnDMAError");
                snprintf(outStringShort, 0x32, "DMAError");
                return;
            
            case 25:
                snprintf(outStringLong, 0x64, "kIOReturnBusy");
                snprintf(outStringShort, 0x32, "Busy");
                return;
            
            case 26:
                snprintf(outStringLong, 0x64, "kIOReturnTimeout");
                snprintf(outStringShort, 0x32, "Timeout");
                return;
            
            case 27:
                snprintf(outStringLong, 0x64, "kIOReturnOffline");
                snprintf(outStringShort, 0x32, "Offline");
                return;
            
            case 28:
                snprintf(outStringLong, 0x64, "kIOReturnNotReady");
                snprintf(outStringShort, 0x32, "NotReady");
                return;
            
            case 29:
                snprintf(outStringLong, 0x64, "kIOReturnNotAttached");
                snprintf(outStringShort, 0x32, "NotAttached");
                return;
            
            case 30:
                snprintf(outStringLong, 0x64, "kIOReturnNoChannels");
                snprintf(outStringShort, 0x32, "NoChannels");
                return;
            
            case 31:
                snprintf(outStringLong, 0x64, "kIOReturnNoSpace");
                snprintf(outStringShort, 0x32, "NoSpace");
                return;
            
            case 33:
                snprintf(outStringLong, 0x64, "kIOReturnPortExists");
                snprintf(outStringShort, 0x32, "PortExists");
                return;
            
            case 34:
                snprintf(outStringLong, 0x64, "kIOReturnCannotWire");
                snprintf(outStringShort, 0x32, "CannotWire");
                return;
            
            case 35:
                snprintf(outStringLong, 0x64, "kIOReturnNoInterrupt");
                snprintf(outStringShort, 0x32, "NoInterrupt");
                return;
            
            case 36:
                snprintf(outStringLong, 0x64, "kIOReturnNoFrames");
                snprintf(outStringShort, 0x32, "NoFrames");
                return;
            
            case 37:
                snprintf(outStringLong, 0x64, "kIOReturnMessageTooLarge");
                snprintf(outStringShort, 0x32, "MessageTooLarge");
                return;
            
            case 38:
                snprintf(outStringLong, 0x64, "kIOReturnNotPermitted");
                snprintf(outStringShort, 0x32, "NotPermitted");
                return;
            
            case 39:
                snprintf(outStringLong, 0x64, "kIOReturnNoPower");
                snprintf(outStringShort, 0x32, "NoPower");
                return;
            
            case 40:
                snprintf(outStringLong, 0x64, "kIOReturnNoMedia");
                snprintf(outStringShort, 0x32, "NoMedia");
                return;
            
            case 41:
                snprintf(outStringLong, 0x64, "kIOReturnUnformattedMedia");
                snprintf(outStringShort, 0x32, "UnformattedMedia");
                return;
            
            case 42:
                snprintf(outStringLong, 0x64, "kIOReturnUnsupportedMode");
                snprintf(outStringShort, 0x32, "UnsupportedMode");
                return;
            
            case 43:
                snprintf(outStringLong, 0x64, "kIOReturnUnderrun");
                snprintf(outStringShort, 0x32, "Underrun");
                return;
            
            case 44:
                snprintf(outStringLong, 0x64, "kIOReturnOverrun");
                snprintf(outStringShort, 0x32, "Overrun");
                return;
            
            case 45:
                snprintf(outStringLong, 0x64, "kIOReturnDeviceError");
                snprintf(outStringShort, 0x32, "DeviceError");
                return;
            
            case 46:
                snprintf(outStringLong, 0x64, "kIOReturnNoCompletion");
                snprintf(outStringShort, 0x32, "NoCompletion");
                return;
            
            case 47:
                snprintf(outStringLong, 0x64, "kIOReturnAborted");
                snprintf(outStringShort, 0x32, "Aborted");
                return;
            
            case 48:
                snprintf(outStringLong, 0x64, "kIOReturnNoBandwidth");
                snprintf(outStringShort, 0x32, "NoBandwidth");
                return;
            
            case 49:
                snprintf(outStringLong, 0x64, "kIOReturnNotResponding");
                snprintf(outStringShort, 0x32, "NotResponding");
                return;
            
            case 50:
                snprintf(outStringLong, 0x64, "kIOReturnIsoTooOld");
                snprintf(outStringShort, 0x32, "IsoTooOld");
                return;
            
            case 51:
                snprintf(outStringLong, 0x64, "kIOReturnIsoTooNew");
                snprintf(outStringShort, 0x32, "IsoTooNew");
                return;
            
            case 52:
                snprintf(outStringLong, 0x64, "kIOReturnNotFound");
                snprintf(outStringShort, 0x32, "NotFound");
                return;
                
            default:
                goto UNKNOWN;
        }
    }
    else
    {
        switch ( errorCode )
        {
            case 0:
                snprintf(outStringLong, 0x64, "kIOReturnSuccess");
                snprintf(outStringShort, 0x32, "Success");
                return;
        
            case 1:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorUnknownHCICommand");
                snprintf(outStringShort, 0x32, "UnknownHCICommand");
                return;
            
            case 2:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorNoConnection");
                snprintf(outStringShort, 0x32, "NoConnection");
                return;
            
            case 3:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorHardwareFailure");
                snprintf(outStringShort, 0x32, "HardwareFailure");
                return;
            
            case 4:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorPageTimeout");
                snprintf(outStringShort, 0x32, "PageTimeout");
                return;
            
            case 5:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorAuthenticationFailure");
                snprintf(outStringShort, 0x32, "AuthenticationFailure");
                return;
            
            case 6:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorKeyMissing");
                snprintf(outStringShort, 0x32, "KeyMissing");
                return;
            
            case 7:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorMemoryFull");
                snprintf(outStringShort, 0x32, "MemoryFull");
                return;
            
            case 8:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorConnectionTimeout");
                snprintf(outStringShort, 0x32, "ConnectionTimeout");
                return;
            
            case 9:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorMaxNumberOfConnections");
                snprintf(outStringShort, 0x32, "MaxNumberOfConnections");
                return;
            
            case 10:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorMaxNumberOfSCOConnectionsToADevice");
                snprintf(outStringShort, 0x32, "MaxNumberOfSCOConnectionsToADevice");
                return;
            
            case 11:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorACLConnectionAlreadyExists");
                snprintf(outStringShort, 0x32, "ACLConnectionAlreadyExists");
                return;
            
            case 12:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorCommandDisallowed");
                snprintf(outStringShort, 0x32, "CommandDisallowed");
                return;
            
            case 13:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorHostRejectedLimitedResources");
                snprintf(outStringShort, 0x32, "HostRejectedLimitedResources");
                return;
            
            case 14:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorHostRejectedSecurityReasons");
                snprintf(outStringShort, 0x32, "HostRejectedSecurityReasons");
                return;
            
            case 15:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorHostRejectedUnacceptableDeviceAddress");
                snprintf(outStringShort, 0x32, "HostRejectedUnacceptableDeviceAddress");
                return;
            
            case 16:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorHostTimeout");
                snprintf(outStringShort, 0x32, "HostTimeout");
                return;
            
            case 17:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorUnsupportedFeatureOrParameterValue");
                snprintf(outStringShort, 0x32, "UnsupportedFeatureOrParameterValue");
                return;
                
            case 18:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorInvalidHCICommandParameters");
                snprintf(outStringShort, 0x32, "InvalidHCICommandParameters");
                return;
                
            case 19:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorOtherEndTerminatedConnectionUserEnded");
                snprintf(outStringShort, 0x32, "OtherEndTerminatedConnectionUserEnded");
                return;
                
            case 20:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorOtherEndTerminatedConnectionLowResources");
                snprintf(outStringShort, 0x32, "OtherEndTerminatedConnectionLowResources");
                return;
                
            case 21:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorOtherEndTerminatedConnectionAboutToPowerOff");
                snprintf(outStringShort, 0x32, "OtherEndTerminatedConnectionAboutToPowerOff");
                return;
                
            case 22:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorConnectionTerminatedByLocalHost");
                snprintf(outStringShort, 0x32, "ConnectionTerminatedByLocalHost");
                return;
                
            case 23:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorRepeatedAttempts");
                snprintf(outStringShort, 0x32, "RepeatedAttempts");
                return;
                
            case 24:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorPairingNotAllowed");
                snprintf(outStringShort, 0x32, "PairingNotAllowed");
                return;
                
            case 25:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorUnknownLMPPDU");
                snprintf(outStringShort, 0x32, "UnknownLMPPDU");
                return;
                
            case 26:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorUnsupportedRemoteFeature");
                snprintf(outStringShort, 0x32, "UnsupportedRemoteFeature");
                return;
                
            case 27:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorSCOOffsetRejected");
                snprintf(outStringShort, 0x32, "SCOOffsetRejected");
                return;
                
            case 28:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorSCOIntervalRejected");
                snprintf(outStringShort, 0x32, "SCOIntervalRejected");
                return;
                
            case 29:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorSCOAirModeRejected");
                snprintf(outStringShort, 0x32, "SCOAirModeRejected");
                return;
                
            case 30:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorInvalidLMPParameters");
                snprintf(outStringShort, 0x32, "InvalidLMPParameters");
                return;
                
            case 31:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorUnspecifiedError");
                snprintf(outStringShort, 0x32, "UnspecifiedError");
                return;
                
            case 32:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorUnsupportedLMPParameterValue");
                snprintf(outStringShort, 0x32, "UnsupportedLMPParameterValue");
                return;
                
            case 33:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorRoleChangeNotAllowed");
                snprintf(outStringShort, 0x32, "RoleChangeNotAllowed");
                return;
                
            case 34:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorLMPResponseTimeout");
                snprintf(outStringShort, 0x32, "LMPResponseTimeout");
                return;
                
            case 35:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorLMPErrorTransactionCollision");
                snprintf(outStringShort, 0x32, "LMPErrorTransactionCollision");
                return;
                
            case 36:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorLMPPDUNotAllowed");
                snprintf(outStringShort, 0x32, "LMPPDUNotAllowed");
                return;
                
            case 37:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorEncryptionModeNotAcceptable");
                snprintf(outStringShort, 0x32, "EncryptionModeNotAcceptable");
                return;
                
            case 38:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorUnitKeyUsed");
                snprintf(outStringShort, 0x32, "UnitKeyUsed");
                return;
                
            case 39:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorQoSNotSupported");
                snprintf(outStringShort, 0x32, "QoSNotSupported");
                return;
                
            case 40:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorInstantPassed");
                snprintf(outStringShort, 0x32, "InstantPassed");
                return;
                
            case 41:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorPairingWithUnitKeyNotSupported");
                snprintf(outStringShort, 0x32, "PairingWithUnitKeyNotSupported");
                return;
                
            case 42:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorDifferentTransactionCollision");
                snprintf(outStringShort, 0x32, "DifferentTransactionCollision");
                return;
                
            case 44:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorQoSUnacceptableParameter");
                snprintf(outStringShort, 0x32, "QoSUnacceptableParameter");
                return;
            
            case 45:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorQoSRejected");
                snprintf(outStringShort, 0x32, "QoSRejected");
                return;
                
            case 46:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorChannelClassificationNotSupported");
                snprintf(outStringShort, 0x32, "ChannelClassificationNotSupported");
                return;
            
            case 47:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorInsufficientSecurity");
                snprintf(outStringShort, 0x32, "InsufficientSecurity");
                return;
            
            case 48:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorParameterOutOfMandatoryRange");
                snprintf(outStringShort, 0x32, "ParameterOutOfMandatoryRange");
                return;
            
            case 49:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorRoleSwitchPending");
                snprintf(outStringShort, 0x32, "RoleSwitchPending");
                return;
            
            case 52:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorReservedSlotViolation");
                snprintf(outStringShort, 0x32, "ReservedSlotViolation");
                return;
            
            case 53:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorRoleSwitchFailed");
                snprintf(outStringShort, 0x32, "RoleSwitchFailed");
                return;
            
            case 54:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorExtendedInquiryResponseTooLarge");
                snprintf(outStringShort, 0x32, "ExtendedInquiryResponseTooLarge");
                return;
            
            case 55:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorSecureSimplePairingNotSupportedByHost");
                snprintf(outStringShort, 0x32, "SecureSimplePairingNotSupportedByHost");
                return;
            
            case 56:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorHostBusyPairing");
                snprintf(outStringShort, 0x32, "HostBusyPairing");
                return;
                
            case 57:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorConnectionRejectedDueToNoSuitableChannelFound");
                snprintf(outStringShort, 0x32, "ConnectionRejectedDueToNoSuitableChannelFound");
                return;
            
            case 58:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorControllerBusy");
                snprintf(outStringShort, 0x32, "ControllerBusy");
                return;
            
            case 59:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorUnacceptableConnectionInterval");
                snprintf(outStringShort, 0x32, "UnacceptableConnectionInterval");
                return;
            
            case 60:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorDirectedAdvertisingTimeout");
                snprintf(outStringShort, 0x32, "DirectedAdvertisingTimeout");
                return;
                
            case 61:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorConnectionTerminatedDueToMICFailure");
                snprintf(outStringShort, 0x32, "ConnectionTerminatedDueToMICFailure");
                return;
            
            case 62:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorConnectionFailedToBeEstablished");
                snprintf(outStringShort, 0x32, "ConnectionFailedToBeEstablished");
                return;
            
            case 63:
                snprintf(outStringLong, 0x64, "kBluetoothHCIErrorMACConnectionFailed");
                snprintf(outStringShort, 0x32, "MACConnectionFailed");
                return;
                    
            case 65281:
                snprintf(outStringLong, 0x64, "kBluetoothSyncHCIRequestTimedOutAfterSent");
                snprintf(outStringShort, 0x32, "SyncHCIRequestTimedOutAfterSent");
                return;
                  
            case 65282:
                snprintf(outStringLong, 0x64, "kBluetoothSyncHCIRequestTimedOutWaitingToBeSent");
                snprintf(outStringShort, 0x32, "SyncHCIRequestTimedOutWaitingToBeSent");
                return;
                  
            case 65283:
                snprintf(outStringLong, 0x64, "kBluetoothControllerTransportInactive");
                snprintf(outStringShort, 0x32, "ControllerTransportInactive");
                return;
                
            case 65284:
                snprintf(outStringLong, 0x64, "kBluetoothControllerTransportSuspended");
                snprintf(outStringShort, 0x32, "ControllerTransportSuspended");
                return;
        }
UNKNOWN:
        snprintf(outStringLong, 0x64, "0x%x -- Unknown", errorCode);
        snprintf(outStringShort, 0x32, "0x%x -- Unknown", errorCode);
    }
}

OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 0)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 1)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 2)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 3)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 4)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 5)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 6)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 7)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 8)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 9)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 10)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 11)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 12)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 13)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 14)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 15)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 16)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 17)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 18)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 19)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 20)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 21)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 22)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 23)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 24)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 25)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 26)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 27)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 28)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 29)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 30)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 31)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 32)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 33)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 34)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 35)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 36)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 37)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 38)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 39)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 40)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 41)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 42)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 43)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 44)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 45)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 46)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 47)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 48)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 49)
OSMetaClassDefineReservedUnused(IOBluetoothHCIController, 50)

void IOBluetoothHCIController::USBHardResetWL()
{
    IOReturn result;
    IOReturn error;
    const IORegistryPlane * usbPlane;
    IOUSBHostDevice * myHub;
    char errStringShort;
    char errStringLong;
    
    os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- entering  ****");
    
    mUSBHardResetWLCallTime = GetCurrentTime();
    *(UInt8 *)(this + 453) = 0;
    
    if ( !mActiveBluetoothHardware || !mActiveBluetoothHardware->mBluetoothHostController )
    {
        ReleaseAllHardResetObjects();
        os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- Either mActiveBluetoothHardware or mActiveBluetoothHardware->mBluetoothHostController is NULL -- cannot re-enumerate ****");
        os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- exiting  ****");
        return;
    }
    
    *(UInt8 *)(mActiveBluetoothHardware->mBluetoothHostController + 966) = 0;
    *(UInt8 *)(mActiveBluetoothHardware->mBluetoothHostController + 969) = 1;
    if ( mActiveBluetoothHardware->mHardResetCounter > 4 )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- Tried to re-enumerate the Bluetooth module multiple times but still could not bring it back to life ****");
        goto OVER;
    }
    
    ++mActiveBluetoothHardware->mHardResetCounter;
    
    os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- setting mActiveBluetoothHardware->mBluetoothHostController->mTransportGoingAway to TRUE ****");
    mActiveBluetoothHardware->mBluetoothHostController->mTransportIsGoingAway = true;
    
    if ( mActiveBluetoothHardware && mActiveBluetoothHardware->mBluetoothTransport )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- calling mActiveBluetoothHardware->mBluetoothTransport->AbortPipesAndClose( TRUE, TRUE ) ****");
        mActiveBluetoothHardware->mBluetoothTransport->AbortPipesAndClose(true, true);
    }
    
    if ( mACPIMethods && mACPIMethods->mResetMethodAvailable )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- mACPIMethods->calling SetBTRS() to toggle the GPIO for Bluetooth module -- counter = %d ****", mActiveBluetoothHardware->mHardResetCounter);
        error = mACPIMethods->SetBTRS();
        if ( error )
        {
            ConvertErrorCodeToString(error, &errStringLong, &errStringShort);
            os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- SetBTRS failed -- error = 0x%4x (%s) ****", error, &errStringLong);
        }
        else
            os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- SetBTRS() successful ****");
        goto OVER;
    }
    
    if ( mHardResetUSBHostDevice )
    {
        if ( mHardResetUSBHostDevice->isInactive() )
        {
            os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- mHardResetUSBHostDevice->isInactive() is true ****");
            goto OVER;
        }
        os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- calling mHardResetUSBHostDevice->reset() -- counter = %d ****", mActiveBluetoothHardware->mHardResetCounter);
        
        result = mHardResetUSBHostDevice->reset();
        ConvertErrorCodeToString(result, &errStringLong, &errStringShort);
        os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- mHardResetUSBHostDevice->reset() -- result = 0x%04X (%s) ****", result, &errStringLong);
        goto OVER;
    }
    
    if ( mHardResetUSBHub )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- calling mHardResetUSBHub->reset() -- counter = %d ****", mActiveBluetoothHardware->mHardResetCounter);
        if ( mHardResetUSBHub->isInactive() )
        {
            os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- mHardResetUSBHub->isInactive() is true ****");
            goto OVER;
        }
        
        result = mHardResetUSBHub->reset();
        ConvertErrorCodeToString(result, &errStringLong, &errStringShort);
        os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- mHardResetUSBHub->reset() -- result = 0x%04X (%s) ****", result, &errStringLong);
        goto OVER;
    }
    
    usbPlane = IORegistryEntry::getPlane("IOUSB");
    if ( !usbPlane )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- usbPlane is NULL ****");
        goto OVER;
    }
    
    myHub = OSDynamicCast(IOUSBHostDevice, mActiveBluetoothHardware->mBluetoothHostController->getParentEntry(usbPlane));
    if ( !myHub )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- no hub found! ****");
        goto OVER;
    }
    
    os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- calling myHub->reset() -- counter = %d ****", mActiveBluetoothHardware->mHardResetCounter);
    BluetoothFamilyLogPacket(this, 248, "Hub reset() - counter = %d", mActiveBluetoothHardware->mHardResetCounter);
    
    result = myHub->reset();
    ConvertErrorCodeToString(result, &errStringLong, &errStringShort);
    os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- myHub->reset() -- result = 0x%04X (%s) ****", result, &errStringLong);

OVER:
    ReleaseAllHardResetObjects();
    os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][USBHardResetWL] -- exiting ****");
}

IOReturn IOBluetoothHCIController::USBHardReset()
{
    UInt64 time;

    clock_interval_to_deadline(10, 1000000, &time);
    if ( thread_call_enter_delayed(mHardResetThreadCall, time))
        return -536870212;
  
    return kIOReturnSuccess;
}

IOReturn IOBluetoothHCIController::StartFullWakeTimer()
{
    IOReturn result;
    char errStringShort;
    char errStringLong;
    
    if ( !mFullWakeTimer || !mActiveBluetoothHardware || !mActiveBluetoothHardware->mBluetoothTransport )
    {
        ConvertErrorCodeToString(-536870212, &errStringLong, &errStringShort);
        return -536870212;
    }
    
    mFullWakeTimerHasTimeout = true;
    if ( (mActiveBluetoothHardware->mBluetoothTransport->mPowerMask & 0xFFFE) == 6 )
        result = mFullWakeTimer->setTimeoutMS(1500);
    else
        result = mFullWakeTimer->setTimeoutMS(1000);
    
    ConvertErrorCodeToString(result, &errStringLong, &errStringShort);
    return result;
}

void IOBluetoothHCIController::CancelFullWakeTimer()
{
    if ( mFullWakeTimer && *(_BYTE *)(this + 416) && mFullWakeTimerHasTimeout )
    {
        mFullWakeTimer->cancelTimeout();
        mFullWakeTimerHasTimeout = false;
    }
}

void IOBluetoothHCIController::WakeUpDisplay()
{
    char * log;
    
    if (!mDisplayManager)
        return;
    
    BluetoothFamilyLogPacket(this, 251, "activityTickle");
    if ( mActivityTickleCallTime )
    {
        log = (char *) IOMalloc(511);
        if ( log )
        {
            bzero(log, 511);
            snprintf(log, 511, "[IOBluetoothHCIController][WakeUpDisplay] -- took %u microseconds to call activityTickle()\n", GetCurrentTime() - mActivityTickleCallTime);
            os_log(mInternalOSLogObject, "%s", log);
            kprintf("%s", log);
            LogPacket(251, log, strlen(log));
            IOFree(log, 511);
      }
        mActivityTickleCallTime = 0;
    }
    
    if ( IOService::getPMRootDomain() )
        IOService::getPMRootDomain()->requestUserActive((IOService *) mDisplayManager, "Bluetooth");
    *(UInt8 *)(this + 416) = 0;
}

UInt32 IOBluetoothHCIController::GetNextBluetoothObjectID()
{
    if ( IOLockTryLock(mBluetoothObjectsLock) )
        goto LABEL_20;
    
    for (UInt8 i = 0; i <= 9; ++i)
    {
        IOSleep(10);
        if (IOLockTryLock(mBluetoothObjectsLock))
        {
LABEL_20:
            for (int j = 0; j < 0xFFFE; ++j)
            {
                if ( !mBluetoothObjects[mCurrentBluetoothObjectID] )
                {
                    mBluetoothObjects[mCurrentBluetoothObjectID] = 1;
                    if ( mCurrentBluetoothObjectID != -2 )
                        ++mCurrentBluetoothObjectID;
                    else
                        mCurrentBluetoothObjectID = 1;
                    IOLockUnlock(mBluetoothObjectsLock);
                    return mCurrentBluetoothObjectID;
                }
                ++mCurrentBluetoothObjectID;
                if ( mCurrentBluetoothObjectID == -1 )
                    mCurrentBluetoothObjectID = 1;
            }
            IOLockUnlock(mBluetoothObjectsLock);
            return 0;
        }
    }
    
    return 0;
    /*
    var1 = -9;
    do
    {
        
        var2 = IOLockTryLock(mBluetoothObjectIDsLock);
        if ( !var1 )
            break;
        ++var1;
    }
    while ( !var2 );
     */
}

IOReturn IOBluetoothHCIController::ProcessBluetoothTransportShowsUpActionWL(IOBluetoothHostControllerTransport * transport)
{
  __int64 v4; // rax
  OSMetaClassBase *v5; // rax
  __int64 v6; // rsi
  const OSMetaClass *v7; // rdx
  __int64 v8; // rax
  __int64 v9; // rdi
  unsigned int v10; // eax
  unsigned int v11; // er14
  unsigned __int16 v12; // r12
  unsigned __int16 v13; // ax
  __int64 v14; // rsi
  IOService *v15; // r12
  __int64 *v16; // rax
  __int64 v17; // r14
  __int64 v18; // rdx
  __int64 v19; // rax
  __int64 v20; // rax
  OSMetaClassBase *v21; // rax
  __int64 v22; // rsi
  const OSMetaClass *v23; // rdx
  __int64 v24; // rax
  char *v25; // rax
  unsigned int v26; // er13
  char *v27; // rbx
  __int64 v28; // r9
  char *v29; // rdi
  __int64 v30; // rax
  _BYTE *v31; // rdi
  __int16 v32; // cx
  __int64 v33; // rax
  __int16 v34; // ax
  __int64 v35; // rcx
  char v36; // al
  __int64 v37; // rcx
  _BYTE *v38; // rdi
  _BYTE *v39; // rdi
  __int64 v40; // r15
  unsigned int v41; // ebx
  unsigned int v42; // eax
  __int64 result; // rax
  __int64 v44; // rsi
  char *v45; // rax
  char *v46; // r15
  unsigned int v47; // ebx
  unsigned int v48; // eax
  __int64 v49; // r9
  size_t v50; // rax
  __int64 v51; // rax
  char *v52; // rax
  char *v53; // r15
  unsigned int v54; // eax
  __int64 v55; // rax
  __int64 v56; // rcx
  char v57; // bl
  __int64 v58; // rax
  _QWORD *v59; // rax
  __int64 v60; // rbx
  _BYTE *v61; // rdi
  _QWORD *v62; // rax
  __int64 v63; // rcx
  __int64 v64; // rcx
  __int64 v65; // rdi
  char *v66; // rax
  char *v67; // rbx
  __int64 v68; // r9
  char *v69; // rax
  char *v70; // rbx
  __int64 v71; // r9
  int v72; // er14
  __int64 v73; // rdi
  unsigned int v74; // eax
  char *v75; // rax
  __int64 v76; // r9
  int v77; // ecx
  unsigned __int16 v78; // r15
  __int64 v79; // rsi
  unsigned int v80; // eax
  char *v81; // rax
  char *v82; // rbx
  __int64 v83; // r9
  char *v84; // rax
  char *v85; // rbx
  unsigned int v86; // er15
  unsigned int v87; // er13
  unsigned int v88; // eax
  __int64 v89; // r9
  char *v90; // rax
  char *v91; // rbx
  __int64 v92; // r9
  signed __int64 v93; // rsi
  char *v94; // rax
  char *v95; // r15
  unsigned int v96; // ST18_4
  unsigned int v97; // ebx
  unsigned int v98; // eax
  __int64 v99; // r9
  char *v100; // rax
  char *v101; // rbx
  unsigned int v102; // ST14_4
  unsigned int v103; // er15
  unsigned int v104; // eax
  __int64 v105; // r9
  __int64 v106; // rax
  char v107; // bl
  char v108; // al
  __int64 v109; // rdi
  int v110; // [rsp+0h] [rbp-170h]
  int v111; // [rsp+4h] [rbp-16Ch]
  _BYTE *v112; // [rsp+8h] [rbp-168h]
  int v113; // [rsp+14h] [rbp-15Ch]
  __int64 v115; // [rsp+20h] [rbp-150h]
  __int64 v116; // [rsp+28h] [rbp-148h]
  __int64 v117; // [rsp+30h] [rbp-140h]
  __int64 v118; // [rsp+38h] [rbp-138h]
  __int64 v119; // [rsp+40h] [rbp-130h]
  __int64 v120; // [rsp+48h] [rbp-128h]
  __int16 v121; // [rsp+50h] [rbp-120h]
  char v122[8]; // [rsp+60h] [rbp-110h]
  __int64 v123; // [rsp+68h] [rbp-108h]
  __int64 v124; // [rsp+70h] [rbp-100h]
  __int64 v125; // [rsp+78h] [rbp-F8h]
  __int64 v126; // [rsp+80h] [rbp-F0h]
  __int64 v127; // [rsp+88h] [rbp-E8h]
  __int64 v128; // [rsp+90h] [rbp-E0h]
  __int64 v129; // [rsp+98h] [rbp-D8h]
  __int64 v130; // [rsp+A0h] [rbp-D0h]
  __int64 v131; // [rsp+A8h] [rbp-C8h]
  __int64 v132; // [rsp+B0h] [rbp-C0h]
  __int64 v133; // [rsp+B8h] [rbp-B8h]
  int v134; // [rsp+C0h] [rbp-B0h]
  __int64 v135; // [rsp+D0h] [rbp-A0h]
  __int64 v136; // [rsp+D8h] [rbp-98h]
  __int64 v137; // [rsp+E0h] [rbp-90h]
  __int64 v138; // [rsp+E8h] [rbp-88h]
  __int64 v139; // [rsp+F0h] [rbp-80h]
  __int64 v140; // [rsp+F8h] [rbp-78h]
  __int64 v141; // [rsp+100h] [rbp-70h]
  __int64 v142; // [rsp+108h] [rbp-68h]
  __int64 v143; // [rsp+110h] [rbp-60h]
  __int64 v144; // [rsp+118h] [rbp-58h]
  __int64 v145; // [rsp+120h] [rbp-50h]
  __int64 v146; // [rsp+128h] [rbp-48h]
  int v147; // [rsp+130h] [rbp-40h]
  __int64 v148; // [rsp+140h] [rbp-30h]

    BluetoothHardwareListType * hardware;
    OSBoolean * interfaceMatched;
    
    snprintf((char *)&v135, 0x64uLL, "IOBluetoothFamily::ProcessBluetoothTransportShowsUpActionWL()");
    if ( !transport )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][ProcessBluetoothTransportShowsUpActionWL] -- the Bluetooth HCI USB Transport is missing ****\n");
        v26 = -536870208;
        goto LABEL_21;
    }
    
    hardware = FindBluetoothHardware(transport);
    if ( hardware )
        *(_BYTE *)(hardware + 16) = 1;
    transport->RetainTransport("IOBluetoothFamily::ProcessBluetoothTransportShowsUpActionWL()");
    interfaceMatched = OSDynamicCast(OSBoolean, transport->getProperty("InterfaceMatched"));
    
    if ( interfaceMatched && interfaceMatched->getValue() )
    {
        if ( !transport->isInactive() )
        {
            transport->ResetBluetoothDevice();
            if ( mSearchForTransportEventTimer )
            {
                mReceivedTransportNotification = 0;
                v10 = mSearchForTransportEventTimer->setTimeoutMS(20000);
                ConvertErrorCodeToString(v10, v122, &v115);
                *(_BYTE *)(this + 224) = 1;
            }
        }
        goto LABEL_26;
    }
    
    if ( !transport->mProvider )
        goto LABEL_26;
    
    *(_BYTE *)(this + 433) = *(_BYTE *)(transport + 296);
    v14 = transport->GetControllerVendorID();
    v15 = (IOService *)this;
    v113 = transport->GetControllerVendorID();
    v110 = transport->GetControllerProductID();
    v16 = FindBluetoothHardware(transport->GetControllerVendorID(), transport->GetControllerProductID(), transport->mLocationID);
    if ( v16 )
    {
        v17 = (__int64)v16;
        *v16 = transport;
        *(_QWORD *)(transport + 144) = v16[1];
        *(_QWORD *)(transport + 160) = *(_QWORD *)(this + 136);
        (*(void (__fastcall **)(__int64, __int64 *))(*(_QWORD *)this + 2472LL))(this, v16);
        goto LABEL_12;
    }
    
  v30 = IOMalloc(40LL);
  if ( !v30 )
  {
LABEL_26:
    (*(void (__fastcall **)(__int64, __int64 *))(*(_QWORD *)transport + 2704LL))(transport, &v135);
LABEL_40:
    v26 = -536870212;
    goto LABEL_41;
  }
  v17 = v30;
  *(_QWORD *)v30 = transport;
  *(_BYTE *)(v30 + 19) = 0;
  *(_QWORD *)(transport + 160) = *(_QWORD *)(this + 136);
  if ( (*(unsigned int (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 2448LL))(this, v30) )
  {
    IOFree(v17, 40LL);
    goto LABEL_26;
  }
  *(_QWORD *)(v17 + 24) = 0LL;
  *(_QWORD *)(v17 + 32) = 0LL;
LABEL_12:
  *(_WORD *)(v17 + 16) = 1;
  *(_BYTE *)(v17 + 18) = 0;
  v18 = *(_QWORD *)(v17 + 8);
  *(_QWORD *)(v18 + 832) = transport;
  *(_WORD *)(v18 + 880) = *(_WORD *)(transport + 176);
  *(_BYTE *)(v18 + 877) = 0;
  *(_BYTE *)(v18 + 885) = 0;
  *(_BYTE *)(v18 + 879) = 0;
  *(_QWORD *)(v18 + 824) = this;
  *(_QWORD *)(*(_QWORD *)v17 + 136LL) = this;
  *(_DWORD *)(v18 + 1276) = 38356;
  v19 = *(_QWORD *)this;
  if ( *(_BYTE *)(v18 + 966) )
  {
    if ( (*(unsigned __int8 (__fastcall **)(__int64))(v19 + 2616))(this) )
    {
      v20 = IOService::getPMRootDomain((IOService *)this);
      if ( !v20 )
        goto LABEL_29;
      v21 = (OSMetaClassBase *)(*(__int64 (__fastcall **)(__int64, const char *))(*(_QWORD *)v20 + 696LL))(
                                 v20,
                                 "IOPMSystemSleepType");
      v22 = OSNumber::metaClass;
      v24 = OSMetaClassBase::safeMetaCast(v21, OSNumber::metaClass, v23);
      if ( !v24
        || (*(unsigned int (__fastcall **)(__int64, __int64))(*(_QWORD *)v24 + 328LL))(v24, v22) != 6
        || (*(unsigned __int8 (__fastcall **)(__int64))(*(_QWORD *)this + 2536LL))(this) )
      {
        goto LABEL_29;
      }
    }
    goto LABEL_28;
  }
  if ( (*(unsigned __int8 (__fastcall **)(__int64))(v19 + 2560))(this) )
LABEL_28:
    *(_BYTE *)(this + 416) = 1;
LABEL_29:
  (*(void (__fastcall **)(_QWORD, __int64 *))(**(_QWORD **)v17 + 2696LL))(*(_QWORD *)v17, &v135);
  v31 = *(_BYTE **)v17;
  v31[266] = 0;
  v112 = v31;
  if ( !(*(unsigned __int8 (__fastcall **)(_BYTE *, __int64, _QWORD))(*(_QWORD *)v31 + 2136LL))(
          v31,
          v17,
          *(_QWORD *)(this + 136)) )
    goto LABEL_168;
  v32 = *(_WORD *)(transport + 176);
  v33 = *(_QWORD *)(v17 + 8);
  *(_WORD *)(v33 + 880) = v32;
  if ( v32 == 7 )
    *(_BYTE *)(v33 + 1307) = 1;
  *(_WORD *)(*(_QWORD *)(v17 + 8) + 858LL) = (*(__int64 (__fastcall **)(__int64))(*(_QWORD *)transport + 2472LL))(transport);
  v34 = (*(__int64 (__fastcall **)(__int64))(*(_QWORD *)transport + 2480LL))(transport);
  v35 = *(_QWORD *)(v17 + 8);
  *(_WORD *)(v35 + 860) = v34;
  *(_WORD *)(v35 + 875) = 0;
  *(_BYTE *)(v35 + 878) = 0;
  *(_BYTE *)(v35 + 882) = 1;
  *(_BYTE *)(v35 + 753) = 0;
  *(_DWORD *)(*(_QWORD *)(v17 + 8) + 864LL) = (*(__int64 (__fastcall **)(__int64))(*(_QWORD *)transport + 2512LL))(transport);
  *(_BYTE *)(*(_QWORD *)(v17 + 8) + 874LL) = (*(__int64 (__fastcall **)(__int64))(*(_QWORD *)transport + 2520LL))(transport);
  v36 = (*(__int64 (__fastcall **)(__int64))(*(_QWORD *)transport + 2528LL))(transport);
  v37 = *(_QWORD *)(v17 + 8);
  *(_BYTE *)(v37 + 887) = v36;
  *(_WORD *)(v37 + 883) = 0;
  *(_BYTE *)(v37 + 889) = 0;
  *(_BYTE *)(v37 + 896) = 1;
  v15 = (IOService *)this;
  v38 = *(_BYTE **)(this + 488);
  if ( v38 && !v38[160] )
  {
    (*(void (**)(void))(*(_QWORD *)v38 + 2136LL))();
    v37 = *(_QWORD *)(v17 + 8);
  }
  if ( !*(_BYTE *)(v37 + 877) )
  {
    *(_QWORD *)v122 = -6148914691236517206LL;
    clock_interval_to_deadline(10000LL, 1000000LL, v122);
    (*(void (__fastcall **)(_QWORD, signed __int64, _QWORD, _QWORD))(**(_QWORD **)(this + 144) + 496LL))(
      *(_QWORD *)(this + 144),
      *(_QWORD *)(v17 + 8) + 877LL,
      *(_QWORD *)v122,
      0LL);
    v37 = *(_QWORD *)(v17 + 8);
  }
  if ( *(_BYTE *)(v37 + 878) || (*(unsigned int (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 2368LL))(this, v17) )
  {
LABEL_168:
    (*(void (__fastcall **)(_QWORD, signed __int64, _QWORD))(**((_QWORD **)v15 + 18) + 488LL))(
      *((_QWORD *)v15 + 18),
      *(_QWORD *)(v17 + 8) + 879LL,
      0LL);
    v39 = *(_BYTE **)v17;
    v39[266] = 1;
    (*(void (__fastcall **)(_BYTE *, __int64 *))(*(_QWORD *)v39 + 2704LL))(v39, &v135);
    IOFree(v17, 40LL);
    (*(void (__fastcall **)(IOService *, IOService *))(*(_QWORD *)v15 + 2600LL))(v15, v15);
    (*(void (__fastcall **)(IOService *, _QWORD))(*(_QWORD *)v15 + 2600LL))(v15, *(_QWORD *)v17);
    v40 = *((_QWORD *)v15 + 53);
    v41 = (*(__int64 (__fastcall **)(IOService *, IOService *))(*(_QWORD *)v15 + 2600LL))(v15, v15);
    v42 = (*(__int64 (__fastcall **)(IOService *, _QWORD))(*(_QWORD *)v15 + 2600LL))(v15, *(_QWORD *)v17);
    _os_log_internal(
      &dword_0,
      v40,
      0LL,
                     "**** [IOBluetoothFamily][ProcessBluetoothTransportShowsUpActionWL] -- Error!! -- Something went wrong in the setup process. Cannot add the transport object -- 0x%04x -- 0x%04x ****",
      v41,
      v42);
    goto LABEL_40;
  }
  if ( *(_BYTE *)(*(_QWORD *)(v17 + 8) + 878LL)
    || !*(_QWORD *)v17
    || !(*(unsigned __int8 (**)(void))(**(_QWORD **)v17 + 2152LL))() )
  {
    goto LABEL_45;
  }
  if ( *(_QWORD *)(this + 304) || *(_WORD *)(this + 348) != 1452 )
    goto LABEL_60;
  v51 = *(_QWORD *)(v17 + 8);
  if ( (*(_WORD *)(v51 + 880) & 0xFFFE) != 6 )
  {
    if ( *(_WORD *)(v51 + 858) != 1452
      || *(_WORD *)(v51 + 860) != *(_WORD *)(this + 346)
      || *(_DWORD *)(v51 + 864) != *(_DWORD *)(this + 356) )
    {
      v52 = (char *)IOMalloc(511LL);
      if ( v52 )
      {
        v53 = v52;
        bzero(v52, 0x1FFuLL);
        snprintf(
          v53,
          0x1FFuLL,
          "**** [IOBluetoothFamily][ProcessBluetoothTransportShowsUpActionWL] -- So let's wait for the internal controlle"
          "r to show up ****\n");
        kprintf("%s", v53);
        IOFree(v53, 511LL);
      }
      v54 = (*(__int64 (__fastcall **)(__int64, __int64, signed __int64, __int64 *, signed __int64))(*(_QWORD *)this + 2584LL))(
              this,
              this + 384,
              10000LL,
              &v135,
              1LL);
      if ( v54 >= 2 )
      {
        v133 = -6148914691236517206LL;
        v132 = -6148914691236517206LL;
        v131 = -6148914691236517206LL;
        v130 = -6148914691236517206LL;
        v129 = -6148914691236517206LL;
        v128 = -6148914691236517206LL;
        v127 = -6148914691236517206LL;
        v126 = -6148914691236517206LL;
        v125 = -6148914691236517206LL;
        v124 = -6148914691236517206LL;
        v123 = -6148914691236517206LL;
        *(_QWORD *)v122 = -6148914691236517206LL;
        v134 = -1431655766;
        v120 = -6148914691236517206LL;
        v119 = -6148914691236517206LL;
        v118 = -6148914691236517206LL;
        v117 = -6148914691236517206LL;
        v116 = -6148914691236517206LL;
        v115 = -6148914691236517206LL;
        v121 = -21846;
        (*(void (__fastcall **)(__int64, _QWORD))(*(_QWORD *)this + 2848LL))(this, v54);
      }
    }
LABEL_60:
    v51 = *(_QWORD *)(v17 + 8);
  }
  if ( v51 && *(_BYTE *)(v51 + 878) )
    goto LABEL_45;
  if ( !*(_QWORD *)(this + 304) )
  {
    *(_QWORD *)(this + 304) = v17;
    (*(void (__fastcall **)(_QWORD, signed __int64))(**(_QWORD **)v17 + 2568LL))(*(_QWORD *)v17, 1LL);
    *(_QWORD *)(this + 312) = v17;
    goto LABEL_86;
  }
  if ( !(*(__int64 (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 2344LL))(this, v17) )
    goto LABEL_45;
  v55 = *(_QWORD *)(v17 + 32);
  if ( v55 )
  {
    v56 = *(_QWORD *)(v55 + 8);
    if ( v56 )
    {
      if ( !*(_BYTE *)(v56 + 879) )
      {
        v77 = 0;
        v78 = 0;
        v57 = 0;
        while ( 1 )
        {
          if ( v55 && (v79 = *(_QWORD *)(v55 + 8)) != 0 )
          {
            v111 = v77;
            v80 = (*(__int64 (__fastcall **)(__int64, __int64, signed __int64, __int64 *, signed __int64))(*(_QWORD *)this + 2584LL))(
                    this,
                    v79 + 879,
                    5000LL,
                    &v135,
                    1LL);
            if ( v80 )
            {
              v77 = v111;
              if ( v80 != 1 )
              {
                v133 = -6148914691236517206LL;
                v132 = -6148914691236517206LL;
                v131 = -6148914691236517206LL;
                v130 = -6148914691236517206LL;
                v129 = -6148914691236517206LL;
                v128 = -6148914691236517206LL;
                v127 = -6148914691236517206LL;
                v126 = -6148914691236517206LL;
                v125 = -6148914691236517206LL;
                v124 = -6148914691236517206LL;
                v123 = -6148914691236517206LL;
                *(_QWORD *)v122 = -6148914691236517206LL;
                v134 = -1431655766;
                v120 = -6148914691236517206LL;
                v119 = -6148914691236517206LL;
                v118 = -6148914691236517206LL;
                v117 = -6148914691236517206LL;
                v116 = -6148914691236517206LL;
                v115 = -6148914691236517206LL;
                v121 = -21846;
                (*(void (__fastcall **)(__int64, _QWORD))(*(_QWORD *)this + 2848LL))(this, v80);
                goto LABEL_45;
              }
              if ( v78 >= 7u )
              {
                v100 = (char *)IOMalloc(511LL);
                if ( v100 )
                {
                  v101 = v100;
                  bzero(v100, 0x1FFuLL);
                  v102 = (*(__int64 (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 2600LL))(this, v17);
                  v103 = (*(__int64 (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 2600LL))(this, this);
                  v104 = (*(__int64 (__fastcall **)(__int64, _QWORD))(*(_QWORD *)this + 2600LL))(this, *(_QWORD *)v17);
                  snprintf(
                    v101,
                    0x1FFuLL,
                    "**** [IOBluetoothFamily][ProcessBluetoothTransportShowsUpActionWL] -- BluetoothHardware = 0x%04x -- "
                    "ERROR!! Something wrong with the second Bluetooth controller's setup process causing this controller"
                    "'s setup to fail -- 0x%04x -- 0x%04x ****\n",
                    v102,
                    v103,
                    v104);
                  _os_log_internal(
                    &dword_0,
                    *(_QWORD *)(this + 424),
                    0LL,
                    IOBluetoothHCIController::ProcessBluetoothTransportShowsUpActionWL(IOBluetoothHostControllerTransport *)::_os_log_fmt,
                    v101,
                    v105);
                  IOFree(v101, 511LL);
                }
                goto LABEL_45;
              }
            }
            else
            {
              LOBYTE(v77) = 1;
            }
          }
          else
          {
            v57 = 1;
          }
          if ( v78 > 6u || ((unsigned __int8)v57 | (unsigned __int8)v77) & 1 )
            goto LABEL_69;
          ++v78;
          v55 = *(_QWORD *)(v17 + 32);
        }
      }
    }
  }
  v57 = 0;
LABEL_69:
  if ( !(*(__int64 (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 2344LL))(this, v17) )
  {
LABEL_45:
    v44 = *(_QWORD *)(v17 + 8);
    *(_BYTE *)(v44 + 879) = 1;
    (*(void (__fastcall **)(_QWORD, __int64, _QWORD))(**((_QWORD **)v15 + 18) + 488LL))(
      *((_QWORD *)v15 + 18),
      v44 + 879,
      0LL);
    v112[266] = 1;
    (*(void (__fastcall **)(_BYTE *, __int64 *))(*(_QWORD *)v112 + 2704LL))(v112, &v135);
    if ( *((_QWORD *)v15 + 38) == v17 )
    {
      v59 = (_QWORD *)(*(__int64 (__fastcall **)(IOService *))(*(_QWORD *)v15 + 2360LL))(v15);
      *((_QWORD *)v15 + 38) = v59;
      if ( v59 )
      {
        if ( v59 != (_QWORD *)v17 )
        {
          if ( *v59 )
          {
            v133 = -6148914691236517206LL;
            v132 = -6148914691236517206LL;
            v131 = -6148914691236517206LL;
            v130 = -6148914691236517206LL;
            v129 = -6148914691236517206LL;
            v128 = -6148914691236517206LL;
            v127 = -6148914691236517206LL;
            v126 = -6148914691236517206LL;
            v125 = -6148914691236517206LL;
            v124 = -6148914691236517206LL;
            v123 = -6148914691236517206LL;
            *(_QWORD *)v122 = -6148914691236517206LL;
            v134 = -1431655766;
            v120 = -6148914691236517206LL;
            v119 = -6148914691236517206LL;
            v118 = -6148914691236517206LL;
            v117 = -6148914691236517206LL;
            v116 = -6148914691236517206LL;
            v115 = -6148914691236517206LL;
            v121 = -21846;
            (*(void (__fastcall **)(_QWORD, signed __int64))(*(_QWORD *)*v59 + 2568LL))(*v59, 1LL);
            if ( !(*(unsigned int (__fastcall **)(_QWORD, signed __int64, __int64 *))(**(_QWORD **)(*((_QWORD *)v15 + 38)
                                                                                                  + 8LL)
                                                                                    + 3736LL))(
                    *(_QWORD *)(*((_QWORD *)v15 + 38) + 8LL),
                    1LL,
                    &v135) )
            {
              v60 = (*(unsigned int (__fastcall **)(_QWORD, signed __int64, __int64 *))(**(_QWORD **)(*((_QWORD *)v15 + 38) + 8LL)
                                                                                      + 3808LL))(
                      *(_QWORD *)(*((_QWORD *)v15 + 38) + 8LL),
                      1LL,
                      &v135);
              (*(void (__fastcall **)(IOService *, __int64, char *, __int64 *))(*(_QWORD *)v15 + 2848LL))(
                v15,
                v60,
                v122,
                &v115);
              if ( !(_DWORD)v60 )
              {
                if ( (*(unsigned __int8 (__fastcall **)(_QWORD, bool))(**(_QWORD **)(*((_QWORD *)v15 + 38) + 8LL)
                                                                     + 2392LL))(
                       *(_QWORD *)(*((_QWORD *)v15 + 38) + 8LL),
                       *(_BYTE *)(*(_QWORD *)(*((_QWORD *)v15 + 38) + 8LL) + 875LL) == 0) )
                {
                  if ( *((_BYTE *)v15 + 381) )
                  {
                    v65 = *(_QWORD *)(*((_QWORD *)v15 + 38) + 8LL);
                    (*(void (__fastcall **)(IOService *, _QWORD, _QWORD, __int64, _QWORD, _QWORD))(*(_QWORD *)v15
                                                                                                 + 2408LL))(
                      v15,
                      *(unsigned __int16 *)(v65 + 860),
                      *(unsigned __int16 *)(v65 + 858),
                      v65 + 868,
                      *(unsigned int *)(v65 + 864),
                      *(unsigned __int16 *)(v65 + 928));
                    (*(void (__fastcall **)(IOService *, signed __int64, signed __int64, signed __int64, signed __int64, signed __int64))(*(_QWORD *)v15 + 2416LL))(
                      v15,
                      (signed __int64)v15 + 346,
                      (signed __int64)v15 + 348,
                      (signed __int64)v15 + 350,
                      (signed __int64)v15 + 356,
                      (signed __int64)v15 + 360);
                  }
                  (*(void (__fastcall **)(IOService *, _QWORD, char *, __int64 *))(*(_QWORD *)v15 + 2848LL))(
                    v15,
                    0LL,
                    v122,
                    &v115);
                  v66 = (char *)IOMalloc(511LL);
                  if ( v66 )
                  {
                    v67 = v66;
                    bzero(v66, 0x1FFuLL);
                    snprintf(
                      v67,
                      0x1FFuLL,
                      "**** [IOBluetoothFamily][ProcessBluetoothTransportShowsUpActionWL] -- calling ControllerSetupCompl"
                      "ete (0x%04X (%s)) ****\n",
                      0LL,
                      v122);
                    _os_log_internal(
                      &dword_0,
                      *((_QWORD *)v15 + 53),
                      0LL,
                      IOBluetoothHCIController::ProcessBluetoothTransportShowsUpActionWL(IOBluetoothHostControllerTransport *)::_os_log_fmt,
                      v67,
                      v68);
                    IOFree(v67, 511LL);
                  }
                  (*(void (__fastcall **)(_QWORD, _QWORD))(**(_QWORD **)(*((_QWORD *)v15 + 38) + 8LL) + 2376LL))(
                    *(_QWORD *)(*((_QWORD *)v15 + 38) + 8LL),
                    0LL);
                }
              }
            }
          }
        }
      }
    }
    v45 = (char *)IOMalloc(511LL);
    v26 = -536870212;
    if ( v45 )
    {
      v46 = v45;
      bzero(v45, 0x1FFuLL);
      v47 = (*(__int64 (__fastcall **)(IOService *, IOService *))(*(_QWORD *)v15 + 2600LL))(v15, v15);
      v48 = (*(__int64 (__fastcall **)(IOService *, _QWORD))(*(_QWORD *)v15 + 2600LL))(v15, *(_QWORD *)v17);
      snprintf(
        v46,
        0x1FFuLL,
        "**** [IOBluetoothFamily][ProcessBluetoothTransportShowsUpActionWL] -- Error!! -- Something went wrong in the set"
        "up process. Could not communicate with Bluetooth Transport successfully -- 0x%04x -- 0x%04x ****\n"
        "\n",
        v47,
        v48);
      _os_log_internal(
        &dword_0,
        *((_QWORD *)v15 + 53),
        0LL,
        IOBluetoothHCIController::ProcessBluetoothTransportShowsUpActionWL(IOBluetoothHostControllerTransport *)::_os_log_fmt,
        v46,
        v49);
      v50 = strlen(v46);
      (*(void (__fastcall **)(IOService *, signed __int64, char *, size_t))(*(_QWORD *)v15 + 2248LL))(
        v15,
        249LL,
        v46,
        v50);
      v29 = v46;
      goto LABEL_22;
    }
    goto LABEL_41;
  }
  if ( v57 & 1 && *(_QWORD *)(this + 304) == v17 )
  {
    (*(void (__fastcall **)(__int64, signed __int64, _QWORD, _QWORD))(*(_QWORD *)this + 1856LL))(
      this,
      3758227465LL,
      0LL,
      0LL);
LABEL_86:
    if ( !(*(unsigned __int8 (__fastcall **)(_QWORD, signed __int64))(**(_QWORD **)(v17 + 8) + 2392LL))(
            *(_QWORD *)(v17 + 8),
            1LL) )
      goto LABEL_45;
    goto LABEL_91;
  }
  if ( (*(unsigned __int8 (__fastcall **)(__int64, __int64, __int64))(*(_QWORD *)this + 2256LL))(this, this, v17) )
  {
    if ( (*(unsigned int (__fastcall **)(_QWORD, _QWORD, __int64 *))(**(_QWORD **)(*(_QWORD *)(this + 304) + 8LL)
                                                                   + 3736LL))(
           *(_QWORD *)(*(_QWORD *)(this + 304) + 8LL),
           0LL,
           &v135)
      || (*(unsigned int (__fastcall **)(_QWORD, _QWORD, __int64 *))(**(_QWORD **)(*(_QWORD *)(this + 304) + 8LL)
                                                                   + 3808LL))(
           *(_QWORD *)(*(_QWORD *)(this + 304) + 8LL),
           0LL,
           &v135) )
    {
      goto LABEL_45;
    }
    v58 = *(_QWORD *)(v17 + 8);
    if ( v58
      && (*(_BYTE *)(v58 + 878) || !(*(__int64 (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 2344LL))(this, v17)) )
    {
      v15 = (IOService *)this;
      if ( !(*(unsigned int (__fastcall **)(_QWORD, signed __int64, __int64 *))(**(_QWORD **)(*(_QWORD *)(this + 304)
                                                                                            + 8LL)
                                                                              + 3736LL))(
              *(_QWORD *)(*(_QWORD *)(this + 304) + 8LL),
              1LL,
              &v135) )
        (*(void (__fastcall **)(_QWORD, signed __int64, __int64 *))(**(_QWORD **)(*(_QWORD *)(this + 304) + 8LL) + 3808LL))(
          *(_QWORD *)(*(_QWORD *)(this + 304) + 8LL),
          1LL,
          &v135);
      goto LABEL_45;
    }
    v15 = (IOService *)this;
    (*(void (__fastcall **)(_QWORD, _QWORD))(***(_QWORD ***)(this + 304) + 2568LL))(**(_QWORD **)(this + 304), 0LL);
    *(_QWORD *)(this + 304) = v17;
    (*(void (__fastcall **)(_QWORD, signed __int64))(**(_QWORD **)v17 + 2568LL))(*(_QWORD *)v17, 1LL);
    *(_QWORD *)(this + 312) = v17;
    v106 = *(_QWORD *)(*(_QWORD *)(this + 304) + 8LL);
    v107 = *(_BYTE *)(v106 + 957);
    *(_BYTE *)(v106 + 957) = 0;
    (*(void (__fastcall **)(IOService *, signed __int64, _QWORD, _QWORD))(*(_QWORD *)v15 + 1856LL))(
      v15,
      3758227465LL,
      0LL,
      0LL);
    v108 = (*(__int64 (__fastcall **)(_QWORD, signed __int64))(**(_QWORD **)(v17 + 8) + 2392LL))(
             *(_QWORD *)(v17 + 8),
             1LL);
    v109 = *(_QWORD *)(*(_QWORD *)(this + 304) + 8LL);
    *(_BYTE *)(v109 + 957) = v107;
    if ( !v108 )
      goto LABEL_45;
    if ( *(_BYTE *)(v109 + 868) != *(_BYTE *)(this + 350)
      || *(_BYTE *)(v109 + 869) != *(_BYTE *)(this + 351)
      || *(_BYTE *)(v109 + 870) != *(_BYTE *)(this + 352)
      || *(_BYTE *)(v109 + 871) != *(_BYTE *)(this + 353)
      || *(_BYTE *)(v109 + 872) != *(_BYTE *)(this + 354)
      || *(_BYTE *)(v109 + 873) != *(_BYTE *)(this + 355) )
    {
      (*(void (__fastcall **)(__int64, _QWORD, _QWORD, __int64, _QWORD, _QWORD))(*(_QWORD *)this + 2408LL))(
        this,
        *(unsigned __int16 *)(v109 + 860),
        *(unsigned __int16 *)(v109 + 858),
        v109 + 868,
        *(unsigned int *)(v109 + 864),
        *(unsigned __int16 *)(v109 + 928));
      (*(void (__fastcall **)(__int64, __int64, __int64, __int64, __int64, __int64))(*(_QWORD *)this + 2416LL))(
        this,
        this + 346,
        this + 348,
        this + 350,
        this + 356,
        this + 360);
    }
  }
  else
  {
    v133 = -6148914691236517206LL;
    v132 = -6148914691236517206LL;
    v131 = -6148914691236517206LL;
    v130 = -6148914691236517206LL;
    v129 = -6148914691236517206LL;
    v128 = -6148914691236517206LL;
    v127 = -6148914691236517206LL;
    v126 = -6148914691236517206LL;
    v125 = -6148914691236517206LL;
    v124 = -6148914691236517206LL;
    v123 = -6148914691236517206LL;
    *(_QWORD *)v122 = -6148914691236517206LL;
    v134 = -1431655766;
    snprintf(v122, 0x64uLL, "IOBluetoothFamily::ProcessBluetoothTransportShowsUpActionWL()");
    *(_BYTE *)(*(_QWORD *)(v17 + 8) + 879LL) = 1;
    if ( !*(_BYTE *)(this + 381) )
      *(_BYTE *)(this + 381) = (*(__int64 (__fastcall **)(__int64))(*(_QWORD *)this + 2576LL))(this);
    (*(void (__fastcall **)(_QWORD, _QWORD))(**(_QWORD **)v17 + 2568LL))(*(_QWORD *)v17, 0LL);
    (*(void (__fastcall **)(_QWORD, _QWORD, char *))(**(_QWORD **)v17 + 2352LL))(*(_QWORD *)v17, 0LL, v122);
  }
LABEL_91:
  *(_BYTE *)(**((_QWORD **)v15 + 38) + 266LL) = 1;
  (*(void (__fastcall **)(IOService *, __int64 *))(*(_QWORD *)v15 + 2680LL))(v15, &v135);
  (*(void (__fastcall **)(_QWORD, signed __int64, _QWORD))(**((_QWORD **)v15 + 18) + 488LL))(
    *((_QWORD *)v15 + 18),
    *(_QWORD *)(v17 + 8) + 879LL,
    0LL);
  if ( *((_BYTE *)v15 + 380) )
  {
    v61 = v112;
    if ( *((_QWORD *)v15 + 40) )
      goto LABEL_116;
    v62 = (_QWORD *)*((_QWORD *)v15 + 38);
    v63 = v62[1];
    if ( *(_WORD *)(v63 + 858) == *((_WORD *)v15 + 183)
      && *(_WORD *)(v63 + 860) == *((_WORD *)v15 + 182)
      && *(_DWORD *)(v63 + 864) == *((_DWORD *)v15 + 94)
      && *(_BYTE *)(v63 + 868) == *((_BYTE *)v15 + 368)
      && *(_BYTE *)(v63 + 869) == *((_BYTE *)v15 + 369)
      && *(_BYTE *)(v63 + 870) == *((_BYTE *)v15 + 370)
      && *(_BYTE *)(v63 + 871) == *((_BYTE *)v15 + 371)
      && *(_BYTE *)(v63 + 872) == *((_BYTE *)v15 + 372)
      && *(_BYTE *)(v63 + 873) == *((_BYTE *)v15 + 373) )
    {
      goto LABEL_115;
    }
  }
  else
  {
    v64 = *(_QWORD *)(*((_QWORD *)v15 + 38) + 8LL);
    if ( *(_WORD *)(v64 + 858) == 1452 )
    {
      (*(void (__fastcall **)(IOService *, _QWORD, signed __int64, __int64, _QWORD))(*(_QWORD *)v15 + 2432LL))(
        v15,
        *(unsigned __int16 *)(v64 + 860),
        1452LL,
        v64 + 868,
        *(unsigned int *)(v64 + 864));
      (*(void (__fastcall **)(IOService *, signed __int64, signed __int64, signed __int64, signed __int64))(*(_QWORD *)v15 + 2440LL))(
        v15,
        (signed __int64)v15 + 364,
        (signed __int64)v15 + 366,
        (signed __int64)v15 + 368,
        (signed __int64)v15 + 376);
      *((_QWORD *)v15 + 40) = *((_QWORD *)v15 + 38);
      v61 = v112;
      goto LABEL_116;
    }
    v61 = v112;
    if ( *((_QWORD *)v15 + 40) )
      goto LABEL_116;
  }
  v62 = (_QWORD *)*((_QWORD *)v15 + 38);
  if ( v62 && *v62 && *(_BYTE *)(*v62 + 227LL) )
LABEL_115:
    *((_QWORD *)v15 + 40) = v62;
LABEL_116:
  if ( v61 == *(_BYTE **)v17 )
    *(_BYTE *)(v17 + 19) = 0;
  (*(void (__fastcall **)(_BYTE *, __int64 *))(*(_QWORD *)v61 + 2704LL))(v61, &v135);
  (*(void (**)(void))(**(_QWORD **)(*((_QWORD *)v15 + 38) + 8LL) + 2512LL))();
  if ( v113 != 2652
    || v110 != 8703
    || !*((_BYTE *)v15 + 433)
    || !(*(unsigned __int8 (__fastcall **)(IOService *))(*(_QWORD *)v15 + 2784LL))(v15) )
  {
    goto LABEL_145;
  }
  if ( !*((_BYTE *)v15 + 66057) )
  {
    v81 = (char *)IOMalloc(511LL);
    v15 = (IOService *)this;
    if ( v81 )
    {
      v82 = v81;
      bzero(v81, 0x1FFuLL);
      snprintf(
        v82,
        0x1FFuLL,
        "**** [IOBluetoothFamily][ProcessBluetoothTransportShowsUpActionWL] -- Already tried to recover this X238E Blueto"
        "oth module booting from ROM once but failed. Letting it functions with the built-in ROM. ****\n");
      _os_log_internal(
        &dword_0,
        *(_QWORD *)(this + 424),
        0LL,
        IOBluetoothHCIController::ProcessBluetoothTransportShowsUpActionWL(IOBluetoothHostControllerTransport *)::_os_log_fmt,
        v82,
        v83);
      IOFree(v82, 511LL);
    }
LABEL_145:
    v84 = (char *)IOMalloc(511LL);
    if ( v84 )
    {
      v85 = v84;
      bzero(v84, 0x1FFuLL);
      v86 = (*(__int64 (__fastcall **)(IOService *, __int64))(*(_QWORD *)v15 + 2600LL))(v15, v17);
      v87 = (*(__int64 (__fastcall **)(IOService *, IOService *))(*(_QWORD *)v15 + 2600LL))(v15, v15);
      v88 = (*(__int64 (__fastcall **)(IOService *, _QWORD))(*(_QWORD *)v15 + 2600LL))(v15, *(_QWORD *)v17);
      snprintf(
        v85,
        0x1FFuLL,
        "**** [IOBluetoothFamily][ProcessBluetoothTransportShowsUpActionWL] -- calling IOBluetoothFamily's registerServic"
        "e() -- 0x%04x -- 0x%04x -- 0x%04x ****\n"
        "\n",
        v86,
        v87,
        v88);
      _os_log_internal(
        &dword_0,
        *((_QWORD *)v15 + 53),
        0LL,
        IOBluetoothHCIController::ProcessBluetoothTransportShowsUpActionWL(IOBluetoothHostControllerTransport *)::_os_log_fmt,
        v85,
        v89);
      IOFree(v85, 511LL);
    }
    *((_BYTE *)v15 + 435) = 1;
    (*(void (__fastcall **)(IOService *, _QWORD))(*(_QWORD *)v15 + 1456LL))(v15, 0LL);
    v90 = (char *)IOMalloc(511LL);
    if ( v90 )
    {
      v91 = v90;
      bzero(v90, 0x1FFuLL);
      snprintf(
        v91,
        0x1FFuLL,
        "**** [IOBluetoothFamily][ProcessBluetoothTransportShowsUpActionWL] -- calling messageClients (kIOBluetoothDevice"
        "TransportPublished) ****\n");
      _os_log_internal(
        &dword_0,
        *((_QWORD *)v15 + 53),
        0LL,
        IOBluetoothHCIController::ProcessBluetoothTransportShowsUpActionWL(IOBluetoothHostControllerTransport *)::_os_log_fmt,
        v91,
        v92);
      IOFree(v91, 511LL);
    }
    v26 = 0;
    v93 = 3758227464LL;
    (*(void (__fastcall **)(IOService *, signed __int64, _QWORD, _QWORD))(*(_QWORD *)v15 + 1856LL))(
      v15,
      3758227464LL,
      0LL,
      0LL);
    v94 = (char *)IOMalloc(511LL);
    if ( v94 )
    {
      v95 = v94;
      bzero(v94, 0x1FFuLL);
      v96 = (*(__int64 (__fastcall **)(IOService *, __int64))(*(_QWORD *)v15 + 2600LL))(v15, v17);
      v97 = (*(__int64 (__fastcall **)(IOService *, IOService *))(*(_QWORD *)v15 + 2600LL))(v15, v15);
      v98 = (*(__int64 (__fastcall **)(IOService *, _QWORD))(*(_QWORD *)v15 + 2600LL))(v15, *(_QWORD *)v17);
      snprintf(
        v95,
        0x1FFuLL,
        "**** [IOBluetoothFamily][ProcessBluetoothTransportShowsUpActionWL] -- Connected to the transport successfully --"
        " 0x%04x -- 0x%04x -- 0x%04x ****\n"
        "\n",
        v96,
        v97,
        v98);
      _os_log_internal(
        &dword_0,
        *((_QWORD *)v15 + 53),
        0LL,
        IOBluetoothHCIController::ProcessBluetoothTransportShowsUpActionWL(IOBluetoothHostControllerTransport *)::_os_log_fmt,
        v95,
        v99);
      v93 = 511LL;
      IOFree(v95, 511LL);
    }
    (*(void (__fastcall **)(_QWORD, signed __int64))(**(_QWORD **)(*((_QWORD *)v15 + 38) + 8LL) + 2296LL))(
      *(_QWORD *)(*((_QWORD *)v15 + 38) + 8LL),
      v93);
    *(_BYTE *)(*((_QWORD *)v15 + 38) + 19LL) = 0;
    goto LABEL_41;
  }
  *(_BYTE *)(this + 66057) = 0;
  v69 = (char *)IOMalloc(511LL);
  if ( v69 )
  {
    v70 = v69;
    bzero(v69, 0x1FFuLL);
    snprintf(
      v70,
      0x1FFuLL,
      "**** [IOBluetoothFamily][ProcessBluetoothTransportShowsUpActionWL] -- Try to recover internal X238E USB Bluetooth module. ****\n");
    _os_log_internal(
      &dword_0,
      *(_QWORD *)(this + 424),
      0LL,
      IOBluetoothHCIController::ProcessBluetoothTransportShowsUpActionWL(IOBluetoothHostControllerTransport *)::_os_log_fmt,
      v70,
      v71);
    IOFree(v70, 511LL);
  }
  v72 = (*(__int64 (__fastcall **)(__int64, _QWORD))(*(_QWORD *)this + 2792LL))(this, *(_QWORD *)(this + 304));
  v73 = *(_QWORD *)(this + 232);
  if ( v73 )
  {
    *(_BYTE *)(this + 344) = 0;
    v74 = (*(__int64 (__fastcall **)(__int64, signed __int64))(*(_QWORD *)v73 + 464LL))(v73, 10000LL);
    *(_BYTE *)(this + 224) = 1;
    (*(void (__fastcall **)(__int64, _QWORD, char *, __int64 *))(*(_QWORD *)this + 2848LL))(this, v74, v122, &v115);
  }
  if ( v72 )
  {
    v75 = (char *)IOMalloc(511LL);
    if ( v75 )
    {
      v27 = v75;
      bzero(v75, 0x1FFuLL);
      v26 = 0;
      snprintf(
        v27,
        0x1FFuLL,
        "**** [IOBluetoothFamily][ProcessBluetoothTransportShowsUpActionWL] -- Recovery attempt failed! Letting it functi"
        "ons with the built-in ROM. ****\n"
        "\n");
      _os_log_internal(
        &dword_0,
        *(_QWORD *)(this + 424),
        0LL,
        IOBluetoothHCIController::ProcessBluetoothTransportShowsUpActionWL(IOBluetoothHostControllerTransport *)::_os_log_fmt,
        v27,
        v76);
LABEL_21:
      v29 = v27;
LABEL_22:
      IOFree(v29, 511LL);
      goto LABEL_41;
    }
  }
  else
  {
    *(_BYTE *)(*(_QWORD *)(this + 304) + 19LL) = 0;
  }
  v26 = 0;
LABEL_41:
    return v26;
}
