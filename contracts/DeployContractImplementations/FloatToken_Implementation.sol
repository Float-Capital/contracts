// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.3;

import "../FloatToken.sol";

// This is a little trick to make sure the implementation contract is initialized automatically

contract FloatToken_Implementation is FloatToken {
  constructor() {
    address deadAddress = 0xf10A7_F10A7_f10A7_F10a7_F10A7_f10a7_F10A7_f10a7;
    string memory deadString = "DEAD";
    initialize(deadString, deadString, deadAddress);
  }
}
