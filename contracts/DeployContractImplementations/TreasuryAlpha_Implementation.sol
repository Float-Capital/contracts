// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.10;

import "../TreasuryAlpha.sol";

// This is a little trick to make sure the implementation contract is initialized automatically

contract TreasuryAlpha_Implementation is TreasuryAlpha {
  constructor() {
    address deadAddress = 0xf10A7_F10A7_f10A7_F10a7_F10A7_f10a7_F10A7_f10a7;
    initialize(deadAddress, deadAddress, deadAddress, deadAddress);
  }
}
