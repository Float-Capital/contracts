// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.3;

import "./OracleManagerChainlink.sol";

contract OracleManagerChainlinkTestnet is OracleManagerChainlink {
  uint256 lastUpdate;
  uint256 maxUpdateIntervalSeconds;
  int256 forcedPriceAdjustment;

  constructor(
    address _admin,
    address _chainlinkOracle,
    uint256 _maxUpdateIntervalSeconds
  ) OracleManagerChainlink(_admin, _chainlinkOracle) {
    maxUpdateIntervalSeconds = _maxUpdateIntervalSeconds;
  }

  function setMaxUpdateInterval(uint256 newMaxUpdateIntervalSeconds) external adminOnly {
    maxUpdateIntervalSeconds = newMaxUpdateIntervalSeconds;
  }

  function updatePrice() external override returns (int256) {
    int256 latestPrice = super._getLatestPrice();

    int256 priceAdjustment = forcedPriceAdjustment;
    if (lastUpdate + maxUpdateIntervalSeconds < block.timestamp) {
      priceAdjustment = (priceAdjustment + 1) % 2;
      forcedPriceAdjustment = priceAdjustment;
      lastUpdate = block.timestamp;
    }

    return latestPrice + priceAdjustment;
  }
}
