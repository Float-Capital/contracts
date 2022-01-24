/*
background: This is an experimental, quickly hashed together contract.
It will never hold value of any kind.

The goal is that a future version of this contract becomes a proper keeper contract.

For now no effort has been put into making it conform to the proper keeper interface etc.
*/

// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.10;

import "./abstract/AccessControlledAndUpgradeable.sol";
import "./interfaces/ILongShort.sol";
import "./interfaces/IStaker.sol";
import "./interfaces/IOracleManager.sol";

contract KeeperExperiment is AccessControlledAndUpgradeable {
  address public longShortAddress;
  address public stakerAddress;

  mapping(uint32 => uint256) public updateTimeThresholdInSeconds;
  mapping(uint32 => uint256) public percentChangeThreshold;

  function initialize(
    address _admin,
    address _longShort,
    address _staker
  ) external virtual initializer {
    longShortAddress = _longShort;
    stakerAddress = _staker;

    _AccessControlledAndUpgradeable_init(_admin);
  }

  function updateHeartbeatThresholdForMarket(uint32 marketIndex, uint256 heartbeatInSeconds)
    external
    onlyRole(ADMIN_ROLE)
  {
    // No validation - we are admin ;)
    updateTimeThresholdInSeconds[marketIndex] = heartbeatInSeconds;
  }

  function updatePercentChangeThresholdForMarket(
    uint32 marketIndex,
    uint256 _percentChangeThreshold
  ) external onlyRole(ADMIN_ROLE) {
    // No validation - we are admin ;)
    percentChangeThreshold[marketIndex] = _percentChangeThreshold;
  }

  function wasLastUpdateOlderThanThreshold(uint32 marketIndex) public view returns (bool) {
    uint256 latestRewardIndex = IStaker(stakerAddress).latestRewardIndex(marketIndex);
    uint256 lastUpdateTimestamp = IStaker(stakerAddress).safe_getUpdateTimestamp(
      marketIndex,
      latestRewardIndex
    );

    return lastUpdateTimestamp + updateTimeThresholdInSeconds[marketIndex] < block.timestamp;
  }

  function isPriceChangeOverThreshold(uint32 marketIndex) public view returns (bool) {
    int256 currentAssetPrice = ILongShort(longShortAddress).assetPrice(marketIndex);
    IOracleManager oracle = IOracleManager(
      ILongShort(longShortAddress).oracleManagers(marketIndex)
    );
    // TODO: for some markets (eg flippening) this needs to be `updatePrice()` which isn't a view function!
    int256 latestAssetPrice = oracle.getLatestPrice();

    int256 percentChange = ((currentAssetPrice - latestAssetPrice) * 1e18) / currentAssetPrice;
    int256 percentChangeThresholdUnsigned = int256(percentChangeThreshold[marketIndex]);
    return
      percentChange > percentChangeThresholdUnsigned ||
      -percentChange > percentChangeThresholdUnsigned;
  }

  // This function is the same as the `isPriceChangeOverThreshold` but it isn't a view function, so it can simulate on oracle that requires an update
  function isPriceChangeOverThresholdNotView(uint32 marketIndex) public returns (bool, int256) {
    int256 currentAssetPrice = ILongShort(longShortAddress).assetPrice(marketIndex);
    IOracleManager oracle = IOracleManager(
      ILongShort(longShortAddress).oracleManagers(marketIndex)
    );
    int256 latestAssetPrice = oracle.updatePrice();

    int256 percentChange = ((currentAssetPrice - latestAssetPrice) * 1e18) / currentAssetPrice;
    int256 percentChangeThresholdUnsigned = int256(percentChangeThreshold[marketIndex]);

    return (
      percentChange > percentChangeThresholdUnsigned ||
        -percentChange > percentChangeThresholdUnsigned,
      percentChange
    );
  }

  function areTherePendingNextPriceActions(uint32 marketIndex) public view returns (bool) {
    return
      ILongShort(longShortAddress).batched_amountPaymentToken_deposit(marketIndex, true) > 0 ||
      ILongShort(longShortAddress).batched_amountSyntheticToken_redeem(marketIndex, true) > 0 ||
      ILongShort(longShortAddress).batched_amountSyntheticToken_toShiftAwayFrom_marketSide(
        marketIndex,
        true
      ) >
      0 ||
      ILongShort(longShortAddress).batched_amountPaymentToken_deposit(marketIndex, false) > 0 ||
      ILongShort(longShortAddress).batched_amountSyntheticToken_redeem(marketIndex, false) > 0 ||
      ILongShort(longShortAddress).batched_amountSyntheticToken_toShiftAwayFrom_marketSide(
        marketIndex,
        false
      ) >
      0;
  }

  function pendingNextPriceActionsBreakdown(uint32 marketIndex)
    public
    view
    returns (
      uint256,
      uint256,
      uint256,
      uint256,
      uint256,
      uint256
    )
  {
    return (
      ILongShort(longShortAddress).batched_amountPaymentToken_deposit(marketIndex, true),
      ILongShort(longShortAddress).batched_amountSyntheticToken_redeem(marketIndex, true),
      ILongShort(longShortAddress).batched_amountSyntheticToken_toShiftAwayFrom_marketSide(
        marketIndex,
        true
      ),
      ILongShort(longShortAddress).batched_amountPaymentToken_deposit(marketIndex, false),
      ILongShort(longShortAddress).batched_amountSyntheticToken_redeem(marketIndex, false),
      ILongShort(longShortAddress).batched_amountSyntheticToken_toShiftAwayFrom_marketSide(
        marketIndex,
        false
      )
    );
  }

  function pendingNextPriceActionsBreakdownCall(uint32 marketIndex) public {
    pendingNextPriceActionsBreakdown(marketIndex);
  }

  // TODO: make this conform to the 'keeper' standard
  function shouldUpdateOracle(uint32 marketIndex) external view returns (bool) {
    // was the last update longer than (update threshold ago)
    if (wasLastUpdateOlderThanThreshold(marketIndex)) {
      return true;
    }
    // is the current price greater than a certain threshold?
    if (isPriceChangeOverThreshold(marketIndex)) {
      return true;
    }
    // are there pending nextPriceActions?
    if (areTherePendingNextPriceActions(marketIndex)) {
      return true;
    }
  }

  // This function is the same as the `shouldUpdateOracle` but it isn't a view function, so it can simulate on oracle that requires an update
  function shouldUpdateOracleNotView(uint32 marketIndex) external returns (bool) {
    // was the last update longer than (update threshold ago)
    if (wasLastUpdateOlderThanThreshold(marketIndex)) {
      return true;
    }
    // is the current price greater than a certain threshold?
    (bool priceChangeIsOverThreshold, ) = isPriceChangeOverThresholdNotView(marketIndex);
    if (priceChangeIsOverThreshold) {
      return true;
    }
    // are there pending nextPriceActions?
    if (areTherePendingNextPriceActions(marketIndex)) {
      return true;
    }
  }

  function shouldUpdateOracleNotViewBreakdown(uint32 marketIndex)
    public
    returns (
      bool,
      bool,
      int256,
      bool
    )
  {
    (
      bool priceChangeIsOverThreshold,
      int256 priceChangePercent_1e18
    ) = isPriceChangeOverThresholdNotView(marketIndex);
    return (
      wasLastUpdateOlderThanThreshold(marketIndex),
      priceChangeIsOverThreshold,
      priceChangePercent_1e18,
      areTherePendingNextPriceActions(marketIndex)
    );
  }

  function shouldUpdateOracleNotViewBreakdownWithUpdateIndex(uint32 marketIndex)
    external
    returns (
      bool,
      bool,
      int256,
      bool,
      uint256
    )
  {
    (
      bool wasLastUpdateOlderThanThreshold,
      bool priceChangeIsOverThreshold,
      int256 priceChangePercent_1e18,
      bool areTherePendingNextPriceActions
    ) = shouldUpdateOracleNotViewBreakdown(marketIndex);
    return (
      wasLastUpdateOlderThanThreshold,
      priceChangeIsOverThreshold,
      priceChangePercent_1e18,
      areTherePendingNextPriceActions,
      ILongShort(longShortAddress).marketUpdateIndex(marketIndex)
    );
  }
}
