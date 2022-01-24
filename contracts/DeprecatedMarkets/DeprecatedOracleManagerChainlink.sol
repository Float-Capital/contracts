// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.10;

import "../oracles/OracleManagerChainlink.sol";

contract DeprecatedOracleManagerChainlink is OracleManagerChainlink {
  int256 forcedPriceAdjustment;
  int256 immutable frozenFinalOraclePrice;
  address longShort;

  constructor(
    address _admin,
    address _chainlinkOracle,
    address _longShort
  ) OracleManagerChainlink(_admin, _chainlinkOracle) {
    (, int256 price, , , ) = chainlinkOracle.latestRoundData();
    frozenFinalOraclePrice = price;

    longShort = _longShort;
  }

  function _getLatestPrice() internal view override returns (int256) {
    int256 priceAdjustment = forcedPriceAdjustment;
    priceAdjustment = (priceAdjustment + 1) % 2;

    return frozenFinalOraclePrice + priceAdjustment;
  }

  function updatePrice() external override returns (int256) {
    require(msg.sender == longShort, "Only long short");

    int256 priceAdjustment = forcedPriceAdjustment;
    priceAdjustment = (priceAdjustment + 1) % 2;
    forcedPriceAdjustment = priceAdjustment;

    return frozenFinalOraclePrice + priceAdjustment;
  }
}
