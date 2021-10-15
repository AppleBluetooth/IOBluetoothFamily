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
#include <IOKit/bluetooth/IOBluetoothInactivityTimerEventSource.h>
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
      //&& ((unsigned __int16)(cmdPacket->opCode + 978) > 0x20u || (v 23 = 5368709121LL, !_bittest64(&v 23, (unsigned __int16)(cmdPacket->opCode + 978))))
      && cmdPacket->opCode != 65374 )
    {
        os_log(mInternalOSLogObject, "**** [IOBluetoothHostController][SendRawHCICommand] -- mUpdatingFirmware is TRUE -- Not sending -- opCode = 0x%04x (%s) \n", cmdPacket->opCode, &opStr);
        err = -536870212;
        goto LABEL_34;
    }
    
    if (cmdPacket->opCode == 8200 || cmdPacket->opCode == 64843)
        UpdateLESetAdvertisingDataReporter(request);
    
    if ( mSupportNewIdlePolicy )
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
    
    if ( options == -536870272 )
        *(_BYTE *)(this + 959) = 1;
    
    return kIOReturnSuccess;
}

void IOBluetoothHostController::ProcessEventDataWL(UInt8 * inDataPtr, UInt32 inDataSize, UInt32 sequenceNumber)
{
  char *v6; // rax
  char *v7; // rbx
  __int64 v8; // rax
  const char *v9; // rcx
  const char *v10; // r8
  __int64 v11; // r14
  size_t v12; // rax
  __int64 v13; // r8
  __int64 v14; // rdi
  signed __int64 v15; // rsi
  unsigned __int16 v18; // r14
  signed __int64 v19; // rdx
  __int64 v20; // rbx
  char *v21; // rax
  __int64 v22; // r14
  int v23; // eax
  signed __int64 v24; // rdi
  char *v25; // rax
  char *v26; // rbx
  __int64 v27; // r14
  size_t v28; // rax
  __int64 v29; // rcx
  void *v30; // rsp
  __int64 v31; // rax
  unsigned int v32; // er14
  __int64 v33; // r13
  char *v34; // rax
  char *v35; // rbx
  __int64 v36; // r14
  size_t v37; // rax
  __int64 v38; // r14
  unsigned __int64 v39; // rbx
  __int64 v40; // rsi
  int v41; // eax
  __int64 v42; // rdx
  __int64 v43; // rcx
  unsigned int v44; // ebx
  unsigned int v45; // ebx
  __int64 v46; // rsi
  __int64 v47; // rax
  unsigned int v48; // esi
  char *v49; // rax
  char *v50; // rbx
  signed __int64 v51; // rdx
  unsigned __int8 v52; // al
  char *v53; // rax
  char *v54; // r13
  unsigned int v55; // er14
  char *v56; // rax
  char *v57; // rbx
  __int64 v58; // r14
  size_t v59; // rax
  char *v60; // rax
  char *v61; // r13
  unsigned int v62; // er14
  unsigned int v63; // ebx
  unsigned int v64; // eax
  char *v65; // rax
  char *v66; // rbx
  unsigned int v67; // er14
  unsigned int v68; // eax
  char *v69; // rax
  char *v70; // rbx
  unsigned int v71; // er14
  unsigned int v72; // eax
  char *v73; // rax
  char *v74; // rbx
  unsigned int v75; // er14
  unsigned int v76; // eax
  int v77; // ebx
  signed __int64 v78; // rsi
  char *v79; // rax
  char *v80; // rbx
  unsigned int v81; // er14
  unsigned int v82; // eax
  unsigned __int64 v83; // rsi
  __int64 v84; // rdi
  __int64 v85; // rbx
  unsigned int v86; // eax
  __int64 v87; // r13
  unsigned int v88; // eax
  signed int v89; // ecx
  unsigned int v90; // eax
  unsigned int v91; // er14
  unsigned int v92; // er13
  char *v93; // rax
  char *v94; // r15
  __int64 v95; // r14
  size_t v96; // rax
  unsigned int v97; // er13
  __int64 v98; // r15
  char *v99; // rax
  char *v100; // r15
  __int64 v101; // r14
  size_t v102; // rax
  __int64 v103; // rdi
  unsigned __int8 v104; // cl
  unsigned __int8 v105; // al
  char v106; // r14
  char *v107; // rax
  char *v108; // rbx
  char *v109; // rax
  char *v110; // rbx
  __int64 v111; // r14
  size_t v112; // rax
  char *v113; // rax
  __int64 v114; // rdi
  __int64 v115; // rax
  char v116; // al
  unsigned int v117; // ebx
  __int64 v118; // rax
  char v119; // r13
  char *v120; // rax
  char *v121; // r15
  bool v122; // r14
  __int64 v123; // rax
  unsigned __int16 v124; // ax
  __int64 v125; // rax
  bool v126; // bl
  __int64 v127; // rax
  bool v128; // al
  __int64 v129; // rcx
  __int64 v130; // r9
  __int64 v131; // r10
  __int64 v132; // rdx
  __int64 v133; // rdi
  signed __int64 v134; // rsi
  __int64 v135; // r8
  signed int v136; // ecx
  int v137; // ebx
  char *v138; // rax
  char *v139; // rbx
  __int64 v140; // r14
  size_t v141; // rax
  char *v142; // rax
  char *v143; // rbx
  __int64 v144; // r14
  size_t v145; // rax
  __int64 v146; // rax
  char *v147; // rax
  char *v148; // rbx
  __int64 v149; // r14
  size_t v150; // rax
  char *v151; // rax
  char *v152; // rbx
  __int64 v153; // r14
  size_t v154; // rax
  __int64 v155; // rdi
  __int64 v156; // rsi
  unsigned int v157; // er15
  char *v158; // rax
  char *v159; // rbx
  __int64 v160; // rcx
  char *v161; // rax
  char *v162; // rbx
  unsigned __int16 v163; // ax
  unsigned int v164; // er14
  unsigned int v165; // er13
  unsigned int v166; // er13
  unsigned __int64 v167; // r15
  char *v168; // rax
  char *v169; // rbx
  char *v170; // rax
  __int64 v171; // r14
  size_t v172; // rax
  __int64 v173; // rbx
  size_t v174; // rax
  __int16 v175; // r14
  char *v176; // rax
  char *v177; // rbx
  __int64 v178; // r14
  size_t v179; // rax
  __int64 v180; // rdx
  __int16 v181; // ax
  bool v182; // cf
  bool v183; // zf
  unsigned __int16 v184; // r13
  int v185; // eax
  unsigned __int8 v186; // cl
  unsigned int v187; // edx
  unsigned __int8 v188; // al
  void *v189; // rsp
  __int64 v190; // rax
  unsigned __int8 v191; // cl
  unsigned __int16 v192; // ax
  unsigned int v193; // er8
  unsigned __int8 *v194; // rdi
  __int64 v195; // rax
  unsigned __int64 v196; // r13
  __int64 v197; // rdi
  __int64 v198; // rdx
  int v199; // ebx
  unsigned __int8 v200; // al
  unsigned int v201; // ecx
  char *v202; // rax
  char *v203; // rbx
  __int64 v204; // r14
  size_t v205; // rax
  __int64 v206; // [rsp-10h] [rbp-3B0h]
  __int64 v207; // [rsp-8h] [rbp-3A8h]
  char *v208; // [rsp-8h] [rbp-3A8h]
  __int64 v209; // [rsp-8h] [rbp-3A8h]
  __int64 v210; // [rsp-8h] [rbp-3A8h]
  unsigned __int64 v211; // [rsp+0h] [rbp-3A0h]
  __int64 v212; // [rsp+8h] [rbp-398h]
  __int64 v213; // [rsp+10h] [rbp-390h]
  __int16 v215; // [rsp+1Ch] [rbp-384h]
  __int64 v216; // [rsp+20h] [rbp-380h]
  unsigned int v217; // [rsp+2Ch] [rbp-374h]
  __int64 v218; // [rsp+30h] [rbp-370h]
  void *v219; // [rsp+38h] [rbp-368h]
  __int64 v222; // [rsp+50h] [rbp-350h]
  unsigned int v223; // [rsp+5Ch] [rbp-344h]
  __int64 v226; // [rsp+70h] [rbp-330h]
  char v227; // [rsp+7Dh] [rbp-323h]
  unsigned __int8 v228; // [rsp+7Eh] [rbp-322h]
  __int64 v230; // [rsp+80h] [rbp-320h]
  __int64 v231; // [rsp+88h] [rbp-318h]
  __int64 v232; // [rsp+90h] [rbp-310h]
  __int64 v233; // [rsp+98h] [rbp-308h]
  __int64 v234; // [rsp+A0h] [rbp-300h]
  __int64 v235; // [rsp+A8h] [rbp-2F8h]
  __int16 v236; // [rsp+B0h] [rbp-2F0h]
  char v237[4]; // [rsp+C0h] [rbp-2E0h]
  __int64 v238; // [rsp+C8h] [rbp-2D8h]
  __int64 v239; // [rsp+D0h] [rbp-2D0h]
  __int64 v240; // [rsp+D8h] [rbp-2C8h]
  __int64 v241; // [rsp+E0h] [rbp-2C0h]
  __int64 v242; // [rsp+E8h] [rbp-2B8h]
  __int64 v243; // [rsp+F0h] [rbp-2B0h]
  __int64 v244; // [rsp+F8h] [rbp-2A8h]
  __int64 v245; // [rsp+100h] [rbp-2A0h]
  __int64 v246; // [rsp+108h] [rbp-298h]
  __int64 v247; // [rsp+110h] [rbp-290h]
  __int64 v248; // [rsp+118h] [rbp-288h]
  int v249; // [rsp+120h] [rbp-280h]
  char v250[8]; // [rsp+130h] [rbp-270h]
  __int64 v251; // [rsp+138h] [rbp-268h]
  __int64 v252; // [rsp+140h] [rbp-260h]
  __int64 v253; // [rsp+148h] [rbp-258h]
  __int64 v254; // [rsp+150h] [rbp-250h]
  __int64 v255; // [rsp+158h] [rbp-248h]
  __int64 v256; // [rsp+160h] [rbp-240h]
  __int64 v257; // [rsp+168h] [rbp-238h]
  __int64 v258; // [rsp+170h] [rbp-230h]
  __int64 v259; // [rsp+178h] [rbp-228h]
  __int64 v260; // [rsp+180h] [rbp-220h]
  __int64 v261; // [rsp+188h] [rbp-218h]
  int v262; // [rsp+190h] [rbp-210h]
  char v263; // [rsp+1A0h] [rbp-200h]
  __int64 v264; // [rsp+270h] [rbp-130h]
  __int64 v265; // [rsp+278h] [rbp-128h]
  __int64 v266; // [rsp+280h] [rbp-120h]
  __int64 v267; // [rsp+288h] [rbp-118h]
  __int64 v268; // [rsp+290h] [rbp-110h]
  __int64 v269; // [rsp+298h] [rbp-108h]
  __int64 v270; // [rsp+2A0h] [rbp-100h]
  __int64 v271; // [rsp+2A8h] [rbp-F8h]
  __int64 v272; // [rsp+2B0h] [rbp-F0h]
  __int64 v273; // [rsp+2B8h] [rbp-E8h]
  __int64 v274; // [rsp+2C0h] [rbp-E0h]
  __int64 v275; // [rsp+2C8h] [rbp-D8h]
  int v276; // [rsp+2D0h] [rbp-D0h]
  __int64 v277; // [rsp+2E0h] [rbp-C0h]
  __int64 v278; // [rsp+2E8h] [rbp-B8h]
  __int64 v279; // [rsp+2F0h] [rbp-B0h]
  __int64 v280; // [rsp+2F8h] [rbp-A8h]
  __int64 v281; // [rsp+300h] [rbp-A0h]
  __int64 v282; // [rsp+308h] [rbp-98h]
  __int64 v283; // [rsp+310h] [rbp-90h]
  __int64 v284; // [rsp+318h] [rbp-88h]
  __int64 v285; // [rsp+320h] [rbp-80h]
  __int64 v286; // [rsp+328h] [rbp-78h]
  __int64 v287; // [rsp+330h] [rbp-70h]
  __int64 v288; // [rsp+338h] [rbp-68h]
  int v289; // [rsp+340h] [rbp-60h]
  __int64 v291; // [rsp+358h] [rbp-48h]
  int v292; // [rsp+360h] [rbp-40h]

  
    BluetoothHCICommandOpCode opCode;
    BluetoothHCIEventCode eventCode;
    BluetoothHCIEventStatus eventStatus;
    UInt8 v228;
    BluetoothDeviceAddress deviceAddress;
    BluetoothConnectionHandle handle;
    bool v227;
    
    
    if ( mBluetoothFamily->mTestHCICommandTimeoutHardReset || mBluetoothFamily->mTestNoHardResetWhenSleepCommandTimeout || mBluetoothFamily->mTestHCICommandTimeoutWhenWake )
    {
        BluetoothFamilyLogPacketWithOSLog(mInternalOSLogObject, mBluetoothFamily, 249, "\n**** [IOBluetoothHostController][ProcessEventDataWL] -- exiting -- mBluetoothFamily->mTestHCICommandTimeoutHardReset is %s, mBluetoothFamily->mTestNoHardResetWhenSleepCommandTimeout is %s, mBluetoothFamily->mTestHCICommandTimeoutWhenWake is %s **** \n\n", mBluetoothFamily->mTestHCICommandTimeoutHardReset ? "TRUE" : "FALSE", mBluetoothFamily->mTestNoHardResetWhenSleepCommandTimeout ? "TRUE" : "FALSE", mBluetoothFamily->mTestHCICommandTimeoutWhenWake ? "TRUE" : "FALSE");
        return;
    }
    
    bzero(v250, 0x64uLL);
    snprintf(v250, 0x64uLL, "IOBluetoothHostController::ProcessEventDataWL()");
    
    *(_BYTE *)(this + 896) = 1;
    
    if ( mIdleTimer )
        mIdleTimer->resetTimer();
    
    if ( !inDataPtr || !inDataSize )
      goto LABEL_253;
      
    v18 = 0;
    v19 = 0xFFFFFFFFLL;
    
    v20 = inDataSize;
    mBluetoothFamily->LogPacket(1, inDataPtr, inDataSize);
    
    bzero(&deviceAddress, 6uLL);
    if ( inDataSize <= 1 )
    {
        BluetoothFamilyLogPacketWithOSLog(mInternalOSLogObject, mBluetoothFamily, 250, "**** [IOBluetoothHostController][ProcessEventDataWL] -- Incoming event data size is invalid (%u bytes) -- not enough data! \n", inDataSize);
        return;
    }
    
    v222 = inDataSize;
    
    v23 = GetOpCodeAndEventCode(inDataPtr, inDataSize, &opCode, &eventCode, &eventStatus, &v228, &deviceAddress, &handle, &v227);
    
    if ( v23 )
    {
        BluetoothFamilyLogPacketWithOSLog(mInternalOSLogObject, mBluetoothFamily, 250, "**** [IOBluetoothHostController][ProcessEventDataWL] -- GetOpCodeAndEventCode() failed -- inDataSize = %d -- this = %p ****\n", inDataSize, this);
        
        if ( inDataSize >= 3 && inDataPtr[0] == 15 && inDataPtr[2] == 1 )
        {
            /*
            v216 = (__int64)&v211;
            *(_WORD *)v237 = -21846;
            v237[2] = -86;
            v30 = alloca(__chkstk_darwin(v24, inDataPtr));
            if ( (_DWORD)v29 )
        {
          v31 = 0LL;
          do
            *((_BYTE *)&v211 + v31++) = -86;
          while ( v29 != v31 );
        }
        v218 = v29;
        v32 = 0;
        v33 = 0LL;
        do
        {
          snprintf(v237, 3uLL, "%02X", inDataPtr[v33]);
          *((_BYTE *)&v211 + v32) = v237[0];
          *((_BYTE *)&v211 + v32 + 1) = v237[1];
          *((_BYTE *)&v211 + v32 + 2) = 32;
          ++v33;
          v32 += 3;
        }
        while ( inDataSize != v33 );
        *((_BYTE *)&v211 + (unsigned int)(v218 - 1)) = 0;
            */ // Totally ridiculous code. What it does is translate the data into a string form
            
            os_log(mInternalOSLogObject, "**** [IOBluetoothHostController][ProcessEventDataWL] -- Received Unknown HCI Command error for Command Status Event -- data: %s", &v211);
            BluetoothFamilyLogPacket(mBluetoothFamily, 250, "**** [IOBluetoothHostController][ProcessEventDataWL] -- Received Unknown HCI Command error for Command Status Event! \n");
      }
    }
    
    if ( eventCode >= 2 )
    {
      v39 = 0LL;
      while ( 1 )
      {
        v38 = *((unsigned __int16 *)&opCode + v39);
        (*(void (__fastcall **)(_QWORD, _QWORD, __int64 *))(**(_QWORD **)(this + 824) + 2856LL))(
          *(_QWORD *)(this + 824),
          (unsigned __int16)v38,
          &v277);
        v40 = (unsigned __int16)v38;
        v41 = (*(__int64 (__fastcall **)(__int64, _QWORD, int *, _QWORD, bool, __int64 *))(*(_QWORD *)this + 3296LL))(
                this,
                (unsigned __int16)v38,
                &deviceAddress,
                handle,
                eventStatus != 15,
                &v226);
        if ( v226 )
          break;
        ++v39;
        v43 = eventCode;
        if ( v39 >= eventCode )
        {
          LODWORD(v216) = v41;
          v38 = 0LL;
          goto LABEL_47;
        }
      }
      LODWORD(v216) = v41;
      v40 = (unsigned __int16)v38;
      (*(void (__fastcall **)(_QWORD, _QWORD, __int64 *))(**(_QWORD **)(this + 824) + 2856LL))(
        *(_QWORD *)(this + 824),
        (unsigned __int16)v38,
        &v277);
LABEL_47:
      inDataPtr = inDataPtr;
    }
    else
    {
      if ( eventCode == 1 )
        v38 = (unsigned __int16)opCode;
      else
        v38 = 0LL;
      v40 = (unsigned __int16)v38;
      LODWORD(v216) = (*(__int64 (__fastcall **)(__int64, _QWORD, int *, _QWORD, bool, __int64 *))(*(_QWORD *)this + 3296LL))(
                        this,
                        (unsigned __int16)v38,
                        &deviceAddress,
                        handle,
                        eventStatus != 15,
                        &v226);
    }
    
    v44 = (unsigned __int16)v38;
    LOBYTE(v42) = (unsigned __int16)v38 == 8214;
    LOBYTE(v43) = (unsigned __int16)v38 == 8211;
    if ( !v227 && eventStatus == 15 )
    {
      LOBYTE(v43) = v42 | ((unsigned __int16)v38 == 8205) | v43;
      if ( (_BYTE)v43 )
        v227 = 1;
    }
    v222 = (unsigned __int16)v38;
    if ( v226 )
    {
      (*(void (__fastcall **)(__int64, __int64, __int64, __int64))(*(_QWORD *)this + 3368LL))(this, v40, v42, v43);
      if ( eventStatus == -1 )
        (*(void (__fastcall **)(__int64, _QWORD, __int64))(*(_QWORD *)this + 3216LL))(this, inDataPtr[2], v226);
      else
        (*(void (__fastcall **)(__int64, _QWORD, __int64))(*(_QWORD *)this + 3216LL))(this, eventStatus, v226);
      if ( v226 )
      {
        if ( eventStatus == -1 )
        {
          v45 = inDataPtr[2];
          v248 = -6148914691236517206LL;
          v247 = -6148914691236517206LL;
          v246 = -6148914691236517206LL;
          v245 = -6148914691236517206LL;
          v244 = -6148914691236517206LL;
          v243 = -6148914691236517206LL;
          v242 = -6148914691236517206LL;
          v241 = -6148914691236517206LL;
          v240 = -6148914691236517206LL;
          v239 = -6148914691236517206LL;
          v238 = -6148914691236517206LL;
          *(_QWORD *)v237 = -6148914691236517206LL;
          v249 = -1431655766;
          (*(void (__fastcall **)(_QWORD, _QWORD))(**(_QWORD **)(this + 824) + 2872LL))(*(_QWORD *)(this + 824), v45);
          v46 = v45;
          v44 = v222;
          (*(void (__fastcall **)(__int64, __int64, __int64))(*(_QWORD *)this + 3224LL))(this, v46, v226);
        }
        else
        {
          (*(void (__fastcall **)(__int64, _QWORD))(*(_QWORD *)this + 3224LL))(this, eventStatus);
        }
      }
    }
    (*(void (__fastcall **)(_QWORD, _QWORD, __int64 *, __int64))(**(_QWORD **)(this + 824) + 2864LL))(
      *(_QWORD *)(this + 824),
      eventStatus,
      &v264,
      v43);
    (*(void (__fastcall **)(_QWORD, _QWORD, __int64 *))(**(_QWORD **)(this + 824) + 2856LL))(
      *(_QWORD *)(this + 824),
      v44,
      &v277);
    (*(void (__fastcall **)(_QWORD, _QWORD, char *))(**(_QWORD **)(this + 824) + 2880LL))(
      *(_QWORD *)(this + 824),
      v228,
      &v263);
    v212 = v38;
    if ( eventStatus != 14 )
    {
      if ( eventStatus == 16 )
      {
        v60 = (char *)IOMalloc(511LL);
        if ( v60 )
        {
          v61 = v60;
          bzero(v60, 0x1FFuLL);
          v62 = v228;
          v63 = *(unsigned __int8 *)(this + 912);
          v64 = (*(__int64 (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 4168LL))(this, v226);
          snprintf(
            v61,
            0x1FFuLL,
            "[IOBluetoothHostController][ProcessEventDataWL] -- kBluetoothHCIEventHardwareError occurred -- status = 0x%0"
            "4X (%s) -- mNumberOfCommandsAllowedByHardware is %d,  processing -- opCode = 0x%04x (%s),  requestPtr = 0x%04X \n",
            v62,
            &v263,
            v63,
            v222,
            &v277,
            v64);
          _os_log_internal(
            &dword_0,
            *(_QWORD *)(this + 1288),
            0LL,
            IOBluetoothHostController::ProcessEventDataWL(unsigned char *,unsigned int,unsigned int)::_os_log_fmt,
            v61);
          IOFree(v61, 511LL);
        }
        if ( v228 == 3 )
        {
          v65 = (char *)IOMalloc(511LL);
          if ( v65 )
          {
            v66 = v65;
            bzero(v65, 0x1FFuLL);
            v67 = *(unsigned __int8 *)(this + 912);
            v68 = (*(__int64 (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 4168LL))(this, v226);
            snprintf(
              v66,
              0x1FFuLL,
              "[IOBluetoothHostController][ProcessEventDataWL] -- eventStatus = kBluetoothHCIErrorHardwareFailure -- we a"
              "re attempting a device reset! -- mNumberOfCommandsAllowedByHardware is %d,  processing -- opCode = 0x%04x "
              "(%s),  requestPtr = 0x%04X \n",
              v67,
              v222,
              &v277,
              v68);
            _os_log_internal(
              &dword_0,
              *(_QWORD *)(this + 1288),
              0LL,
              IOBluetoothHostController::ProcessEventDataWL(unsigned char *,unsigned int,unsigned int)::_os_log_fmt,
              v66);
            IOFree(v66, 511LL);
          }
          v217 = -1431655766;
          v234 = 0LL;
          v233 = 0LL;
          v232 = 0LL;
          v230 = 0LL;
          v231 = 1751345507LL;
          v69 = (char *)IOMalloc(511LL);
          if ( v69 )
          {
            v70 = v69;
            bzero(v69, 0x1FFuLL);
            v71 = *(unsigned __int8 *)(this + 912);
            v72 = (*(__int64 (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 4168LL))(this, v226);
            snprintf(
              v70,
              0x1FFuLL,
              "[IOBluetoothHostController][ProcessEventDataWL] -- eventStatus = kBluetoothHCIErrorHardwareFailure -- call"
              "ing KillAllPendingRequests() -- mNumberOfCommandsAllowedByHardware is %d,  processing -- opCode = 0x%04x ("
              "%s),  requestPtr = 0x%04X \n",
              v71,
              v222,
              &v277,
              v72);
            _os_log_internal(
              &dword_0,
              *(_QWORD *)(this + 1288),
              0LL,
              IOBluetoothHostController::ProcessEventDataWL(unsigned char *,unsigned int,unsigned int)::_os_log_fmt,
              v70);
            IOFree(v70, 511LL);
          }
          (*(void (__fastcall **)(__int64, signed __int64, signed __int64))(*(_QWORD *)this + 3344LL))(this, 1LL, 1LL);
          if ( !(*(unsigned int (__fastcall **)(__int64, unsigned int *, _QWORD, signed __int64, __int64 *, _QWORD, _QWORD))(*(_QWORD *)this + 3128LL))(
                  this,
                  &v217,
                  0LL,
                  3000LL,
                  &v230,
                  0LL,
                  0LL) )
          {
            v73 = (char *)IOMalloc(511LL);
            if ( v73 )
            {
              v74 = v73;
              bzero(v73, 0x1FFuLL);
              v75 = *(unsigned __int8 *)(this + 912);
              v76 = (*(__int64 (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 4168LL))(this, v226);
              snprintf(
                v74,
                0x1FFuLL,
                "[IOBluetoothHostController][ProcessEventDataWL] -- eventStatus = kBluetoothHCIErrorHardwareFailure -- ca"
                "lling BluetoothHCIReset() -- mNumberOfCommandsAllowedByHardware is %d,  processing -- opCode = 0x%04x (%"
                "s),  requestPtr = 0x%04X \n",
                v75,
                v222,
                &v277,
                v76);
              _os_log_internal(
                &dword_0,
                *(_QWORD *)(this + 1288),
                0LL,
                IOBluetoothHostController::ProcessEventDataWL(unsigned char *,unsigned int,unsigned int)::_os_log_fmt,
                v74);
              IOFree(v74, 511LL);
            }
            qmemcpy(v237, "IOBluetoothHostController::ProcessEventDataWL -- HardwareFailure error", 0x40uLL);
            *(_DWORD *)((char *)&v245 + 3) = 7499634;
            LODWORD(v245) = 1920099616;
            v77 = (*(__int64 (__fastcall **)(__int64, _QWORD))(*(_QWORD *)this + 4800LL))(this, v217);
            v78 = 0LL;
            (*(void (__fastcall **)(__int64, _QWORD, _QWORD))(*(_QWORD *)this + 3136LL))(this, 0LL, v217);
            if ( !v77 )
            {
              v79 = (char *)IOMalloc(511LL);
              if ( v79 )
              {
                v80 = v79;
                bzero(v79, 0x1FFuLL);
                v81 = *(unsigned __int8 *)(this + 912);
                v82 = (*(__int64 (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 4168LL))(this, v226);
                snprintf(
                  v80,
                  0x1FFuLL,
                  "[IOBluetoothHostController][ProcessEventDataWL] -- eventStatus = kBluetoothHCIErrorHardwareFailure -- "
                  "calling SetupLighthouse() -- mNumberOfCommandsAllowedByHardware is %d,  processing -- opCode = 0x%04x "
                  "(%s),  requestPtr = 0x%04X \n",
                  v81,
                  v222,
                  &v277,
                  v82);
                _os_log_internal(
                  &dword_0,
                  *(_QWORD *)(this + 1288),
                  0LL,
                  IOBluetoothHostController::ProcessEventDataWL(unsigned char *,unsigned int,unsigned int)::_os_log_fmt,
                  v80);
                v78 = 511LL;
                IOFree(v80, 511LL);
              }
              (*(void (__fastcall **)(__int64, signed __int64))(*(_QWORD *)this + 4224LL))(this, v78);
              (*(void (__fastcall **)(__int64))(*(_QWORD *)this + 4272LL))(this);
            }
          }
        }
      }
      else
      {
        if ( eventStatus == 15 )
        {
          v47 = v226;
          v48 = inDataSize;
          if ( (v38 & 0xFFF7) == 1025 && v226 )
            *(_BYTE *)(v226 + 2788) = 1;
          if ( *(_BYTE *)(this + 888) && v44 == 1029 && v47 )
            *(_BYTE *)(v47 + 2788) = 1;
LABEL_75:
          if ( v48 >= 3 && (v51 = 2LL, eventStatus == 14) || (v52 = 0, v48 >= 4) && (v51 = 3LL, eventStatus == 15) )
            v52 = inDataPtr[v51];
          if ( v52 || !*(_BYTE *)(this + 912) )
          {
            *(_BYTE *)(this + 912) = v52;
          }
          else
          {
            v53 = (char *)IOMalloc(511LL);
            if ( v53 )
            {
              v54 = v53;
              bzero(v53, 0x1FFuLL);
              v55 = *(unsigned __int8 *)(this + 912);
              v207 = (*(unsigned int (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 4168LL))(this, v226);
              snprintf(
                v54,
                0x1FFuLL,
                "**** [IOBluetoothHostController][ProcessEventDataWL] -- Error: inconsistent allowed command count -- mNu"
                "mberOfCommandsAllowedByHardware is %d,  numberOfCommandWeCanProcess = %d,  opCode = 0x%04x (%s),  reques"
                "tPtr = 0x%04x \n",
                v55,
                0LL,
                v222,
                &v277,
                v207);
              _os_log_internal(
                &dword_0,
                *(_QWORD *)(this + 1288),
                0LL,
                IOBluetoothHostController::ProcessEventDataWL(unsigned char *,unsigned int,unsigned int)::_os_log_fmt,
                v54);
              IOFree(v54, 511LL);
            }
            v56 = (char *)IOMalloc(511LL);
            if ( v56 )
            {
              v57 = v56;
              bzero(v56, 0x1FFuLL);
              snprintf(v57, 0x1FFuLL, "Incorrect cmd cnt Allow %d Process %d", *(unsigned __int8 *)(this + 912), 0LL);
              v58 = *(_QWORD *)(this + 824);
              if ( v58 )
              {
                v59 = strlen(v57);
                (*(void (__fastcall **)(__int64, signed __int64, char *, size_t))(*(_QWORD *)v58 + 2248LL))(
                  v58,
                  250LL,
                  v57,
                  v59);
              }
              IOFree(v57, 511LL);
            }
          }
          goto LABEL_107;
        }
        if ( eventStatus == 1 && v226 && !*(_BYTE *)(v226 + 2788) )
          *(_BYTE *)(this + 912) = 1;
      }
LABEL_107:
      bzero(*(void **)(this + 264), *(_QWORD *)(this + 272));
      if ( (_DWORD)v216 || !v226 )
      {
        v87 = *(_QWORD *)(this + 264);
        v88 = *(_DWORD *)(this + 272);
        v223 = *(_DWORD *)(this + 272);
        v89 = -1;
      }
      else
      {
        v248 = -6148914691236517206LL;
        v247 = -6148914691236517206LL;
        v246 = -6148914691236517206LL;
        v245 = -6148914691236517206LL;
        v244 = -6148914691236517206LL;
        v243 = -6148914691236517206LL;
        v242 = -6148914691236517206LL;
        v241 = -6148914691236517206LL;
        v240 = -6148914691236517206LL;
        v239 = -6148914691236517206LL;
        v238 = -6148914691236517206LL;
        *(_QWORD *)v237 = -6148914691236517206LL;
        v249 = -1431655766;
        snprintf(v237, 0x64uLL, "IOBluetoothHostController::ProcessEventDataWL -- Found request");
        v83 = (unsigned __int64)v237;
        IOBluetoothHCIRequest::RetainRequest((IOBluetoothHCIRequest *)v226, v237);
        v84 = v226;
        *(_DWORD *)(v226 + 2760) = v228;
        v85 = (__int64)IOBluetoothHCIRequest::GetResultsBuffer(v84);
        v86 = IOBluetoothHCIRequest::GetResultsBufferSize(v226);
        v223 = v86;
        if ( v85 && v86 )
        {
          LODWORD(v218) = *(_DWORD *)(v226 + 2192);
          v87 = *(_QWORD *)(this + 264);
          v223 = *(_DWORD *)(this + 272);
          LOBYTE(v85) = 1;
          goto LABEL_116;
        }
        v87 = *(_QWORD *)(this + 264);
        v88 = *(_DWORD *)(this + 272);
        v223 = v88;
        v89 = *(_DWORD *)(v226 + 2192);
        if ( !v88 )
        {
          LODWORD(v218) = *(_DWORD *)(v226 + 2192);
LABEL_175:
          v106 = 0;
LABEL_201:
          v105 = eventStatus;
LABEL_202:
          LOBYTE(v85) = 0;
          if ( v105 == 19 )
            goto LABEL_212;
          goto LABEL_203;
        }
      }
      LODWORD(v218) = v89;
      LODWORD(v85) = 0;
      if ( !v87 || !v88 )
        goto LABEL_212;
LABEL_116:
      v83 = inDataSize;
      v90 = ParseHCIEvent(inDataPtr, inDataSize, v87, &v223, &v228);
      v91 = v222;
      v219 = (void *)v87;
      if ( v90 )
      {
        LODWORD(v213) = v85;
        v248 = -6148914691236517206LL;
        v247 = -6148914691236517206LL;
        v246 = -6148914691236517206LL;
        v245 = -6148914691236517206LL;
        v244 = -6148914691236517206LL;
        v243 = -6148914691236517206LL;
        v242 = -6148914691236517206LL;
        v241 = -6148914691236517206LL;
        v240 = -6148914691236517206LL;
        v239 = -6148914691236517206LL;
        v238 = -6148914691236517206LL;
        *(_QWORD *)v237 = -6148914691236517206LL;
        v249 = -1431655766;
        v235 = -6148914691236517206LL;
        v234 = -6148914691236517206LL;
        v233 = -6148914691236517206LL;
        v232 = -6148914691236517206LL;
        v231 = -6148914691236517206LL;
        v230 = -6148914691236517206LL;
        v236 = -21846;
        v92 = v90;
        v83 = v90;
        (*(void (__fastcall **)(_QWORD, _QWORD))(**(_QWORD **)(this + 824) + 2848LL))(*(_QWORD *)(this + 824), v90);
        v93 = (char *)IOMalloc(511LL);
        if ( v93 )
        {
          v94 = v93;
          bzero(v93, 0x1FFuLL);
          v208 = &v263;
          snprintf(
            v94,
            0x1FFuLL,
            "[IOBluetoothHostController][ProcessEventDataWL] -- broadcastDataPtr is NOT NULL and broadcastDataSize > 0 --"
            " ParseHCIEvent() failed -- Error code 0x%04X (%s) -- opCode = 0x%04X (%s),  requestPtr = %p, eventStatus = 0x%02x (%s) \n",
            v92,
            v237,
            v91,
            &v277,
            v226,
            v228,
            &v263);
          v95 = *(_QWORD *)(this + 824);
          if ( v95 )
          {
            v96 = strlen(v94);
            (*(void (__fastcall **)(__int64, signed __int64, char *, size_t))(*(_QWORD *)v95 + 2248LL))(
              v95,
              250LL,
              v94,
              v96);
          }
          v83 = 511LL;
          IOFree(v94, 511LL);
          v91 = v222;
        }
        v87 = (__int64)v219;
        LODWORD(v85) = v213;
      }
      if ( (_BYTE)v85 )
      {
        v85 = (__int64)IOBluetoothHCIRequest::GetResultsBuffer(v226);
        v97 = IOBluetoothHCIRequest::GetResultsBufferSize(v226);
        v98 = (__int64)inDataPtr;
        if ( v223 > v97 )
        {
          v99 = (char *)IOMalloc(511LL);
          if ( v99 )
          {
            v100 = v99;
            v213 = v85;
            bzero(v99, 0x1FFuLL);
            snprintf(
              v100,
              0x1FFuLL,
              "[IOBluetoothHostController][ProcessEventDataWL] -- Not enough room in request's result data buffer (%d byt"
              "es) to copy the actual data (%d bytes) -- opCode = 0x%04X (%s)\n",
              v97,
              v223,
              v91,
              &v277);
            _os_log_internal(
              &dword_0,
              *(_QWORD *)(this + 1288),
              0LL,
              IOBluetoothHostController::ProcessEventDataWL(unsigned char *,unsigned int,unsigned int)::_os_log_fmt,
              v100);
            v101 = *(_QWORD *)(this + 824);
            if ( v101 )
            {
              v102 = strlen(v100);
              (*(void (__fastcall **)(__int64, signed __int64, char *, size_t))(*(_QWORD *)v101 + 2248LL))(
                v101,
                250LL,
                v100,
                v102);
            }
            IOFree(v100, 511LL);
            v85 = v213;
          }
          v223 = v97;
          v98 = (__int64)inDataPtr;
        }
        v83 = (unsigned __int64)v219;
        memmove((void *)v85, v219, v97);
        v87 = (__int64)IOBluetoothHCIRequest::GetResultsBuffer(v226);
      }
      else
      {
        v98 = (__int64)inDataPtr;
      }
      v103 = v226;
      v104 = v228;
      if ( !(_DWORD)v216 && v226 )
        *(_DWORD *)(v226 + 2760) = v228;
      v105 = eventStatus;
      v106 = 0;
      if ( v104 )
      {
        if ( (char)eventStatus <= 13 )
        {
          if ( eventStatus != 3 )
          {
            if ( eventStatus != 5 )
              goto LABEL_202;
            (*(void (__fastcall **)(_QWORD, unsigned __int64))(**(_QWORD **)(this + 824) + 2504LL))(
              *(_QWORD *)(this + 824),
              v83);
            v113 = (char *)IOMalloc(511LL);
            if ( !v113 )
              goto LABEL_174;
            v108 = v113;
            bzero(v113, 0x1FFuLL);
            snprintf(
              v108,
              0x1FFuLL,
              "[IOBluetoothHostController][ProcessEventDataWL] -- Destroying device even though we had an error on discon"
              "nection -- calling DestroyDeviceWithDisconnectionResults() \n");
            _os_log_internal(
              &dword_0,
              *(_QWORD *)(this + 1288),
              0LL,
              _ZZN25IOBluetoothHostController18ProcessEventDataWLEPhjjE11_os_log_fmt__17_,
              v108);
            goto LABEL_156;
          }
        }
        else
        {
          if ( eventStatus == 14 )
          {
            LOBYTE(v85) = v104 == 12;
            goto LABEL_184;
          }
          if ( eventStatus != 44 )
          {
            if ( eventStatus != 15 )
              goto LABEL_202;
            if ( v104 == 2 )
            {
              (*(void (__fastcall **)(_QWORD, unsigned __int64))(**(_QWORD **)(this + 824) + 2504LL))(
                *(_QWORD *)(this + 824),
                v83);
              v107 = (char *)IOMalloc(511LL);
              if ( !v107 )
              {
LABEL_174:
                v83 = v87;
                (*(void (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 2680LL))(this, v87);
                goto LABEL_175;
              }
              v108 = v107;
              bzero(v107, 0x1FFuLL);
              snprintf(
                v108,
                0x1FFuLL,
                "[IOBluetoothHostController][ProcessEventDataWL] -- kBluetoothHCIErrorNoConnection -- Destroying device b"
                "ecause controller is saying this connection does not exist -- calling DestroyDeviceWithDisconnectionResults() \n");
              _os_log_internal(
                &dword_0,
                *(_QWORD *)(this + 1288),
                0LL,
                _ZZN25IOBluetoothHostController18ProcessEventDataWLEPhjjE11_os_log_fmt__16_,
                v108);
LABEL_156:
              IOFree(v108, 511LL);
              goto LABEL_174;
            }
LABEL_183:
            LOBYTE(v85) = 0;
LABEL_184:
            v106 = 0;
LABEL_203:
            if ( (*(unsigned __int8 (__fastcall **)(__int64, unsigned __int64))(*(_QWORD *)this + 3768LL))(this, v83) )
              goto LABEL_212;
            if ( !v226 || *(_BYTE *)(v226 + 2077) )
            {
              v129 = v228;
              v130 = v223;
              if ( !v106 )
              {
                (*(void (__fastcall **)(__int64, _QWORD, _QWORD, _QWORD, __int64, _QWORD, __int64, _QWORD, signed __int64))(*(_QWORD *)this + 2280LL))(
                  this,
                  (unsigned int)v218,
                  eventStatus,
                  v228,
                  v87,
                  v223,
                  v222,
                  0LL,
                  255LL);
                goto LABEL_212;
              }
              v131 = *(_QWORD *)this;
              v132 = eventStatus;
              v133 = this;
              v134 = (unsigned int)v218;
              v135 = v87;
              v206 = *((unsigned __int8 *)&loc_10208 + *(_QWORD *)(this + 824));
              goto LABEL_211;
            }
            v129 = v228;
            v130 = v223;
            v132 = eventStatus;
            if ( v106 )
            {
              v131 = *(_QWORD *)this;
              v133 = this;
              v134 = 0xFFFFFFFFLL;
              v135 = v87;
              v206 = *((unsigned __int8 *)&loc_10208 + *(_QWORD *)(this + 824));
LABEL_211:
              (*(void (__fastcall **)(__int64, signed __int64, __int64, __int64, __int64, __int64, __int64, signed __int64, __int64))(v131 + 2280))(
                v133,
                v134,
                v132,
                v129,
                v135,
                v130,
                v222,
                1LL,
                v206);
              goto LABEL_212;
            }
            (*(void (__fastcall **)(__int64, signed __int64, _QWORD, _QWORD, __int64, _QWORD, __int64, _QWORD, signed __int64))(*(_QWORD *)this + 2280LL))(
              this,
              0xFFFFFFFFLL,
              eventStatus,
              v228,
              v87,
              v223,
              v222,
              0LL,
              255LL);
LABEL_212:
            v18 = v212;
            if ( !v227 )
              goto LABEL_223;
            if ( v226 && eventStatus == 15 )
            {
              if ( (unsigned __int16)(v212 - 8205) <= 9u )
              {
                v136 = 577;
                if ( _bittest(&v136, v212 - 8205) )
                {
                  IOBluetoothHCIRequest::Complete(v226);
LABEL_223:
                  inDataPtr = inDataPtr;
                  if ( (_BYTE)v85 )
                  {
                    if ( *(_BYTE *)(this + 876) )
                    {
                      LOBYTE(v217) = 0;
                      v137 = v222;
                      (*(void (__fastcall **)(_QWORD, _QWORD, __int64 *))(**(_QWORD **)(this + 824) + 2856LL))(
                        *(_QWORD *)(this + 824),
                        (unsigned int)v222,
                        &v277);
                      if ( v137 != 64766 && v137 != 3092 )
                      {
                        v138 = (char *)IOMalloc(511LL);
                        if ( v138 )
                        {
                          v139 = v138;
                          bzero(v138, 0x1FFuLL);
                          snprintf(v139, 0x1FFuLL, "Received Command Disallowed (0x0C) error -- In UHE mode?", v211);
                          v140 = *(_QWORD *)(this + 824);
                          if ( v140 )
                          {
                            v141 = strlen(v139);
                            (*(void (__fastcall **)(__int64, signed __int64, char *, size_t))(*(_QWORD *)v140 + 2248LL))(
                              v140,
                              248LL,
                              v139,
                              v141);
                          }
                          IOFree(v139, 511LL);
                          v18 = v212;
                        }
                        (*(void (__fastcall **)(__int64, unsigned int *))(*(_QWORD *)this + 3944LL))(this, &v217);
                        if ( (_BYTE)v217 )
                        {
                          if ( (*(unsigned __int8 (__fastcall **)(__int64))(*(_QWORD *)this + 3752LL))(this) )
                          {
                            *(_DWORD *)v237 = 0;
                            LODWORD(v230) = 0;
                            (*(void (__fastcall **)(__int64))(*(_QWORD *)this + 3784LL))(this);
                            (*(void (__fastcall **)(__int64, __int64 *))(*(_QWORD *)this + 3792LL))(this, &v230);
                            v142 = (char *)IOMalloc(511LL);
                            if ( v142 )
                            {
                              v143 = v142;
                              bzero(v142, 0x1FFuLL);
                              snprintf(
                                v143,
                                0x1FFuLL,
                                "UHE Mode -- but Power_State_Change_In_Progress() returned true -- Current PowerState (%s"
                                ") -> Pending PowerState (%s)",
                                gInternalPowerStateString[*(unsigned int *)v237],
                                gInternalPowerStateString[(unsigned int)v230],
                                v211);
                              _os_log_internal(
                                &dword_0,
                                *(_QWORD *)(this + 1288),
                                0LL,
                                _ZZN25IOBluetoothHostController18ProcessEventDataWLEPhjjE11_os_log_fmt__19_,
                                v143);
                              v144 = *(_QWORD *)(this + 824);
                              if ( v144 )
                              {
                                v145 = strlen(v143);
                                (*(void (__fastcall **)(__int64, signed __int64, char *, size_t))(*(_QWORD *)v144
                                                                                                + 2248LL))(
                                  v144,
                                  248LL,
                                  v143,
                                  v145);
                              }
                              IOFree(v143, 511LL);
                              v18 = v212;
                            }
                            v146 = *(_QWORD *)(this + 832);
                            if ( v146 )
                              *(_BYTE *)(v146 + 312) = 1;
                          }
                          else
                          {
                            v151 = (char *)IOMalloc(511LL);
                            if ( v151 )
                            {
                              v152 = v151;
                              bzero(v151, 0x1FFuLL);
                              snprintf(v152, 0x1FFuLL, "UHE Mode -- Calling SetupController()", v211);
                              _os_log_internal(
                                &dword_0,
                                *(_QWORD *)(this + 1288),
                                0LL,
                                _ZZN25IOBluetoothHostController18ProcessEventDataWLEPhjjE11_os_log_fmt__18_,
                                v152);
                              v153 = *(_QWORD *)(this + 824);
                              if ( v153 )
                              {
                                v154 = strlen(v152);
                                (*(void (__fastcall **)(__int64, signed __int64, char *, size_t))(*(_QWORD *)v153
                                                                                                + 2248LL))(
                                  v153,
                                  248LL,
                                  v152,
                                  v154);
                              }
                              IOFree(v152, 511LL);
                              v18 = v212;
                            }
                            (*(void (__fastcall **)(__int64, const char *, signed __int64))(*(_QWORD *)this + 632LL))(
                              this,
                              "HCIResetHappenedDuringWake",
                              1LL);
                            (*(void (__fastcall **)(__int64, signed __int64, signed __int64))(*(_QWORD *)this + 3344LL))(
                              this,
                              1LL,
                              1LL);
                            *(_BYTE *)(this + 876) = 0;
                            (*(void (__fastcall **)(__int64, _QWORD))(*(_QWORD *)this + 2344LL))(this, 0LL);
                          }
                        }
                        else
                        {
                          v147 = (char *)IOMalloc(511LL);
                          if ( v147 )
                          {
                            v148 = v147;
                            bzero(v147, 0x1FFuLL);
                            snprintf(v148, 0x1FFuLL, "Not in UHE Mode -- continue", v211);
                            v149 = *(_QWORD *)(this + 824);
                            if ( v149 )
                            {
                              v150 = strlen(v148);
                              (*(void (__fastcall **)(__int64, signed __int64, char *, size_t))(*(_QWORD *)v149 + 2248LL))(
                                v149,
                                248LL,
                                v148,
                                v150);
                            }
                            IOFree(v148, 511LL);
                            v18 = v212;
                          }
                        }
                      }
                    }
                  }
                  if ( *(_BYTE *)(this + 1282) )
                  {
                    v155 = *(_QWORD *)(this + 832);
                    if ( v155 )
                    {
                      if ( !(unsigned __int8)IOService::isInactive((IOService *)v155) )
                        (*(void (__fastcall **)(__int64, signed __int64))(*(_QWORD *)this + 3248LL))(this, 1LL);
                    }
                  }
                  v19 = (unsigned int)v218;
LABEL_253:
                  v156 = v226;
                  v157 = v19;
                  (*(void (__fastcall **)(__int64, __int64, signed __int64, unsigned __int8 *))(*(_QWORD *)this + 3080LL))(
                    this,
                    v226,
                    v19,
                    inDataPtr);
                  if ( *(_BYTE *)(this + 959) && *(_BYTE *)(this + 972) )
                  {
                    v158 = (char *)IOMalloc(511LL);
                    if ( v158 )
                    {
                      v159 = v158;
                      bzero(v158, 0x1FFuLL);
                      v160 = *(unsigned __int8 *)(this + 912);
                      v209 = v226;
                      snprintf(
                        v159,
                        0x1FFuLL,
                        "[IOBluetoothHostController][ProcessEventDataWL] -- mWaitingForCompletedHCICommandsToSleep is TRU"
                        "E -- mNumberOfCommandsAllowedByHardware is %d -- requestID = %d,  opCode = 0x%04X (%s),  requestPtr = %p ****\n",
                        v160,
                        v157,
                        v18,
                        &v277,
                        v226);
                      v156 = 511LL;
                      IOFree(v159, 511LL);
                    }
                    if ( (*(unsigned __int8 (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 3184LL))(this, v156) )
                    {
                      v161 = (char *)IOMalloc(511LL);
                      if ( v161 )
                      {
                        v162 = v161;
                        bzero(v161, 0x1FFuLL);
                        v163 = v18;
                        v164 = *(unsigned __int8 *)(this + 912);
                        v165 = v163;
                        v210 = (*(unsigned int (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 4168LL))(this, v226);
                        snprintf(
                          v162,
                          0x1FFuLL,
                          "**** [IOBluetoothHostController][ProcessEventDataWL] -- No HCI commands in transit -- mNumberO"
                          "fCommandsAllowedByHardware is %d -- requestID = %d,  opCode = 0x%04X (%s),  requestPtr = 0x%04x ****\n",
                          v164,
                          v157,
                          v165,
                          &v277,
                          v210);
                        _os_log_internal(
                          &dword_0,
                          *(_QWORD *)(this + 1288),
                          0LL,
                          _ZZN25IOBluetoothHostController18ProcessEventDataWLEPhjjE11_os_log_fmt__20_,
                          v162);
                        IOFree(v162, 511LL);
                      }
                      *(_BYTE *)(this + 972) = 0;
                      if ( !*(_BYTE *)(this + 913) )
                        (*(void (__fastcall **)(_QWORD, char *))(**(_QWORD **)(this + 832) + 2376LL))(
                          *(_QWORD *)(this + 832),
                          v250);
                    }
                  }
                  return __stack_chk_guard;
                }
              }
            }
            else if ( !v226 )
            {
LABEL_221:
              if ( !v228 )
                (*(void (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 3120LL))(this, v222);
              goto LABEL_223;
            }
            (*(void (__fastcall **)(__int64))(*(_QWORD *)this + 3288LL))(this);
            IOBluetoothHCIRequest::Complete(v226);
            if ( *(_BYTE *)(v226 + 2780) )
              (*(void (__fastcall **)(__int64, _QWORD, _QWORD))(*(_QWORD *)this + 3136LL))(this, 0LL, (unsigned int)v218);
            goto LABEL_221;
          }
        }
        *(_BYTE *)(this + 884) = 1;
        v106 = 0;
        (*(void (__fastcall **)(_QWORD, signed __int64, _QWORD))(**(_QWORD **)(this + 208) + 488LL))(
          *(_QWORD *)(this + 208),
          *(_QWORD *)(this + 824) + 384LL,
          0LL);
        *(_BYTE *)(this + 518) = 0;
        v83 = this + 965;
        *(_BYTE *)(this + 965) = 1;
        v114 = *(_QWORD *)(this + 208);
        v115 = *(_QWORD *)v114;
LABEL_200:
        (*(void (__fastcall **)(__int64, unsigned __int64, _QWORD))(v115 + 488))(v114, v83, 0LL);
        goto LABEL_201;
      }
      switch ( eventStatus )
      {
        case 3u:
          goto LABEL_170;
        case 4u:
          *(_BYTE *)(this + 883) = 1;
          if ( *(_BYTE *)(v87 + 12) == 1 )
            *(_BYTE *)(this + 518) = 1;
          if ( *(_BYTE *)(*(_QWORD *)(this + 824) + 416LL) )
          {
            v83 = (unsigned __int64)v250;
            if ( (*(unsigned __int8 (__fastcall **)(__int64, char *))(*(_QWORD *)this + 3008LL))(this, v250) )
            {
              v109 = (char *)IOMalloc(511LL);
              if ( v109 )
              {
                v110 = v109;
                bzero(v109, 0x1FFuLL);
                snprintf(
                  v110,
                  0x1FFuLL,
                  "**** [IOBluetoothHostController][ProcessEventDataWL] -- The Connection Request Event caused full wake \n");
                v111 = *(_QWORD *)(this + 824);
                if ( v111 )
                {
                  v112 = strlen(v110);
                  (*(void (__fastcall **)(__int64, signed __int64, char *, size_t))(*(_QWORD *)v111 + 2248LL))(
                    v111,
                    248LL,
                    v110,
                    v112);
                }
                v83 = 511LL;
                IOFree(v110, 511LL);
              }
              (*(void (__fastcall **)(_QWORD, unsigned __int64))(**(_QWORD **)(this + 824) + 2480LL))(
                *(_QWORD *)(this + 824),
                v83);
            }
          }
          goto LABEL_175;
        case 5u:
          (*(void (__fastcall **)(_QWORD, unsigned __int64))(**(_QWORD **)(this + 824) + 2504LL))(
            *(_QWORD *)(this + 824),
            v83);
          goto LABEL_174;
        case 6u:
        case 7u:
        case 9u:
        case 0xAu:
        case 0xBu:
        case 0xCu:
        case 0xDu:
        case 0xFu:
        case 0x10u:
        case 0x12u:
          goto LABEL_202;
        case 8u:
          v83 = *(unsigned __int16 *)v87;
          v123 = (*(__int64 (__fastcall **)(__int64, unsigned __int64))(*(_QWORD *)this + 2544LL))(this, v83);
          if ( v123 )
          {
            v83 = *(unsigned __int8 *)(v87 + 2);
            (*(void (__fastcall **)(__int64, unsigned __int64))(*(_QWORD *)v123 + 2736LL))(v123, v83);
          }
          goto LABEL_175;
        case 0xEu:
          if ( (_DWORD)v222 != 4101 )
            goto LABEL_183;
          *(_QWORD *)(this + 286) = *(_QWORD *)v87;
          v124 = *(_WORD *)(this + 290);
          *(_WORD *)(this + 332) = v124;
          *(_DWORD *)(this + 334) = 0;
          if ( *(_BYTE *)(this + 282) )
          {
            *(_WORD *)(this + 336) = 2;
            v124 -= 2;
            *(_WORD *)(this + 332) = v124;
          }
          if ( *(_BYTE *)(this + 281) )
          {
            *(_WORD *)(this + 334) = v124 >> 1;
            *(_WORD *)(this + 332) = v124 - (v124 >> 1);
          }
          goto LABEL_175;
        case 0x11u:
          v83 = v87;
          (*(void (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 2760LL))(this, v87);
          goto LABEL_175;
        case 0x13u:
          v83 = v98;
          (*(void (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 2768LL))(this, v98);
          goto LABEL_175;
        case 0x14u:
          v83 = *(unsigned __int16 *)v87;
          v125 = (*(__int64 (__fastcall **)(__int64, unsigned __int64))(*(_QWORD *)this + 2544LL))(this, v83);
          if ( v125 )
          {
            v83 = *(unsigned __int8 *)(v87 + 2);
            (*(void (__fastcall **)(__int64, unsigned __int64, _QWORD))(*(_QWORD *)v125 + 2728LL))(
              v125,
              v83,
              *(unsigned __int16 *)(v87 + 4));
          }
          goto LABEL_175;
        default:
          if ( eventStatus == 44 )
          {
LABEL_170:
            if ( v103 )
            {
              v122 = 1;
              if ( (unsigned __int16)IOBluetoothHCIRequest::GetCommandOpCode(v103) != 1029 )
                v122 = (unsigned __int16)IOBluetoothHCIRequest::GetCommandOpCode(v226) == 64720;
            }
            else
            {
              v122 = 0;
            }
            *(_BYTE *)(this + 884) = 1;
            v126 = 0;
            (*(void (__fastcall **)(_QWORD, signed __int64, _QWORD))(**(_QWORD **)(this + 208) + 488LL))(
              *(_QWORD *)(this + 208),
              *(_QWORD *)(this + 824) + 384LL,
              0LL);
            *(_BYTE *)(this + 518) = 0;
            v127 = *(_QWORD *)(this + 832);
            if ( v127 )
            {
              v126 = *(_BYTE *)(v127 + 182) != 0;
              v128 = *(_BYTE *)(v127 + 181) != 0;
            }
            else
            {
              v128 = 0;
            }
            if ( v126 || v128 )
            {
              *(_DWORD *)v237 = -1431655766;
              if ( !(*(unsigned int (__fastcall **)(__int64, char *, _QWORD, signed __int64, _QWORD, _QWORD, _QWORD))(*(_QWORD *)this + 3128LL))(
                      this,
                      v237,
                      0LL,
                      5000LL,
                      0LL,
                      0LL,
                      0LL) )
              {
                (*(void (__fastcall **)(__int64, _QWORD, _QWORD, signed __int64, _QWORD))(*(_QWORD *)this + 4384LL))(
                  this,
                  *(unsigned int *)v237,
                  *(unsigned __int16 *)v87,
                  19LL,
                  0LL);
                (*(void (__fastcall **)(__int64, _QWORD, _QWORD))(*(_QWORD *)this + 3136LL))(
                  this,
                  0LL,
                  *(unsigned int *)v237);
              }
              v228 = 4;
            }
            else
            {
              (*(void (__fastcall **)(__int64, __int64, bool))(*(_QWORD *)this + 2672LL))(this, v87, v122);
            }
            v83 = this + 965;
            *(_BYTE *)(this + 965) = 1;
            v114 = *(_QWORD *)(this + 208);
            v115 = *(_QWORD *)v114;
            v106 = 0;
            goto LABEL_200;
          }
          if ( eventStatus != 62 )
            goto LABEL_202;
          v116 = *(_BYTE *)(v98 + 2);
          if ( v116 == 10 )
            goto LABEL_165;
          if ( v116 != 2 )
          {
            if ( v116 != 1 )
              goto LABEL_175;
LABEL_165:
            v219 = (void *)v87;
            v117 = *(_WORD *)(v98 + 4) & 0xFFF;
            v83 = *(_WORD *)(v98 + 4) & 0xFFF;
            if ( !(*(unsigned int (__fastcall **)(__int64, unsigned __int64))(*(_QWORD *)this + 2600LL))(this, v83) )
              goto LABEL_277;
            v83 = (unsigned __int16)v117;
            v118 = (*(__int64 (__fastcall **)(__int64, _QWORD))(*(_QWORD *)this + 2592LL))(this, (unsigned __int16)v117);
            if ( !v118 )
              goto LABEL_277;
            v119 = *(_BYTE *)(v118 + 2);
            v120 = (char *)IOMalloc(511LL);
            v121 = v120;
            if ( v119 )
            {
              if ( v120 )
              {
                bzero(v120, 0x1FFuLL);
                snprintf(
                  v121,
                  0x1FFuLL,
                  "[IOBluetoothHostController][ProcessEventDataWL] -- kBluetoothHCIEventLEMetaEvent -- subEventCode is kB"
                  "luetoothHCISubEventLEConnectionComplete -- AddLEDevice() for connection ID 0x%02X failed because bluet"
                  "oothd has not called CreateDeviceFromLEConnectionResults() yet, but Disconnection event for connection"
                  " handle (0x%02X) has been received\n",
                  v117,
                  v117);
                _os_log_internal(
                  &dword_0,
                  *(_QWORD *)(this + 1288),
                  0LL,
                  _ZZN25IOBluetoothHostController18ProcessEventDataWLEPhjjE11_os_log_fmt__10_,
                  v121);
LABEL_274:
                v173 = *(_QWORD *)(this + 824);
                if ( v173 )
                {
                  v174 = strlen(v121);
                  (*(void (__fastcall **)(__int64, signed __int64, char *, size_t))(*(_QWORD *)v173 + 2248LL))(
                    v173,
                    250LL,
                    v121,
                    v174);
                }
                v83 = 511LL;
                IOFree(v121, 511LL);
                goto LABEL_277;
              }
            }
            else if ( v120 )
            {
              bzero(v120, 0x1FFuLL);
              snprintf(
                v121,
                0x1FFuLL,
                "[IOBluetoothHostController][ProcessEventDataWL] -- kBluetoothHCIEventLEMetaEvent -- subEventCode is kBlu"
                "etoothHCISubEventLEConnectionComplete -- AddLEDevice() for connection ID 0x%02X failed because kBluetoot"
                "hHCISubEventLEConnectionComplete for connection handle (0x%02X) has been received twice without being di"
                "sconnected first.\n",
                (unsigned __int16)v117,
                (unsigned __int16)v117);
              _os_log_internal(
                &dword_0,
                *(_QWORD *)(this + 1288),
                0LL,
                _ZZN25IOBluetoothHostController18ProcessEventDataWLEPhjjE11_os_log_fmt__11_,
                v121);
              goto LABEL_274;
            }
LABEL_277:
            *(_DWORD *)(this + 1300) = (*(__int64 (__fastcall **)(_QWORD, unsigned __int64))(**(_QWORD **)(this + 824)
                                                                                         + 2592LL))(
                                       *(_QWORD *)(this + 824),
                                       v83);
            if ( *(_BYTE *)(this + 1283) )
            {
              v175 = *((_WORD *)inDataPtr + 2);
              v176 = (char *)IOMalloc(511LL);
              if ( v176 )
              {
                v177 = v176;
                bzero(v176, 0x1FFuLL);
                snprintf(
                  v177,
                  0x1FFuLL,
                  "[IOBluetoothHostController][ProcessEventDataWL] -- ACL data for handle 0x%04x was received before the "
                  "LEConnectionComplete event -- calling commandWakeup()\n",
                  v175 & 0xFFF);
                v178 = *(_QWORD *)(this + 824);
                if ( v178 )
                {
                  v179 = strlen(v177);
                  (*(void (__fastcall **)(__int64, signed __int64, char *, size_t))(*(_QWORD *)v178 + 2248LL))(
                    v178,
                    248LL,
                    v177,
                    v179);
                }
                IOFree(v177, 511LL);
              }
              v83 = this + 1283;
              (*(void (__fastcall **)(_QWORD, __int64, _QWORD))(**(_QWORD **)(this + 208) + 488LL))(
                *(_QWORD *)(this + 208),
                this + 1283,
                0LL);
            }
            v106 = 1;
            if ( *(_BYTE *)(*(_QWORD *)(this + 824) + 416LL) )
            {
              v83 = (unsigned __int64)v250;
              (*(void (__fastcall **)(__int64, char *))(*(_QWORD *)this + 3008LL))(this, v250);
            }
            goto LABEL_347;
          }
          v219 = (void *)v87;
          v166 = *(unsigned __int8 *)(v98 + 1);
          v167 = *(unsigned __int8 *)(v98 + 3);
          if ( 10 * (signed int)v167 + 2 <= v166 )
          {
            if ( (unsigned __int8)(v167 - 1) < 25u )
            {
              v180 = v167 + *(_QWORD *)(this + 1096);
              *(_QWORD *)(this + 1096) = v180;
              if ( *(_BYTE *)(this + 1112) )
              {
                v103 = *(_QWORD *)(this + 176);
                v83 = 0x4164527074437472LL;
                IOSimpleReporter::setValue((IOSimpleReporter *)v103, 0x4164527074437472uLL, v180);
              }
              if ( (unsigned int)inDataSize < 5 )
              {
                v184 = v166 - 2;
                LOWORD(v85) = 4;
              }
              else
              {
                LOWORD(v85) = 2;
                do
                {
                  v181 = v85;
                  v182 = (unsigned __int8)v85 < (unsigned __int8)v167;
                  v183 = (_BYTE)v85 == (_BYTE)v167;
                  LODWORD(v85) = v85 + 1;
                }
                while ( (v182 || v183) && (unsigned __int16)(v181 + 3) < (unsigned int)inDataSize );
                v184 = v166 - v85;
                LODWORD(v85) = v85 + 2;
              }
              v185 = (unsigned __int16)v85;
              if ( (unsigned __int16)v85 >= (unsigned int)inDataSize )
              {
                v187 = inDataSize;
              }
              else
              {
                v186 = 2;
                v187 = inDataSize;
                do
                {
                  --v184;
                  LODWORD(v85) = v85 + 1;
                  v185 = (unsigned __int16)v85;
                  if ( v186 > (unsigned __int8)v167 )
                    break;
                  ++v186;
                }
                while ( (unsigned __int16)v85 < (unsigned int)inDataSize );
              }
              if ( v185 + 5 < v187 )
              {
                v188 = 2;
                do
                {
                  v184 -= 6;
                  LODWORD(v85) = v85 + 6;
                  if ( v188 > (unsigned __int8)v167 )
                    break;
                  ++v188;
                }
                while ( (unsigned int)(unsigned __int16)v85 + 5 < (unsigned int)inDataSize );
              }
              v216 = (__int64)&v211;
              v189 = alloca(__chkstk_darwin(v103, v83));
              v190 = 0LL;
              do
                *((_BYTE *)&v211 + v190++) = -86;
              while ( v167 != v190 );
              if ( (unsigned __int16)v85 >= (unsigned int)inDataSize )
              {
                v192 = 0;
              }
              else
              {
                v191 = 2;
                v192 = 0;
                v193 = inDataSize;
                v194 = inDataPtr;
                do
                {
                  v83 = v194[(unsigned __int16)v85];
                  *((_BYTE *)&v211 + (unsigned __int8)(v191 - 1) - 1) = v83;
                  v192 += v83;
                  --v184;
                  LODWORD(v85) = v85 + 1;
                  if ( v191 > (unsigned __int8)v167 )
                    break;
                  ++v191;
                }
                while ( (unsigned __int16)v85 < v193 );
              }
              if ( (_DWORD)v167 + v192 == v184 )
              {
                LODWORD(v195) = (unsigned __int16)v85;
                if ( (unsigned __int16)v85 < (unsigned int)inDataSize )
                {
                  v196 = 1LL;
                  v213 = 4713424174761212244LL;
                  v211 = 6081394863561135207LL;
                  do
                  {
                    if ( *((_BYTE *)&v211 + v196 - 1) >= 8u )
                    {
                      v195 = (unsigned int)v195;
                      if ( inDataPtr[(unsigned int)v195 + 4] == -1 && inDataPtr[v195 + 5] == 76 && *(_BYTE *)(this + 1112) )
                      {
                        switch ( inDataPtr[(unsigned __int8)(v85 + 7)] )
                        {
                          case 1u:
                            v197 = *(_QWORD *)(this + 176);
                            v198 = *(_QWORD *)(this + 976) + 1LL;
                            *(_QWORD *)(this + 976) = v198;
                            v83 = 0x4861736800000000LL;
                            goto LABEL_334;
                          case 2u:
                            v197 = *(_QWORD *)(this + 176);
                            v198 = *(_QWORD *)(this + 984) + 1LL;
                            *(_QWORD *)(this + 984) = v198;
                            v83 = 7584736191399816704LL;
                            goto LABEL_334;
                          case 3u:
                            v197 = *(_QWORD *)(this + 176);
                            v198 = *(_QWORD *)(this + 992) + 1LL;
                            *(_QWORD *)(this + 992) = v198;
                            v83 = 5138698434294841344LL;
                            goto LABEL_334;
                          case 4u:
                            v197 = *(_QWORD *)(this + 176);
                            v198 = *(_QWORD *)(this + 1000) + 1LL;
                            *(_QWORD *)(this + 1000) = v198;
                            v83 = 4707482426693416304LL;
                            goto LABEL_334;
                          case 5u:
                            v197 = *(_QWORD *)(this + 176);
                            v198 = *(_QWORD *)(this + 1008) + 1LL;
                            *(_QWORD *)(this + 1008) = v198;
                            v83 = 4713424123323183104LL;
                            goto LABEL_334;
                          case 6u:
                            v197 = *(_QWORD *)(this + 176);
                            v198 = *(_QWORD *)(this + 1016) + 1LL;
                            *(_QWORD *)(this + 1016) = v198;
                            v83 = 5219510774970020864LL;
                            goto LABEL_334;
                          case 7u:
                            v197 = *(_QWORD *)(this + 176);
                            v198 = *(_QWORD *)(this + 1024) + 1LL;
                            *(_QWORD *)(this + 1024) = v198;
                            v83 = 5796818232914569586LL;
                            goto LABEL_334;
                          case 8u:
                            v197 = *(_QWORD *)(this + 176);
                            v198 = *(_QWORD *)(this + 1032) + 1LL;
                            *(_QWORD *)(this + 1032) = v198;
                            v83 = 5216709142536939776LL;
                            goto LABEL_334;
                          case 9u:
                            v197 = *(_QWORD *)(this + 176);
                            v198 = *(_QWORD *)(this + 1040) + 1LL;
                            *(_QWORD *)(this + 1040) = v198;
                            v83 = v213;
                            goto LABEL_334;
                          case 0xAu:
                            v197 = *(_QWORD *)(this + 176);
                            v198 = *(_QWORD *)(this + 1048) + 1LL;
                            *(_QWORD *)(this + 1048) = v198;
                            v83 = 4713424174761212243LL;
                            goto LABEL_334;
                          case 0xBu:
                            v197 = *(_QWORD *)(this + 176);
                            v198 = *(_QWORD *)(this + 1056) + 1LL;
                            *(_QWORD *)(this + 1056) = v198;
                            v83 = 5575851515997026153LL;
                            goto LABEL_334;
                          case 0xCu:
                            v197 = *(_QWORD *)(this + 176);
                            v198 = *(_QWORD *)(this + 1064) + 1LL;
                            *(_QWORD *)(this + 1064) = v198;
                            v83 = 4859223969220162921LL;
                            goto LABEL_334;
                          case 0xDu:
                            v197 = *(_QWORD *)(this + 176);
                            v198 = *(_QWORD *)(this + 1072) + 1LL;
                            *(_QWORD *)(this + 1072) = v198;
                            v83 = v211;
                            goto LABEL_334;
                          case 0xEu:
                            v197 = *(_QWORD *)(this + 176);
                            v198 = *(_QWORD *)(this + 1080) + 1LL;
                            *(_QWORD *)(this + 1080) = v198;
                            v83 = 6081394863561134962LL;
                            goto LABEL_334;
                          case 0xFu:
                            v197 = *(_QWORD *)(this + 176);
                            v198 = *(_QWORD *)(this + 1088) + 1LL;
                            *(_QWORD *)(this + 1088) = v198;
                            v83 = 3707135739674391673LL;
LABEL_334:
                            IOSimpleReporter::setValue((IOSimpleReporter *)v197, v83, v198);
                            break;
                          default:
                            break;
                        }
                      }
                    }
                    LODWORD(v85) = *((unsigned __int8 *)&v211 + v196 - 1) + (_DWORD)v85;
                    LODWORD(v195) = (unsigned __int16)v85;
                    if ( v196 >= v167 )
                      break;
                    ++v196;
                  }
                  while ( (unsigned __int16)v85 < (unsigned int)inDataSize );
                }
                if ( (unsigned int)v195 < (unsigned int)inDataSize )
                {
                  v199 = v85 + 1;
                  v200 = 2;
                  do
                  {
                    if ( v200 > (unsigned __int8)v167 )
                      break;
                    v201 = (unsigned __int16)v199++;
                    ++v200;
                  }
                  while ( v201 < (unsigned int)inDataSize );
                }
              }
              else
              {
                v202 = (char *)IOMalloc(511LL);
                if ( v202 )
                {
                  v203 = v202;
                  bzero(v202, 0x1FFuLL);
                  snprintf(
                    v203,
                    0x1FFuLL,
                    "[IOBluetoothHostController][ProcessEventDataWL] -- Number of bytes in the packets does not match the"
                    " packet length\n");
                  _os_log_internal(
                    &dword_0,
                    *(_QWORD *)(this + 1288),
                    0LL,
                    _ZZN25IOBluetoothHostController18ProcessEventDataWLEPhjjE11_os_log_fmt__15_,
                    v203);
                  v204 = *(_QWORD *)(this + 824);
                  if ( v204 )
                  {
                    v205 = strlen(v203);
                    (*(void (__fastcall **)(__int64, signed __int64, char *, size_t))(*(_QWORD *)v204 + 2248LL))(
                      v204,
                      250LL,
                      v203,
                      v205);
                  }
                  v83 = 511LL;
                  IOFree(v203, 511LL);
                }
              }
              goto LABEL_346;
            }
            v170 = (char *)IOMalloc(511LL);
            if ( v170 )
            {
              v169 = v170;
              bzero(v170, 0x1FFuLL);
              snprintf(
                v169,
                0x1FFuLL,
                "[IOBluetoothHostController][ProcessEventDataWL] -- Number of reports is not within the spec\n");
              _os_log_internal(
                &dword_0,
                *(_QWORD *)(this + 1288),
                0LL,
                _ZZN25IOBluetoothHostController18ProcessEventDataWLEPhjjE11_os_log_fmt__14_,
                v169);
              goto LABEL_269;
            }
          }
          else
          {
            v168 = (char *)IOMalloc(511LL);
            if ( v168 )
            {
              v169 = v168;
              bzero(v168, 0x1FFuLL);
              snprintf(
                v169,
                0x1FFuLL,
                "[IOBluetoothHostController][ProcessEventDataWL] -- Bluetooth Firmware Error!! Not enough data to be a va"
                "lid LE Advertising Report\n");
              _os_log_internal(
                &dword_0,
                *(_QWORD *)(this + 1288),
                0LL,
                _ZZN25IOBluetoothHostController18ProcessEventDataWLEPhjjE11_os_log_fmt__13_,
                v169);
LABEL_269:
              v171 = *(_QWORD *)(this + 824);
              if ( v171 )
              {
                v172 = strlen(v169);
                (*(void (__fastcall **)(__int64, signed __int64, char *, size_t))(*(_QWORD *)v171 + 2248LL))(
                  v171,
                  250LL,
                  v169,
                  v172);
              }
              v83 = 511LL;
              IOFree(v169, 511LL);
              goto LABEL_346;
            }
          }
LABEL_346:
          v106 = 0;
LABEL_347:
          v87 = (__int64)v219;
          goto LABEL_201;
      }
    }
    v48 = inDataSize;
    if ( v44 == 64694 )
    {
      v49 = (char *)IOMalloc(511LL);
      v48 = inDataSize;
      if ( v49 )
      {
        v50 = v49;
        bzero(v49, 0x1FFuLL);
        snprintf(
          v50,
          0x1FFuLL,
          "[IOBluetoothHostController][ProcessEventDataWL] -- BFC Resume failed with error = 0x%x (%s) \n",
          v228,
          &v263);
        _os_log_internal(
          &dword_0,
          *(_QWORD *)(this + 1288),
          0LL,
          IOBluetoothHostController::ProcessEventDataWL(unsigned char *,unsigned int,unsigned int)::_os_log_fmt,
          v50);
        IOFree(v50, 511LL);
        v48 = inDataSize;
      }
    }
    goto LABEL_75;
}
