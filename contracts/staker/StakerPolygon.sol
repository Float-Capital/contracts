// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.10;

import "./template/Staker.sol";

contract StakerPolygon is Staker {
  /// @dev This contract uses legacy data.
  function HAS_LEGACY_DATA() internal pure override returns (bool) {
    return true;
  }

  /// @dev The this contract has markets with active flt issuance multipliers and intends on using them in the future.
  function HAS_FLT_ISSUANCE_MULTIPLIER() internal pure override returns (bool) {
    return false;
  }

  /// @dev The NFT discounts are enabled.
  function NFT_DISCOUNTS_ENABLED() internal pure override returns (bool) {
    return true;
  }

  // auto initialize implementation
  constructor() initializer {}
}
