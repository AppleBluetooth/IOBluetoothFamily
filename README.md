# IOBluetoothFamily
Decompilation of Apple's IOBluetoothFamily (specifically macOS 10.15.7) for simplifying Bluetooth kernel development.

# Status
Tackling 1-3 class everyday, based on the amount of functions/variables they have.

# TO-DO
I will try to decompile IOBluetoothFamily first, though it is very large. From decompilation I observed that there are 27 files in total, specifically:
- [ ] IOBluetoothHCIController.h
- [x] IOBluetoothHCIUserClient.h
- [ ] IOBluetoothHCIRequest.h
- [ ] IOBluetoothDevice.h
- [ ] IOBluetoothDeviceUserClient.h
- [ ] IOBluetoothACPIMethods.h
- [ ] IOBluetoothL2CAPChannel.h
- [ ] IOBluetoothL2CAPChannelUserClient.h
- [ ] IOBluetoothL2CAPSignalChannel.h
- [ ] IOBluetoothRFCOMMChannel.h
- [ ] IOBluetoothRFCOMMChannelUserClient.h
- [ ] IOBluetoothRFCOMMConnection.h
- [ ] IOBluetoothRFCOMMConnectionUserClient.h
- [ ] IOBluetoothDataQueue.h
- [ ] IOWorkQueue.h
- [x] IOBluetoothMemoryBlock.h
- [x] IOBluetoothMemoryBlockQueue.h
- [ ] IOBluetoothHostController.h
- [ ] IOBluetoothHostControllerUserClient.h
- [ ] BroadcomBluetoothHostController.h
- [ ] AppleBroadcomBluetoothHostController.h
- [ ] CSRBluetoothHostController.h
- [ ] AppleCSRBluetoothHostController.h
- [x] IOBluetoothWorkLoop.h
- [x] IOBluetoothObject.h
- [x] IOBluetoothTimerEventSource.h
- [x] IOBluetoothInactivityTimerEventSource.h
