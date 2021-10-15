//
//  test.hpp
//  IOBluetoothFamily
//
//  Created by Charlie Jiang on 9/5/21.
//

#ifndef test_hpp
#define test_hpp

#include <IOKit/bluetooth/IOBluetoothHCIController.h>
#include <IOKit/bluetooth/IOBluetoothHostController.h>
#include <IOKit/bluetooth/IOBluetoothHCIRequest.h>

class tester : public IOBluetoothHCIController
{
    OSDeclareDefaultStructors(tester)
    
};

#endif /* test_hpp */
