// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.10;

import "../YieldManagers/templates/YieldManagerAaveV3.sol";

/** @title YieldManagerBenqiTest
  @notice Used to manually test the benqi yield manager
  */
contract DefaultYieldManagerAaveV3 is YieldManagerAaveV3 {
  /// @dev The maximum amount of payment token this contract will allow
  function maxPaymentTokenNotInYieldManagerThreshold() internal pure override returns (uint256) {
    return 1e21; // 10^21 = 1000 units
  }

  /// @dev The desired minimum amount of payment token this contract will target
  function minPaymentTokenNotInYieldManagerTarget() internal pure override returns (uint256) {
    return 2e20; // 2*10^20 = 200 units
  }
}
