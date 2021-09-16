// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.3;

import "@chainlink/contracts/src/v0.8/interfaces/AggregatorV3Interface.sol";
import "../interfaces/IOracleManager.sol";

contract OracleManagerFlipp3ning is IOracleManager {
  address public ethMarketCapFeed;
  address public btcMarketCapFeed;

  struct PriceData {
    uint80 lastRoundEth;
    uint80 lastRoundBtc;
    uint128 price;
  }

  PriceData public priceData;

  constructor(address _ethMarketCapFeed, address _btcMarketCapFeed) {
    ethMarketCapFeed = _ethMarketCapFeed;
    btcMarketCapFeed = _btcMarketCapFeed;

    require(
      AggregatorV3Interface(ethMarketCapFeed).decimals() ==
        AggregatorV3Interface(btcMarketCapFeed).decimals(),
      "Decimals for feeds are different!"
    );

    (uint80 _ethRound, uint80 _btcRound, int256 _ethMarketCap, int256 _btcMarketCap) = _feedData();

    priceData.price = _dominance(_ethMarketCap, _btcMarketCap);
    priceData.lastRoundEth = _ethRound;
    priceData.lastRoundBtc = _btcRound;
  }

  function _feedData()
    internal
    view
    returns (
      uint80 ethRound,
      uint80 btcRound,
      int256 ethPrice,
      int256 btcPrice
    )
  {
    (ethRound, ethPrice, , , ) = AggregatorV3Interface(ethMarketCapFeed).latestRoundData();
    (btcRound, btcPrice, , , ) = AggregatorV3Interface(btcMarketCapFeed).latestRoundData();
  }

  function _dominance(int256 _ethMarketCap, int256 _btcMarketCap) internal view returns (uint128) {
    // reverts if btc market cap == 0
    return uint128(uint256((_ethMarketCap * 1e20) / _btcMarketCap));
  }

  function getLatestPrice() external view override returns (int256) {
    return int256(uint256(priceData.price));
  }

  function updatePrice() external override returns (int256) {
    (uint80 _ethRound, uint80 _btcRound, int256 _ethMarketCap, int256 _btcMarketCap) = _feedData();

    if (_ethRound == priceData.lastRoundEth || _btcRound == priceData.lastRoundBtc) {
      return int256(uint256(priceData.price));
    }

    uint128 price = _dominance(_ethMarketCap, _btcMarketCap);
    priceData.price = price;
    priceData.lastRoundEth = _ethRound;
    priceData.lastRoundBtc = _btcRound;

    return int256(uint256(price));
  }
}
