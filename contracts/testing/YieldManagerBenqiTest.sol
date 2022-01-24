// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.10;

import "../YieldManagers/templates/YieldManagerCompound.sol";

/** @title YieldManagerBenqiTest
  @notice Used to manually test the benqi yield manager
  */
contract YieldManagerBenqiTest is YieldManagerCompound {
  /// @dev The maximum amount of payment token this contract will allow
  function maxPaymentTokenNotInYieldManagerThreshold() internal pure override returns (uint256) {
    return 10000000000000000000; // 10 units
  }

  /// @dev The desired minimum amount of payment token this contract will target
  function minPaymentTokenNotInYieldManagerTarget() internal pure override returns (uint256) {
    return 2000000000000000000; // 2 units
  }
}
