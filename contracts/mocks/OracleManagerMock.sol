// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.3;

import "../interfaces/IOracleManager.sol";

/*
 * Mock implementation of an OracleManager with fixed, changeable prices.
 */
contract OracleManagerMock is IOracleManager {
  // Admin contract.
  address public admin;

  // Global state.
  int256 currentPrice; // e18

  uint256 lastUpdate;
  uint256 maxUpdateIntervalSeconds;
  int256 forcedPriceAdjustment;

  ////////////////////////////////////
  /////////// MODIFIERS //////////////
  ////////////////////////////////////

  modifier adminOnly() {
    require(msg.sender == admin, "Not admin");
    _;
  }

  ////////////////////////////////////
  ///// CONTRACT SET-UP //////////////
  ////////////////////////////////////

  constructor(address _admin, uint256 _maxUpdateIntervalSeconds) {
    maxUpdateIntervalSeconds = _maxUpdateIntervalSeconds;
    admin = _admin;

    // Default to a price of 1.
    currentPrice = 1e18;
  }

  ////////////////////////////////////
  ///// IMPLEMENTATION ///////////////
  ////////////////////////////////////

  function setPrice(int256 newPrice) public adminOnly {
    currentPrice = newPrice;
  }

  function updatePrice() external override returns (int256) {
    int256 priceAdjustment = forcedPriceAdjustment;
    if (
      maxUpdateIntervalSeconds != 0 && /* a value of zero means it doesn't run */
      lastUpdate + maxUpdateIntervalSeconds < block.timestamp
    ) {
      priceAdjustment = (priceAdjustment + 1) % 2;
      forcedPriceAdjustment = priceAdjustment;
      lastUpdate = block.timestamp;
    }

    return currentPrice + priceAdjustment;
  }

  function getLatestPrice() external view override returns (int256) {
    return currentPrice;
  }
}
