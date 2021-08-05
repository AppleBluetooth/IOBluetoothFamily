
IOReturn IOBluetoothHostControllerUARTTransport::setPowerStateWL(unsigned long powerStateOrdinal, IOService *a3)
{
  __int64 v4; // rax
  const char *v5; // r8
  __int64 v6; // rbx
  size_t v7; // rax
  int v8; // ecx
  signed int v9; // eax
  __int64 v10; // rdi
  bool v11; // al
  __int64 v12; // rcx
  _QWORD *v13; // rdx
  bool v14; // cl
  __int64 v15; // rdx
  __int64 v16; // rbx
  size_t v17; // rax
  _DWORD *v18; // r14
  int v19; // eax
  __int64 v20; // rbx
  size_t v21; // rax
  char *v22; // r15
  const char *v23; // r13
  __int64 v24; // rbx
  size_t v25; // rax
  bool v26; // al
  __int64 v27; // rbx
  size_t v28; // rax
  __int64 v29; // rbx
  size_t v30; // rax
  bool v31; // al
  _DWORD *v32; // rdi
  __int64 v33; // rbx
  size_t v34; // rax
  __int64 v35; // rbx
  size_t v36; // rax
  __int64 v37; // rdi
  __int64 v38; // rdi
  __int64 v39; // rbx
  size_t v40; // rax
  const char *v42; // [rsp+8h] [rbp-1F8h]
  unsigned int v43; // [rsp+14h] [rbp-1ECh]
  
  __int64 v45; // [rsp+20h] [rbp-1E0h]
  __int64 v46; // [rsp+28h] [rbp-1D8h]
  __int64 v47; // [rsp+30h] [rbp-1D0h]
  __int64 v48; // [rsp+38h] [rbp-1C8h]
  __int64 v49; // [rsp+40h] [rbp-1C0h]
  __int64 v50; // [rsp+48h] [rbp-1B8h]
  char v51[15]; // [rsp+50h] [rbp-1B0h]
  char v53[8]; // [rsp+160h] [rbp-A0h]
  __int64 v54; // [rsp+168h] [rbp-98h]
  __int64 v55; // [rsp+170h] [rbp-90h]
  __int64 v56; // [rsp+178h] [rbp-88h]
  __int64 v57; // [rsp+180h] [rbp-80h]
  __int64 v58; // [rsp+188h] [rbp-78h]
  __int64 v59; // [rsp+190h] [rbp-70h]
  __int64 v60; // [rsp+198h] [rbp-68h]
  __int64 v61; // [rsp+1A0h] [rbp-60h]
  __int64 v62; // [rsp+1A8h] [rbp-58h]
  __int64 v63; // [rsp+1B0h] [rbp-50h]
  __int64 v64; // [rsp+1B8h] [rbp-48h]
  int v65; // [rsp+1C0h] [rbp-40h]

    v43 = 0;
    v4 = *(unsigned int *)(this + 216);
    *(bool *)(this + 368) = 1;
    mBluetoothFamilyLogPacket(251, "%s->%s+\n", gInternalPowerStateString[mCurrentInternalPowerState], powerStateOrdinal ? "ON" : "OFF");
    
    v5 = "ON";
    if ( !powerStateOrdinal )
        v5 = "OFF";
    
    *(UInt32 *)(this + 244) = 0;
    
    v9 = mPendingInternalPowerState;
    if ( mCurrentInternalPowerState == mPendingInternalPowerState )
    {
        v9 = mCurrentInternalPowerState;
        if ( powerStateOrdinal )
        {
            if ( powerStateOrdinal == 1 )
            {
                mPendingInternalPowerState = kIOBluetoothHCIControllerInternalPowerStateOn;
                v9 = 1;
            }
            goto LABEL_23;
        }
        if ( mCurrentInternalPowerState == 1 )
        {
            if ( *(UInt8 *)(mBluetoothController + 970) && *(UInt8 *)(mBluetoothController + 971) )
            {
                mPendingInternalPowerState = kIOBluetoothHCIControllerInternalPowerStateOn;
                *(UInt8 *)(mBluetoothController + 971) = 0;
                v9 = 1;
                goto LABEL_23;
            }
            if ((mBluetoothController->mControllerPowerOptions & 6) != 6
            ||  mBluetoothController->BluetoothRemoteWakeEnabled() ^ 1
            ||  *(UInt8 *)(this + 248) != 0
            ||  (mBluetoothFamily->mCurrentBluetoothHardware && mBluetoothFamily->mCurrentBluetoothHardware->mBluetoothTransport != this)
            ||  *(UInt8 *)(mBluetoothFamily + 383)
            ||  !mIsControllerActive
            ||  ((mSleepType | 2) == 6) )
            {
                mPendingInternalPowerState = kIOBluetoothHCIControllerInternalPowerStateOff;
                *(UInt8 *)(mBluetoothController + 966) = 0;
                v9 = 0;
            }
            else
            {
                mPendingInternalPowerState = kIOBluetoothHCIControllerInternalPowerStateSleep;
                *(_BYTE *)(this + 244) |= 3;
                v9 = 2;
            }
        }
    }
LABEL_23:
    mBluetoothFamilyLogPacket(251, "%s->%s\n", gInternalPowerStateString[mCurrentInternalPowerState], gInternalPowerStateString[v9]);
  
  
    v18 = (_DWORD *)(this + 216);
    v19 = *(_DWORD *)(this + 220);
    if ( mPendingInternalPowerState )
    {
        if ( !powerStateOrdinal && mPendingInternalPowerState == 2 )
        {
            StopLMPLogging();
            WaitForSystemReadyForSleep((char *) "IOBluetoothHostControllerUARTTransport::setPowerStateWL()");
            if ( mBluetoothController->CleanupForPowerChangeFromOnToSleep(*(bool *)(this + 249), &v43) )
            {
                    mBluetoothFamilyLogPacket(250, "Power state transition to SLEEP failed\n");
                    v18 = (_DWORD *)(this + 216);
                    *(_BYTE *)(this + 312) = 1;
            }
            mBluetoothController->CallCancelTimeoutForIdleTimer();
            mCurrentInternalPowerState = kIOBluetoothHCIControllerInternalPowerStateSleep;
            v22 = gInternalPowerStateString[mCurrentInternalPowerState];
            v23 = v5;
            if ( !v43 && *(_BYTE *)(this + 192) )
            {
                (*(void (__fastcall **)(__int64))(*(_QWORD *)this + 3024LL))(this);
                (*(void (__fastcall **)(__int64, signed __int64))(*(_QWORD *)this + 2680LL))(this, 1LL);
                *(_BYTE *)(this + 192) = 0;
            }
      if ( *(_BYTE *)(this + 240) )
        (*(void (__fastcall **)(_QWORD, signed __int64, signed __int64))(**(_QWORD **)(this + 144) + 3728LL))(
          *(_QWORD *)(this + 144),
          2LL,
          1LL);
      (*(void (__fastcall **)(_QWORD, _DWORD *, _QWORD))(**(_QWORD **)(this + 168) + 488LL))(
        *(_QWORD *)(this + 168),
        v18,
        0LL);
      (*(void (__fastcall **)(_QWORD, _QWORD))(**(_QWORD **)(this + 144) + 2424LL))(
        *(_QWORD *)(this + 144),
        *(unsigned int *)(this + 216));
      *(_BYTE *)(*(_QWORD *)(this + 136) + 416LL) = 1;
      goto LABEL_94;
    }
    if ( *v18 == 2 )
    {
      v22 = gInternalPowerStateString[mCurrentInternalPowerState];
      v23 = v5;
      if ( v19 != 1 )
        goto LABEL_94;
      if ( *(_BYTE *)(this + 312) )
      {
        *(_BYTE *)(this + 312) = 0;
          mBluetoothFamilyLogPacket(250, "Power state transition to SLEEP failed, doing a hard reset to recover \n");
          v18 = (_DWORD *)(this + 216);
          v22 = gInternalPowerStateString[mCurrentInternalPowerState];
        
          *(_DWORD *)(this + 216) = 1;
          (*(void (__fastcall **)(_QWORD, const char *, signed __int64))(**(_QWORD **)(this + 144) + 632LL))(
          *(_QWORD *)(this + 144),
          "HCIResetHappenedDuringWake",
          1LL);
        (*(void (__fastcall **)(_QWORD, signed __int64))(**(_QWORD **)(this + 144) + 4048LL))(*(_QWORD *)(this + 144), 1LL);
      }
      else
      {
        (*(void (__fastcall **)(__int64, _QWORD))(*(_QWORD *)this + 2680LL))(this, 0LL);
        if ( !(*(unsigned int (__fastcall **)(__int64, _QWORD))(*(_QWORD *)this + 3016LL))(this, 0LL) )
        {
          *(_DWORD *)(this + 216) = 1;
          *(_BYTE *)(this + 192) = 1;
          if ( (*(unsigned __int8 (**)(void))(**(_QWORD **)(this + 144) + 2368LL))()
            && *(_DWORD *)(*(_QWORD *)(this + 144) + 904LL) )
          {
              os_log(mInternalOSLogObject, "IOBluetoothHostControllerUARTTransport::setPowerStateWL -- SLEEP -> ON -- mControllerConfigState != kIOBluetoothHCIControllerConfigStateOnline -- calling SetupController()");
              mBluetoothFamilyLogPacket(251, "mControllerConfigState != kIOBluetoothHCIControllerConfigStateOnline -- calling mBluetoothController->SetupController()\n");
              v22 = gInternalPowerStateString[mCurrentInternalPowerState];
              v18 = (_DWORD *)(this + 216);
              (*(void (__fastcall **)(_QWORD, _QWORD))(**(_QWORD **)(this + 144) + 2344LL))(*(_QWORD *)(this + 144), 0LL);
          }
          (*(void (__fastcall **)(__int64))(*(_QWORD *)this + 2224LL))(this);
        }
      }
      (*(void (**)(void))(**(_QWORD **)(this + 144) + 3896LL))();
      (*(void (__fastcall **)(_QWORD, signed __int64, _QWORD))(**(_QWORD **)(this + 168) + 488LL))(
        *(_QWORD *)(this + 168),
        *(_QWORD *)(this + 144) + 967LL,
        0LL);
      (*(void (__fastcall **)(__int64, signed __int64))(*(_QWORD *)this + 2592LL))(this, 1LL);
    }
    else
    {
      v22 = gInternalPowerStateString[mCurrentInternalPowerState];
      v23 = v5;
      if ( *v18 || v19 != 1 )
        goto LABEL_94;
      if ( *(_BYTE *)(this + 312) )
      {
        *(_BYTE *)(this + 312) = 0;
          mBluetoothFamilyLogPacket(250, "Power state transition to OFF failed, doing a hard reset to recover \n");
          v18 = (_DWORD *)(this + 216);
          v22 = gInternalPowerStateString[mCurrentInternalPowerState];
          *(_DWORD *)(this + 216) = 1;
          (*(void (__fastcall **)(_QWORD, signed __int64))(**(_QWORD **)(this + 144) + 4048LL))(*(_QWORD *)(this + 144), 1LL);
      }
      else
      {
        *(_DWORD *)(this + 220) = 1;
        if ( !*(_BYTE *)(this + 192) )
        {
          (*(void (__fastcall **)(__int64))(*(_QWORD *)this + 3024LL))(this);
          (*(void (__fastcall **)(__int64, _QWORD))(*(_QWORD *)this + 2680LL))(this, 0LL);
          v31 = 1;
          if ( !*(_BYTE *)(this + 369) )
            v31 = *(_BYTE *)(*(_QWORD *)(this + 136) + 434LL) != 0;
          (*(void (__fastcall **)(__int64, bool))(*(_QWORD *)this + 2672LL))(this, v31);
          (*(void (__fastcall **)(__int64))(*(_QWORD *)this + 2648LL))(this);
          if ( !(*(unsigned int (__fastcall **)(__int64, _QWORD))(*(_QWORD *)this + 3016LL))(this, 0LL) )
          {
            *(_BYTE *)(this + 192) = 1;
            *(_DWORD *)(this + 216) = 1;
            if ( (*(unsigned __int8 (**)(void))(**(_QWORD **)(this + 144) + 2368LL))() )
            {
              v32 = *(_DWORD **)(this + 144);
              if ( v32[226] )
              {
                  os_log(mInternalOSLogObject, "IOBluetoothHostControllerUARTTransport::setPowerStateWL -- OFF -> ON -- mControllerConfigState != kIOBluetoothHCIControllerConfigStateOnline -- calling SetupController()");
                  mBluetoothFamilyLogPacket(251, "mControllerConfigState != kIOBluetoothHCIControllerConfigStateOnline -- calling mBluetoothController->SetupController()\n");
                  v18 = (_DWORD *)(this + 216);
                  v22 = gInternalPowerStateString[mCurrentInternalPowerState];
                  (*(void (__fastcall **)(_QWORD, _QWORD))(**(_QWORD **)(this + 144) + 2344LL))(*(_QWORD *)(this + 144), 0LL);
                  v32 = *(_DWORD **)(this + 144);
              }
              strcpy(v51, "StateWL -- ON");
              (*(void (__fastcall **)(_DWORD *, signed __int64))(*(_QWORD *)v32 + 4032LL))(v32, 1LL);
            }
            else if ( *(_BYTE *)(this + 184) )
            {
                os_log(mInternalOSLogObject, "IOBluetoothHostControllerUARTTransport::setPowerStateWL -- The very first Power On from PowerManager -- calling SetupController()");
                (*(void (__fastcall **)(_QWORD, _QWORD))(**(_QWORD **)(this + 144) + 2344LL))(*(_QWORD *)(this + 144), 0LL);
            }
            else
            {
              *(_BYTE *)(this + 184) = 1;
            }
          }
        }
        (*(void (__fastcall **)(__int64))(*(_QWORD *)this + 2224LL))(this);
        *(_DWORD *)(this + 216) = 1;
      }
      (*(void (__fastcall **)(__int64, signed __int64))(*(_QWORD *)this + 2592LL))(this, 1LL);
      if ( !*(_BYTE *)(this + 192) )
        goto LABEL_90;
    }
    if ( *(_BYTE *)(this + 240) )
      (*(void (__fastcall **)(_QWORD, _QWORD, signed __int64))(**(_QWORD **)(this + 144) + 3728LL))(
        *(_QWORD *)(this + 144),
        *(unsigned int *)(this + 216),
        1LL);
LABEL_90:
    v37 = *(_QWORD *)(this + 144);
    if ( v37 )
    {
      (*(void (__fastcall **)(__int64, _QWORD))(*(_QWORD *)v37 + 2424LL))(v37, *(unsigned int *)(this + 216));
      v38 = *(_QWORD *)(this + 144);
      if ( v38 )
        (*(void (__fastcall **)(__int64, _QWORD))(*(_QWORD *)v38 + 3824LL))(v38, 0LL);
    }
    (*(void (__fastcall **)(_QWORD, _DWORD *, _QWORD))(**(_QWORD **)(this + 168) + 488LL))(
      *(_QWORD *)(this + 168),
      v18,
      0LL);
    goto LABEL_94;
  }
  v22 = gInternalPowerStateString[mCurrentInternalPowerState];
  v23 = v5;
  if ( *(_BYTE *)(this + 240) )
  {
    strcpy(v51, "StateWL -- OFF");
    (*(void (**)(void))(**(_QWORD **)(this + 144) + 3616LL))();
    if ( (*(unsigned int (__fastcall **)(_QWORD, _QWORD, __int64 *))(**(_QWORD **)(this + 144) + 4032LL))(
           *(_QWORD *)(this + 144),
           0LL,
           &v45) )
    {
        mBluetoothFamilyLogPacket(250, "Power state transition to OFF failed\n");
        *(_BYTE *)(this + 312) = 1;
    }
  }
  (*(void (__fastcall **)(__int64))(*(_QWORD *)this + 3048LL))(this);
  if ( *(_BYTE *)(this + 192) )
  {
    (*(void (__fastcall **)(__int64))(*(_QWORD *)this + 3024LL))(this);
    (*(void (__fastcall **)(__int64, _QWORD))(*(_QWORD *)this + 2680LL))(this, 0LL);
    v26 = 1;
    if ( !*(_BYTE *)(this + 369) )
      v26 = *(_BYTE *)(*(_QWORD *)(this + 136) + 434LL) != 0;
    (*(void (__fastcall **)(__int64, bool))(*(_QWORD *)this + 2672LL))(this, v26);
    (*(void (__fastcall **)(__int64))(*(_QWORD *)this + 2664LL))(this);
    *(_BYTE *)(this + 192) = 0;
  }
  (*(void (__fastcall **)(__int64, _QWORD))(*(_QWORD *)this + 2592LL))(this, 0LL);
  *(_DWORD *)(this + 216) = 0;
  if ( *(_BYTE *)(this + 240) )
    (*(void (__fastcall **)(_QWORD, _QWORD, signed __int64))(**(_QWORD **)(this + 144) + 3728LL))(
      *(_QWORD *)(this + 144),
      0LL,
      1LL);
  (*(void (__fastcall **)(_QWORD, __int64, _QWORD))(**(_QWORD **)(this + 168) + 488LL))(
    *(_QWORD *)(this + 168),
    this + 216,
    0LL);
  (*(void (__fastcall **)(_QWORD, _QWORD))(**(_QWORD **)(this + 144) + 2424LL))(
    *(_QWORD *)(this + 144),
    *(unsigned int *)(this + 216));
  *(_BYTE *)(this + 180) = 0;
LABEL_94:
  *(_BYTE *)(this + 248) = 0;
    mBluetoothFamilyLogPacket(251, "%s->%s-\n", v22, v23);
    *(_BYTE *)(this + 368) = 0;
  return v43;
}


#if __MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_VERSION_11_0
void IOBluetoothHostControllerUARTTransport::DumpTransportProviderState()
{
  __int64 v1; // rbx
  __int64 v2; // r15
  __int64 v3; // r12
  int v4; // ST00_4
  __int64 v5; // r8
  __int64 v6; // r9
  __int64 v7; // rax
  __int64 v8; // rax
  __int64 result; // rax
  __int64 v10; // rbx
  __int64 v11; // rax
  int v12; // et1
  int v13; // et1
  int v14; // et1
  __int64 v15; // r8
  __int64 v16; // r9
  int v17; // ST00_4

    
  (*(void (__fastcall **)(__int64))(*(_QWORD *)v1 + 1664LL))(v1);
  
  v3 = *(_QWORD *)(v1 + 152);
  (*(void (__fastcall **)(__int64))(*(_QWORD *)v1 + 1664LL))(v1);
  v4 = *(_DWORD *)(v1 + 180);
    os_log(mInternalOSLogObject, "[IOBluetoothHostControllerUARTTransport][DumpTransportProviderState] -- sync->provider (%p) == serial->getProvider() (%p) - sanity check, state=0x%x")
  
  v5 = *(_QWORD *)(v1 + 192);
  v6 = *(_QWORD *)(v1 + 200);
    os_log(mInternalOSLogObject, "[IOBluetoothHostControllerUARTTransport][DumpTransportProviderState] -- TX head=%p tail=%p ");
    
  v7 = (*(__int64 (__fastcall **)(__int64))(*(_QWORD *)v1 + 1664LL))(v1);
  v8 = (*(__int64 (__fastcall **)(__int64))(*(_QWORD *)v7 + 1664LL))(v7);
    
  result = (*(__int64 (__fastcall **)(__int64, _QWORD, _QWORD))(*(_QWORD *)v8 + 1760LL))(v8, 0LL, 0LL);
  if ( result )
  {
    v10 = result;
    v11 = (*(__int64 (__fastcall **)(__int64))(*(_QWORD *)result + 280LL))(result);
    v12 = *(_DWORD *)(v11 + 128);
    v13 = *(_DWORD *)(v11 + 16);
    v14 = *(_DWORD *)(v11 + 24);
    v15 = *(unsigned int *)(v11 + 128);
    v16 = *(unsigned int *)(v11 + 16);
    v17 = *(_DWORD *)(v11 + 24);
      os_log(mInternalOSLogObject, "[IOBluetoothHostControllerUARTTransport][DumpTransportProviderState] -- PCInub Serial: TX FIFO level=%d, MCR=0x%x (bit 1 - RTS), MSR=0x%x (bit 4 - CTS)");
       (*(__int64 (__fastcall **)(__int64))(*(_QWORD *)v10 + 40LL))(v10);
  }
}
#endif


IOReturn IOBluetoothHostController::CleanupForPowerChangeFromOnToSleep(bool a2, UInt32 * a3)
{
  __int64 v5; // r14
  unsigned int v6; // eax
  unsigned int v7; // er13
  char *v8; // rax
  char *v9; // r14
  unsigned int v10; // eax
  __int64 v11; // r9
  __int64 v12; // rbx
  size_t v13; // rax
  char *v14; // rax
  char *v15; // r13
  unsigned int v16; // eax
  __int64 v17; // r9
  __int64 v18; // r15
  size_t v19; // rax
  __int64 v20; // rax
  __int64 v21; // rcx
  int v22; // ecx
  OSMetaClassBase *v23; // rax
  __int64 v24; // rsi
  const OSMetaClass *v25; // rdx
  __int64 v26; // rax
  __int64 v27; // rax
  __int64 v28; // r8
  __int64 v29; // r9
  unsigned __int64 v30; // rax
  unsigned int v31; // eax
  unsigned int v32; // er13
  char *v33; // rax
  char *v34; // r15
  unsigned int v35; // eax
  __int64 v36; // r9
  __int64 v37; // rbx
  size_t v38; // rax
  char *v39; // rax
  char *v40; // rbx
  unsigned int *v41; // r13
  unsigned int v42; // eax
  __int64 v43; // r9
  __int64 v44; // r15
  size_t v45; // rax
  unsigned int v46; // eax
  unsigned int v47; // er13
  char *v48; // rax
  char *v49; // r15
  unsigned int v50; // eax
  __int64 v51; // r9
  __int64 v52; // rbx
  size_t v53; // rax
  char *v54; // rax
  char *v55; // rbx
  unsigned int v56; // eax
  __int64 v57; // r9
  __int64 v58; // r15
  size_t v59; // rax
  unsigned int v60; // eax
  unsigned int v61; // er13
  char *v62; // rax
  char *v63; // r15
  unsigned int v64; // eax
  __int64 v65; // r9
  __int64 v66; // rbx
  size_t v67; // rax
  __int64 v68; // r15
  char *v69; // rax
  char *v70; // r14
  unsigned int v71; // eax
  __int64 v72; // r9
  __int64 v73; // rbx
  size_t v74; // rax
  __int64 v75; // rbx
  char *v76; // rax
  char *v77; // rbx
  unsigned int *v78; // r13
  __int64 v79; // r15
  size_t v80; // rax
  char *v81; // rax
  char *v82; // rbx
  unsigned int *v83; // r13
  unsigned int v84; // er15
  unsigned int v85; // eax
  __int64 v86; // rcx
  __int64 v87; // r9
  char *v88; // rax
  char *v89; // rbx
  unsigned int *v90; // r13
  __int64 v91; // r15
  size_t v92; // rax
  __int64 result; // rax
  __int64 v94; // [rsp+0h] [rbp-170h]
  __int64 v95; // [rsp+0h] [rbp-170h]
  __int64 v96; // [rsp+0h] [rbp-170h]
  __int64 v97; // [rsp+8h] [rbp-168h]
  unsigned int *v98; // [rsp+10h] [rbp-160h]
  __int64 v100; // [rsp+20h] [rbp-150h]
  __int64 v101; // [rsp+28h] [rbp-148h]
  __int64 v102; // [rsp+30h] [rbp-140h]
  __int64 v103; // [rsp+38h] [rbp-138h]
  __int64 v104; // [rsp+40h] [rbp-130h]
  __int64 v105; // [rsp+48h] [rbp-128h]
  __int64 v106; // [rsp+50h] [rbp-120h]
  __int64 v107; // [rsp+58h] [rbp-118h]
  __int64 v108; // [rsp+60h] [rbp-110h]
  __int64 v109; // [rsp+68h] [rbp-108h]
  __int64 v110; // [rsp+70h] [rbp-100h]
  __int64 v111; // [rsp+78h] [rbp-F8h]
  int v112; // [rsp+80h] [rbp-F0h]
  __int64 v113; // [rsp+90h] [rbp-E0h]
  __int64 v114; // [rsp+98h] [rbp-D8h]
  __int64 v115; // [rsp+A0h] [rbp-D0h]
  __int64 v116; // [rsp+A8h] [rbp-C8h]
  __int64 v117; // [rsp+B0h] [rbp-C0h]
  __int64 v118; // [rsp+B8h] [rbp-B8h]
  __int16 v119; // [rsp+C0h] [rbp-B0h]
  __int64 v120; // [rsp+D0h] [rbp-A0h]
  __int64 v121; // [rsp+D8h] [rbp-98h]
  __int64 v122; // [rsp+E0h] [rbp-90h]
  __int64 v123; // [rsp+E8h] [rbp-88h]
  __int64 v124; // [rsp+F0h] [rbp-80h]
  __int64 v125; // [rsp+F8h] [rbp-78h]
  __int64 v126; // [rsp+100h] [rbp-70h]
  __int64 v127; // [rsp+108h] [rbp-68h]
  __int64 v128; // [rsp+110h] [rbp-60h]
  __int64 v129; // [rsp+118h] [rbp-58h]
  __int64 v130; // [rsp+120h] [rbp-50h]
  __int64 v131; // [rsp+128h] [rbp-48h]
  int v132; // [rsp+130h] [rbp-40h]
  __int64 v133; // [rsp+140h] [rbp-30h]

  LODWORD(v5) = 0;
  snprintf((char *)&v100, 0x64uLL, "IOBluetoothHostController::CleanupForPowerChangeFromOnToSleep()");
    
    
    BluetoothHCIRequestID id;
    OSData * data;
    BluetoothHCISupportedFeatures * feature;
    BluetoothSetEventMask mask;
    
    *(bool *)(mBluetoothFamily + 416) = 0;
    KillAllPendingRequests(false, false);
    
    if ( *(_BYTE *)(this + 753) & 1 )
    {
        v6 = HCIRequestCreate(&id, false, 5000);
        if ( v6 )
        {
            BluetoothFamilyLogPacketWithOSLog(mInternalOSLogObject, mBluetoothFamily, 250, "**** [IOBluetoothHostController][CleanupForPowerChangeFromOnToSleep] -- Error! Beware HCIRequestCreate() failed so Write Scan Enable command is not sent, and this could cause strange behavior -- this = 0x%04x\n", ConvertAddressToUInt32(this));
            os_log(OS_LOG_DEFAULT, "REQUIRE_NO_ERR failure: 0x%x - file: %s:%d", v6, "/System/Volumes/Data/SWE/macOS/BuildRoots/2288acc43c/Library/Caches/com.apple.xbs/Sources/IOBluetoothFamily_kexts/IOBluetoothFamily-8004.1.18.3/Core/Family/HCI/HostControllers/IOBluetoothHostController.cpp", __LINE__);
            return 0; //why?
        }
        
        v5 = BluetoothHCIWriteScanEnable(id, *(UInt8 *)(this + 753) & 0xFE, false);
        if ( v5 )
            BluetoothFamilyLogPacketWithOSLog(mInternalOSLogObject, mBluetoothFamily, 250, "**** [IOBluetoothHostController][CleanupForPowerChangeFromOnToSleep] -- Error! Beware Write Scan Enable command failed, and this could cause strange behavior -- %s -- this = 0x%04x\n", "Sleeping", ConvertAddressToUInt32(this));
        
        HCIRequestDelete(NULL, id);
    }
    
    *(_QWORD *)(this + 768) = *(_QWORD *)(this + 760);
    *(_QWORD *)(this + 784) = *(_QWORD *)(this + 776);
    if ( *(_QWORD *)(this + 760) )
    {
        *(_QWORD *)(this + 760) &= 0xFFFFFFFFFFF7FFFFLL;
        if ( mVendorID == 2652 )
        {
            v22 = mProductID;
            if ( mProductID == 8520 || mProductID == 8448 )
                *(_QWORD *)(this + 760) &= 0xFFFFFFFFFBF7FFFFLL;
        }
        
        data = OSDynamicCast(OSData, getProperty("HCISupportedFeatures"));
        if ( data )
        {
            feature = (BluetoothHCISupportedFeatures *) data->getBytesNoCopy();
            if ( feature && feature.data[3] & 0x40 )
            {
                if ( a2 )
                {
                    os_log(mInternalOSLogObject, "Bluetooth -- LE is supported - Disable LE meta event");
                    *(_BYTE *)(this + 767) &= 0xDF;
                }
                *(_BYTE *)(this + 776) &= 0xFB;
                if ( IsTBFCSupported() )
                    *(_WORD *)(this + 762) &= 0xFBFD;
            }
        }
        
        *(_QWORD *)(this + 760) &= 0xFFFF7FFF7FFFFF7FLL;
        v31 = HCIRequestCreate(&id, 0, 5000);
        if ( v31 )
        {
            BluetoothFamilyLogPacketWithOSLog(mInternalOSLogObject, mBluetoothFamily, 250, "**** [IOBluetoothHostController][CleanupForPowerChangeFromOnToSleep] -- Error! Beware HCIRequestCreate() failed so Set Event Mask command is not sent, and this could cause strange behavior -- %s -- this = 0x%04x\n", "Sleeping", ConvertAddressToUInt32(this));
            os_log(OS_LOG_DEFAULT, "REQUIRE_NO_ERR failure: 0x%x - file: %s:%d", "/System/Volumes/Data/SWE/macOS/BuildRoots/2288acc43c/Library/Caches/com.apple.xbs/Sources/IOBluetoothFamily_kexts/IOBluetoothFamily-8004.1.18.3/Core/Family/HCI/HostControllers/IOBluetoothHostController.cpp", __LINE__);
            goto LABEL_86;
        }
        
        v5 = BluetoothHCISetEventMask(id, &mask);
        if ( v5 )
        {
            mBluetoothFamily->ConvertErrorCodeToString(v5, &v120, &v113);
            BluetoothFamilyLogPacketWithOSLog(mInternalOSLogObject, mBluetoothFamily, 250, "**** [IOBluetoothHostController][CleanupForPowerChangeFromOnToSleep] -- Error! Beware Set Event Mask command failed, and this could cause strange behavior -- %s -- this = 0x%04x\n", "Sleeping", ConvertAddressToUInt32(this));
        }
    (*(void (__fastcall **)(__int64, _QWORD, _QWORD))(*(_QWORD *)this + 3136LL))(this, 0LL, id);
    LODWORD(v95) = 0;
    v46 = (*(__int64 (__fastcall **)(__int64, unsigned int *, _QWORD, signed __int64, _QWORD, _QWORD, __int64))(*(_QWORD *)this + 3128LL))(
            this,
            &id,
            0LL,
            5000LL,
            0LL,
            0LL,
            v95);
    if ( v46 )
    {
      v47 = v46;
      v48 = (char *)IOMalloc(511LL);
      if ( v48 )
      {
        v49 = v48;
        bzero(v48, 0x1FFuLL);
        v50 = (*(__int64 (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 4168LL))(this, this);
        snprintf(
          v49,
          0x1FFuLL,
          "**** [IOBluetoothHostController][CleanupForPowerChangeFromOnToSleep] -- Error! Beware HCIRequestCreate() faile"
          "d so LE Set Event Mask command is not sent, and this could cause strange behavior -- %s -- this = 0x%04x\n",
          "Sleeping",
          v50);
        _os_log_internal(
          &dword_0,
          *(_QWORD *)(this + 1288),
          0LL,
          IOBluetoothHostController::CleanupForPowerChangeFromOnToSleep(bool,unsigned int *)::_os_log_fmt,
          v49,
          v51);
        v52 = *(_QWORD *)(this + 824);
        if ( v52 )
        {
          v53 = strlen(v49);
          (*(void (__fastcall **)(__int64, signed __int64, char *, size_t))(*(_QWORD *)v52 + 2248LL))(
            v52,
            250LL,
            v49,
            v53);
        }
        IOFree(v49, 511LL);
      }
      _os_log_internal(
        &dword_0,
        &_os_log_default,
        0LL,
        IOBluetoothHostController::CleanupForPowerChangeFromOnToSleep(bool,unsigned int *)::_os_log_fmt,
        v47,
        "/System/Volumes/Data/SWE/macOS/BuildRoots/2288acc43c/Library/Caches/com.apple.xbs/Sources/IOBluetoothFamily_kext"
        "s/IOBluetoothFamily-8004.1.18.3/Core/Family/HCI/HostControllers/IOBluetoothHostController.cpp");
      goto LABEL_86;
    }
    v98 = a3;
    if ( *(_QWORD *)(this + 784) )
    {
      v5 = (*(unsigned int (__fastcall **)(__int64, _QWORD, __int64))(*(_QWORD *)this + 4752LL))(this, id, this + 776);
      (*(void (__fastcall **)(_QWORD, __int64, __int64 *, __int64 *))(**(_QWORD **)(this + 824) + 2848LL))(
        *(_QWORD *)(this + 824),
        v5,
        &v120,
        &v113);
      if ( (_DWORD)v5 )
      {
        v54 = (char *)IOMalloc(511LL);
        if ( v54 )
        {
          v55 = v54;
          bzero(v54, 0x1FFuLL);
          v56 = (*(__int64 (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 4168LL))(this, this);
          snprintf(
            v55,
            0x1FFuLL,
            "**** [IOBluetoothHostController][CleanupForPowerChangeFromOnToSleep] -- Error! Beware LE Set Event Mask comm"
            "and failed, and this could cause strange behavior -- %s -- this = 0x%04x\n",
            "Sleeping",
            v56);
          _os_log_internal(
            &dword_0,
            *(_QWORD *)(this + 1288),
            0LL,
            IOBluetoothHostController::CleanupForPowerChangeFromOnToSleep(bool,unsigned int *)::_os_log_fmt,
            v55,
            v57);
          v58 = *(_QWORD *)(this + 824);
          if ( v58 )
          {
            v59 = strlen(v55);
            (*(void (__fastcall **)(__int64, signed __int64, char *, size_t))(*(_QWORD *)v58 + 2248LL))(
              v58,
              250LL,
              v55,
              v59);
          }
          IOFree(v55, 511LL);
        }
      }
      else
      {
        LODWORD(v5) = 0;
      }
    }
    (*(void (__fastcall **)(__int64, _QWORD, _QWORD))(*(_QWORD *)this + 3136LL))(this, 0LL, id);
    LODWORD(v96) = 0;
    v60 = (*(__int64 (__fastcall **)(__int64, unsigned int *, _QWORD, signed __int64, _QWORD, _QWORD, __int64))(*(_QWORD *)this + 3128LL))(
            this,
            &id,
            0LL,
            5000LL,
            0LL,
            0LL,
            v96);
    if ( v60 )
    {
      v61 = v60;
      v62 = (char *)IOMalloc(511LL);
      if ( v62 )
      {
        v63 = v62;
        bzero(v62, 0x1FFuLL);
        v64 = (*(__int64 (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 4168LL))(this, this);
        snprintf(
          v63,
          0x1FFuLL,
          "**** [IOBluetoothHostController][CleanupForPowerChangeFromOnToSleep] -- Error! Beware HCIRequestCreate() faile"
          "d so LE Set Advertise Enable command is not sent, and this could cause strange behavior -- %s -- this = 0x%04x\n",
          "Sleeping",
          v64);
        _os_log_internal(
          &dword_0,
          *(_QWORD *)(this + 1288),
          0LL,
          IOBluetoothHostController::CleanupForPowerChangeFromOnToSleep(bool,unsigned int *)::_os_log_fmt,
          v63,
          v65);
        v66 = *(_QWORD *)(this + 824);
        if ( v66 )
        {
          v67 = strlen(v63);
          (*(void (__fastcall **)(__int64, signed __int64, char *, size_t))(*(_QWORD *)v66 + 2248LL))(
            v66,
            250LL,
            v63,
            v67);
        }
        IOFree(v63, 511LL);
      }
      _os_log_internal(
        &dword_0,
        &_os_log_default,
        0LL,
        _ZZN25IOBluetoothHostController34CleanupForPowerChangeFromOnToSleepEbPjE11_os_log_fmt__10_,
        v61,
        "/System/Volumes/Data/SWE/macOS/BuildRoots/2288acc43c/Library/Caches/com.apple.xbs/Sources/IOBluetoothFamily_kext"
        "s/IOBluetoothFamily-8004.1.18.3/Core/Family/HCI/HostControllers/IOBluetoothHostController.cpp");
      goto LABEL_86;
    }
    if ( *(_BYTE *)(this + 792) )
    {
      LODWORD(v5) = 0;
      v68 = (*(unsigned int (__fastcall **)(__int64, _QWORD, _QWORD))(*(_QWORD *)this + 4776LL))(this, id, 0LL);
      (*(void (__fastcall **)(_QWORD, __int64, __int64 *, __int64 *))(**(_QWORD **)(this + 824) + 2848LL))(
        *(_QWORD *)(this + 824),
        v68,
        &v120,
        &v113);
      if ( (_DWORD)v68 )
      {
        v69 = (char *)IOMalloc(511LL);
        if ( v69 )
        {
          v70 = v69;
          bzero(v69, 0x1FFuLL);
          v71 = (*(__int64 (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 4168LL))(this, this);
          snprintf(
            v70,
            0x1FFuLL,
            "**** [IOBluetoothHostController][CleanupForPowerChangeFromOnToSleep] -- Error! Beware LE Set Advertise Enabl"
            "e command failed, and this could cause strange behavior -- %s -- this = 0x%04x\n",
            "Sleeping",
            v71);
          _os_log_internal(
            &dword_0,
            *(_QWORD *)(this + 1288),
            0LL,
            _ZZN25IOBluetoothHostController34CleanupForPowerChangeFromOnToSleepEbPjE11_os_log_fmt__11_,
            v70,
            v72);
          v73 = *(_QWORD *)(this + 824);
          if ( v73 )
          {
            v74 = strlen(v70);
            (*(void (__fastcall **)(__int64, signed __int64, char *, size_t))(*(_QWORD *)v73 + 2248LL))(
              v73,
              250LL,
              v70,
              v74);
          }
          IOFree(v70, 511LL);
        }
        LODWORD(v5) = v68;
      }
    }
    (*(void (__fastcall **)(__int64, _QWORD, _QWORD))(*(_QWORD *)this + 3136LL))(this, 0LL, id);
    a3 = v98;
  }
  v75 = (*(unsigned int (__fastcall **)(__int64, signed __int64, signed __int64, signed __int64))(*(_QWORD *)this
                                                                                                + 3984LL))(
          this,
          1LL,
          1LL,
          1LL);
  (*(void (__fastcall **)(_QWORD, __int64, __int64 *, __int64 *))(**(_QWORD **)(this + 824) + 2848LL))(
    *(_QWORD *)(this + 824),
    v75,
    &v120,
    &v113);
  if ( (_DWORD)v75 != -536870201 && (_DWORD)v75 )
    LODWORD(v5) = v75;
  (*(void (__fastcall **)(__int64, signed __int64))(*(_QWORD *)this + 3888LL))(this, 1LL);
  if ( *(_DWORD *)(this + 306) || *(_WORD *)(this + 310) || *(_DWORD *)(this + 400) || *(_WORD *)(this + 404) )
  {
    v76 = (char *)IOMalloc(511LL);
    if ( v76 )
    {
      v77 = v76;
      v78 = a3;
      bzero(v76, 0x1FFuLL);
      snprintf(v77, 0x1FFuLL, "Wait for ACL Packet");
      v79 = *(_QWORD *)(this + 824);
      if ( v79 )
      {
        v80 = strlen(v77);
        (*(void (__fastcall **)(__int64, signed __int64, char *, size_t))(*(_QWORD *)v79 + 2248LL))(
          v79,
          248LL,
          v77,
          v80);
      }
      IOFree(v77, 511LL);
      a3 = v78;
    }
    if ( a3 )
    {
      *a3 = 20000000;
      *(_BYTE *)(this + 913) = 1;
    }
  }
  if ( !(*(unsigned __int8 (__fastcall **)(__int64))(*(_QWORD *)this + 3184LL))(this) )
  {
    v81 = (char *)IOMalloc(511LL);
    if ( v81 )
    {
      v82 = v81;
      bzero(v81, 0x1FFuLL);
      v83 = a3;
      v84 = *(unsigned __int8 *)(this + 912);
      v85 = (*(__int64 (__fastcall **)(__int64, __int64))(*(_QWORD *)this + 4168LL))(this, this);
      v86 = v84;
      a3 = v83;
      snprintf(
        v82,
        0x1FFuLL,
        "**** [IOBluetoothHostController][CleanupForPowerChangeFromOnToSleep] -- Need to wait for in flight HCI Requests "
        "to be completed -- mNumberOfCommandsAllowedByHardware = %d -- this = 0x%04x ****\n",
        v86,
        v85);
      _os_log_internal(
        &dword_0,
        *(_QWORD *)(this + 1288),
        0LL,
        _ZZN25IOBluetoothHostController34CleanupForPowerChangeFromOnToSleepEbPjE11_os_log_fmt__12_,
        v82,
        v87);
      IOFree(v82, 511LL);
    }
    v88 = (char *)IOMalloc(511LL);
    if ( v88 )
    {
      v89 = v88;
      v90 = a3;
      bzero(v88, 0x1FFuLL);
      snprintf(v89, 0x1FFuLL, "Wait for HCI cmd");
      v91 = *(_QWORD *)(this + 824);
      if ( v91 )
      {
        v92 = strlen(v89);
        (*(void (__fastcall **)(__int64, signed __int64, char *, size_t))(*(_QWORD *)v91 + 2248LL))(
          v91,
          248LL,
          v89,
          v92);
      }
      IOFree(v89, 511LL);
      a3 = v90;
    }
    if ( a3 )
    {
      *a3 = 20000000;
      *(_BYTE *)(this + 972) = 1;
    }
  }
LABEL_86:
    return v5;
}
