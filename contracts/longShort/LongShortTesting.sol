// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.10;

import "./template/LongShort.sol";

contract LongShortTesting is LongShort {
  /// @dev This contract uses legacy data.
  function HAS_LEGACY_DATA() internal pure override returns (bool) {
    return true;
  }

  /// @dev This is the amount of time users need to wait between trades.
  function CONTRACT_SLOW_TRADE_TIME() internal pure override returns (uint256) {
    return 15 minutes;
  }

  // auto initialize implementation
  constructor() initializer {}
}
