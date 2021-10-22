// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.3;

import "../FloatCapital_v0.sol";

// This is a little trick to make sure the implementation contract is initialized automatically

/** @title Float Capital Contract */
contract FloatCapital_v0_Implementation is FloatCapital_v0 {
  constructor() {
    address deadAddress = 0xf10A7_F10A7_f10A7_F10a7_F10A7_f10a7_F10A7_f10a7;
    initialize(deadAddress);
  }
}
