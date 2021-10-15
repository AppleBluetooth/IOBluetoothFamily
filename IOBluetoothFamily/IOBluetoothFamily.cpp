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

IOReturn IOBluetoothHCIController::SwitchToSelectedHostController(UInt32 locationID)
{
  __int64 v5; // r12
  __int64 v6; // r14
  __int64 v7; // r15
  __int64 v8; // rdi
  char *v9; // rax
  char *v10; // rbx
  unsigned int v11; // eax
  char *v12; // rax
  char *v13; // rbx
  size_t v14; // rax
  __int64 result; // rax
  __int64 v16; // rdi
  char *v17; // rax
  char *v18; // rbx
  char *v19; // rax
  char *v20; // rbx
  unsigned int v21; // [rsp+Ch] [rbp-154h]
  __int64 v22; // [rsp+10h] [rbp-150h]
  __int64 v23; // [rsp+18h] [rbp-148h]
  __int64 v24; // [rsp+20h] [rbp-140h]
  __int64 v25; // [rsp+28h] [rbp-138h]
  __int64 v26; // [rsp+30h] [rbp-130h]
  __int64 v27; // [rsp+38h] [rbp-128h]
  __int64 v28; // [rsp+40h] [rbp-120h]
  __int64 v29; // [rsp+48h] [rbp-118h]
  __int64 v30; // [rsp+50h] [rbp-110h]
  __int64 v31; // [rsp+58h] [rbp-108h]
  __int64 v32; // [rsp+60h] [rbp-100h]
  __int64 v33; // [rsp+68h] [rbp-F8h]
  int v34; // [rsp+70h] [rbp-F0h]
  __int64 v35; // [rsp+80h] [rbp-E0h]
  __int64 v36; // [rsp+88h] [rbp-D8h]
  __int64 v37; // [rsp+90h] [rbp-D0h]
  __int64 v38; // [rsp+98h] [rbp-C8h]
  __int64 v39; // [rsp+A0h] [rbp-C0h]
  __int64 v40; // [rsp+A8h] [rbp-B8h]
  __int16 v41; // [rsp+B0h] [rbp-B0h]
  __int64 v42; // [rsp+C0h] [rbp-A0h]
  __int64 v43; // [rsp+C8h] [rbp-98h]
  __int64 v44; // [rsp+D0h] [rbp-90h]
  __int64 v45; // [rsp+D8h] [rbp-88h]
  __int64 v46; // [rsp+E0h] [rbp-80h]
  __int64 v47; // [rsp+E8h] [rbp-78h]
  __int64 v48; // [rsp+F0h] [rbp-70h]
  __int64 v49; // [rsp+F8h] [rbp-68h]
  __int64 v50; // [rsp+100h] [rbp-60h]
  __int64 v51; // [rsp+108h] [rbp-58h]
  __int64 v52; // [rsp+110h] [rbp-50h]
  __int64 v53; // [rsp+118h] [rbp-48h]
  int v54; // [rsp+120h] [rbp-40h]
  __int64 v55; // [rsp+130h] [rbp-30h]
    
    snprintf((char *)&v22, 0x64uLL, "IOBluetoothFamily::SwitchToSelectedHostController()");
    
    BluetoothHardwareListType * hardware;
    
    hardware = FindBluetoothHardware(locationID);
    if ( !hardware )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][SwitchToSelectedHostController] -- Error -- Cannot find controller with locationID (0x%x) -- this = 0x%04x ****\n", locationID, ConvertAddressToUInt32(this));
        BluetoothFamilyLogPacket(this, 250, "No controller @ locationID (0x%x)", locationID);
        *(_BYTE *)(this + 383) = 0;
        return -536870208;
    }
    
    if ( hardware == mActiveBluetoothHardware )
    {
        LODWORD(v6) = -536870208;
        goto LABEL_14;
    }
    
    *(_BYTE *)(this + 383) = 1;
    if ( (*(unsigned int (__fastcall **)(_QWORD, _QWORD, __int64 *))(**(_QWORD **)(mActiveBluetoothHardware + 8) + 3736LL))(
           *(_QWORD *)(mActiveBluetoothHardware + 8),
           0LL,
           &v22)
      || (*(unsigned int (__fastcall **)(_QWORD, _QWORD, __int64 *))(**(_QWORD **)(*(_QWORD *)(this + 304) + 8LL)
                                                                   + 3808LL))(
           *(_QWORD *)(*(_QWORD *)(this + 304) + 8LL),
           0LL,
           &v22) )
    {
      goto LABEL_14;
    }
    (*(void (__fastcall **)(__int64, signed __int64, _QWORD, _QWORD))(*(_QWORD *)this + 1856LL))(
      this,
      3758227465LL,
      0LL,
      0LL);
    (*(void (__fastcall **)(_QWORD, _QWORD))(***(_QWORD ***)(this + 304) + 2568LL))(**(_QWORD **)(this + 304), 0LL);
    *(_QWORD *)(this + 304) = hardware;
    (*(void (__fastcall **)(_QWORD, signed __int64))(**(_QWORD **)hardware + 2568LL))(*(_QWORD *)hardware, 1LL);
    *(_QWORD *)(this + 312) = hardware;
    if ( (*(unsigned int (__fastcall **)(_QWORD, signed __int64, __int64 *))(**(_QWORD **)(*(_QWORD *)(this + 304) + 8LL)
                                                                           + 3736LL))(
           *(_QWORD *)(*(_QWORD *)(this + 304) + 8LL),
           1LL,
           &v22) )
    {
      goto LABEL_6;
    }
    v21 = (*(__int64 (__fastcall **)(_QWORD, signed __int64, __int64 *))(**(_QWORD **)(*(_QWORD *)(this + 304) + 8LL)
                                                                       + 3808LL))(
            *(_QWORD *)(*(_QWORD *)(this + 304) + 8LL),
            1LL,
            &v22);
    (*(void (__fastcall **)(__int64, _QWORD, __int64 *, __int64 *))(*(_QWORD *)this + 2848LL))(this, v21, &v42, &v35);
    if ( !(*(unsigned __int8 (__fastcall **)(_QWORD, signed __int64))(**(_QWORD **)(*(_QWORD *)(this + 304) + 8LL)
                                                                    + 2392LL))(
            *(_QWORD *)(*(_QWORD *)(this + 304) + 8LL),
            1LL) )
    {
      if ( !(*(unsigned int (__fastcall **)(_QWORD, _QWORD, __int64 *))(**(_QWORD **)(*(_QWORD *)(this + 304) + 8LL)
                                                                      + 3736LL))(
              *(_QWORD *)(*(_QWORD *)(this + 304) + 8LL),
              0LL,
              &v22) )
        (*(void (__fastcall **)(_QWORD, _QWORD, __int64 *))(**(_QWORD **)(*(_QWORD *)(this + 304) + 8LL) + 3808LL))(
          *(_QWORD *)(*(_QWORD *)(this + 304) + 8LL),
          0LL,
          &v22);
LABEL_6:
      (*(void (__fastcall **)(_QWORD, _QWORD))(***(_QWORD ***)(this + 304) + 2568LL))(**(_QWORD **)(this + 304), 0LL);
      *(_QWORD *)(this + 304) = mActiveBluetoothHardware;
      (*(void (__fastcall **)(_QWORD, signed __int64))(**(_QWORD **)mActiveBluetoothHardware + 2568LL))(*(_QWORD *)mActiveBluetoothHardware, 1LL);
      if ( !(*(unsigned int (__fastcall **)(_QWORD, signed __int64, __int64 *))(**(_QWORD **)(*(_QWORD *)(this + 304)
                                                                                            + 8LL)
                                                                              + 3736LL))(
              *(_QWORD *)(*(_QWORD *)(this + 304) + 8LL),
              1LL,
              &v22) )
      {
        v7 = (*(unsigned int (__fastcall **)(_QWORD, signed __int64, __int64 *))(**(_QWORD **)(*(_QWORD *)(this + 304)
                                                                                             + 8LL)
                                                                               + 3808LL))(
               *(_QWORD *)(*(_QWORD *)(this + 304) + 8LL),
               1LL,
               &v22);
        (*(void (__fastcall **)(__int64, __int64, __int64 *, __int64 *))(*(_QWORD *)this + 2848LL))(
          this,
          v7,
          &v42,
          &v35);
        if ( (*(unsigned __int8 (__fastcall **)(_QWORD, signed __int64))(**(_QWORD **)(*(_QWORD *)(this + 304) + 8LL)
                                                                       + 2392LL))(
               *(_QWORD *)(*(_QWORD *)(this + 304) + 8LL),
               1LL) )
        {
          if ( *(_BYTE *)(this + 381) )
          {
            v8 = *(_QWORD *)(*(_QWORD *)(this + 304) + 8LL);
            v6 = (*(unsigned __int8 (__fastcall **)(__int64, _QWORD, _QWORD, __int64, _QWORD, _QWORD))(*(_QWORD *)this + 2408LL))(
                   this,
                   *(unsigned __int16 *)(v8 + 860),
                   *(unsigned __int16 *)(v8 + 858),
                   v8 + 868,
                   *(unsigned int *)(v8 + 864),
                   *(unsigned __int16 *)(v8 + 928));
            (*(void (__fastcall **)(__int64, __int64, __int64, __int64, __int64, __int64))(*(_QWORD *)this + 2416LL))(
              this,
              this + 346,
              this + 348,
              this + 350,
              this + 356,
              this + 360);
          }
          else
          {
            LODWORD(v6) = -536870208;
          }
          v19 = (char *)IOMalloc(511LL);
          if ( v19 )
          {
            v20 = v19;
            bzero(v19, 0x1FFuLL);
            snprintf(
              v20,
              0x1FFuLL,
              "**** [IOBluetoothFamily][SwitchToSelectedHostController] -- calling ControllerSetupComplete (0x%04X (%s)) "
              "for original controller ****\n",
              (unsigned int)v7,
              &v42);
            _os_log_internal(
              &dword_0,
              *(_QWORD *)(this + 424),
              0LL,
              IOBluetoothHCIController::SwitchToSelectedHostController(unsigned int)::_os_log_fmt,
              v20);
            IOFree(v20, 511LL);
          }
          (*(void (__fastcall **)(_QWORD, _QWORD))(**(_QWORD **)(*(_QWORD *)(this + 304) + 8LL) + 2376LL))(
            *(_QWORD *)(*(_QWORD *)(this + 304) + 8LL),
            (unsigned int)v7);
        }
      }
      goto LABEL_14;
    }
    if ( *(_BYTE *)(this + 381) )
    {
      v16 = *(_QWORD *)(*(_QWORD *)(this + 304) + 8LL);
      (*(void (__fastcall **)(__int64, _QWORD, _QWORD, __int64, _QWORD, _QWORD))(*(_QWORD *)this + 2408LL))(
        this,
        *(unsigned __int16 *)(v16 + 860),
        *(unsigned __int16 *)(v16 + 858),
        v16 + 868,
        *(unsigned int *)(v16 + 864),
        *(unsigned __int16 *)(v16 + 928));
      (*(void (__fastcall **)(__int64, __int64, __int64, __int64, __int64, __int64))(*(_QWORD *)this + 2416LL))(
        this,
        this + 346,
        this + 348,
        this + 350,
        this + 356,
        this + 360);
    }
    
    os_log(mInternalOSLogObject, "**** [IOBluetoothFamily][SwitchToSelectedHostController] -- Controller at locationID (0x%x) -- calling ControllerSetupComplete (0x%04X (%s)) ****\n", locationID, v21, &v42);
    mActiveBluetoothHardware->mBluetoothHostController->ControllerSetupComplete(v21);
    
    LODWORD(v6) = 0;
    
LABEL_14:
    *(_BYTE *)(this + 383) = 0;
    return v6;
}
