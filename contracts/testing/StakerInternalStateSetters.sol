// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.3;

import "../Staker.sol";

/*
NOTE: This contract is for testing purposes only!
*/

contract StakerInternalStateSetters is Staker {
  ///////////////////////////////////////////////
  //////////// Test Helper Functions ////////////
  ///////////////////////////////////////////////
  // TODO: remove parts of this function that aren't necessary for the updated `_calculateAccumulatedFloat` funciton
  function setFloatRewardCalcParams(
    uint32 marketIndex,
    address longToken,
    address shortToken,
    uint256 newLatestRewardIndex,
    address user,
    uint256 usersLatestClaimedReward,
    uint256 accumulativeFloatPerTokenLatestLong,
    uint256 accumulativeFloatPerTokenLatestShort,
    uint256 accumulativeFloatPerTokenUserLong,
    uint256 accumulativeFloatPerTokenUserShort,
    uint256 newUserAmountStakedLong,
    uint256 newUserAmountStakedShort
  ) public {
    latestRewardIndex[marketIndex] = newLatestRewardIndex;
    userIndexOfLastClaimedReward[marketIndex][user] = usersLatestClaimedReward;
    syntheticTokens[marketIndex][true] = longToken;
    syntheticTokens[marketIndex][false] = shortToken;

    accumulativeFloatPerSyntheticTokenSnapshots[marketIndex][newLatestRewardIndex]
      .accumulativeFloatPerSyntheticToken_long = accumulativeFloatPerTokenLatestLong;

    accumulativeFloatPerSyntheticTokenSnapshots[marketIndex][usersLatestClaimedReward]
      .accumulativeFloatPerSyntheticToken_long = accumulativeFloatPerTokenUserLong;

    accumulativeFloatPerSyntheticTokenSnapshots[marketIndex][newLatestRewardIndex]
      .accumulativeFloatPerSyntheticToken_short = accumulativeFloatPerTokenLatestShort;

    accumulativeFloatPerSyntheticTokenSnapshots[marketIndex][usersLatestClaimedReward]
      .accumulativeFloatPerSyntheticToken_short = accumulativeFloatPerTokenUserShort;

    userAmountStaked[longToken][user] = newUserAmountStakedLong;
    userAmountStaked[shortToken][user] = newUserAmountStakedShort;
  }

  function setCalculateAccumulatedFloatInRangeGlobals(
    uint32 marketIndex,
    uint256 rewardIndexTo,
    uint256 rewardIndexFrom,
    uint256 syntheticRewardToLongToken,
    uint256 syntheticRewardFromLongToken,
    uint256 syntheticRewardToShortToken,
    uint256 syntheticRewardFromShortToken
  ) public {
    accumulativeFloatPerSyntheticTokenSnapshots[marketIndex][rewardIndexTo]
      .accumulativeFloatPerSyntheticToken_long = syntheticRewardToLongToken;
    accumulativeFloatPerSyntheticTokenSnapshots[marketIndex][rewardIndexTo]
      .accumulativeFloatPerSyntheticToken_short = syntheticRewardToShortToken;
    accumulativeFloatPerSyntheticTokenSnapshots[marketIndex][rewardIndexFrom]
      .accumulativeFloatPerSyntheticToken_long = syntheticRewardFromLongToken;
    accumulativeFloatPerSyntheticTokenSnapshots[marketIndex][rewardIndexFrom]
      .accumulativeFloatPerSyntheticToken_short = syntheticRewardFromShortToken;
  }

  function setShiftParams(
    uint32 marketIndex,
    address user,
    uint256 shiftAmountLong,
    uint256 shiftAmountShort,
    uint256 _userNextPrice_stakedActionIndex,
    uint256 _latestRewardIndex
  ) public {
    userNextPrice_stakedActionIndex[marketIndex][user] = _userNextPrice_stakedActionIndex;
    userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom[marketIndex][true][
      user
    ] = shiftAmountLong;
    userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom[marketIndex][false][
      user
    ] = shiftAmountShort;

    latestRewardIndex[marketIndex] = _latestRewardIndex;
  }

  function setShiftTokensParams(
    uint32 marketIndex,
    bool isShiftFromLong,
    address user,
    uint256 amountSyntheticTokensToShift,
    uint256 _userAmountStaked,
    uint256 _userNextPrice_stakedActionIndex,
    uint256 _latestRewardIndex,
    address syntheticToken
  ) public {
    userNextPrice_stakedActionIndex[marketIndex][user] = _userNextPrice_stakedActionIndex;
    latestRewardIndex[marketIndex] = _latestRewardIndex;

    if (isShiftFromLong) {
      userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom[marketIndex][true][
        user
      ] = amountSyntheticTokensToShift;
    } else {
      userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom[marketIndex][false][
        user
      ] = amountSyntheticTokensToShift;
    }

    syntheticTokens[marketIndex][isShiftFromLong] = syntheticToken;
    userAmountStaked[syntheticToken][user] = _userAmountStaked;
  }

  function setLongShort(address _longShort) public {
    longShort = _longShort;
  }

  function setLatestRewardIndexGlobals(uint32 marketIndex, uint256 _latestRewardIndex) external {
    latestRewardIndex[marketIndex] = _latestRewardIndex;
  }

  function setGetMarketLaunchIncentiveParametersParams(
    uint32 marketIndex,
    uint256 period,
    uint256 multiplier
  ) external {
    marketLaunchIncentive_period[marketIndex] = period;
    marketLaunchIncentive_multipliers[marketIndex] = multiplier;
  }

  function setGetKValueParams(uint32 marketIndex, uint256 timestamp) external {
    accumulativeFloatPerSyntheticTokenSnapshots[marketIndex][0].timestamp = timestamp;
  }

  function setStakeFromUserParams(
    address longshort,
    address token,
    uint32 marketIndexForToken,
    address user,
    uint256 _latestRewardIndex,
    uint256 _userAmountStaked,
    uint256 userLastRewardIndex
  ) external {
    latestRewardIndex[marketIndexForToken] = _latestRewardIndex;
    userAmountStaked[token][user] = _userAmountStaked;
    userIndexOfLastClaimedReward[marketIndexForToken][user] = userLastRewardIndex;

    longShort = address(longshort);
    marketIndexOfToken[token] = marketIndexForToken;
  }

  function setCalculateTimeDeltaParams(
    uint32 marketIndex,
    uint256 latestRewardIndexForMarket,
    uint256 timestamp
  ) external {
    accumulativeFloatPerSyntheticTokenSnapshots[marketIndex][latestRewardIndexForMarket]
      .timestamp = timestamp;
  }

  function setCalculateNewCumulativeRateParams(
    uint32 marketIndex,
    uint256 latestRewardIndexForMarket,
    uint256 accumFloatLong,
    uint256 accumFloatShort
  ) external {
    latestRewardIndex[marketIndex] = latestRewardIndexForMarket;
    accumulativeFloatPerSyntheticTokenSnapshots[marketIndex][latestRewardIndex[marketIndex]]
      .accumulativeFloatPerSyntheticToken_long = accumFloatLong;

    accumulativeFloatPerSyntheticTokenSnapshots[marketIndex][latestRewardIndex[marketIndex]]
      .accumulativeFloatPerSyntheticToken_short = accumFloatShort;
  }

  function setSetRewardObjectsParams(uint32 marketIndex, uint256 latestRewardIndexForMarket)
    external
  {
    latestRewardIndex[marketIndex] = latestRewardIndexForMarket;
  }

  function set_updateStateParams(
    address _longShort,
    address token,
    uint32 tokenMarketIndex
  ) public {
    longShort = _longShort;
    marketIndexOfToken[token] = tokenMarketIndex;
  }

  function set_mintFloatParams(address _floatToken, uint16 _floatPercentage) public {
    floatToken = _floatToken;
    floatPercentage = _floatPercentage;
  }

  function setMintAccumulatedFloatAndClaimFloatParams(
    uint32 marketIndex,
    uint256 latestRewardIndexForMarket
  ) public {
    latestRewardIndex[marketIndex] = latestRewardIndexForMarket;
  }

  function set_withdrawGlobals(
    uint32 marketIndex,
    address syntheticToken,
    address user,
    uint256 amountStaked,
    uint256 fees,
    address treasury
  ) external {
    marketIndexOfToken[syntheticToken] = marketIndex;
    marketUnstakeFee_e18[marketIndex] = fees;
    userAmountStaked[syntheticToken][user] = amountStaked;
    floatTreasury = treasury;
  }

  function setWithdrawGlobals(
    uint32 marketIndex,
    address _longShort,
    address token
  ) external {
    marketIndexOfToken[token] = marketIndex;
    longShort = _longShort;
  }

  function setWithdrawAllGlobals(
    uint32 marketIndex,
    address _longShort,
    address user,
    uint256 amountStaked,
    address token,
    uint256 _userNextPrice_stakedActionIndex,
    address _syntheticTokens,
    uint256 _userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom_long,
    uint256 _userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom_short
  ) external {
    marketIndexOfToken[token] = marketIndex;
    longShort = _longShort;
    userAmountStaked[token][user] = amountStaked;
    userNextPrice_stakedActionIndex[marketIndex][user] = _userNextPrice_stakedActionIndex;
    syntheticTokens[marketIndex][true] = _syntheticTokens;
    userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom[marketIndex][true][
      user
    ] = _userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom_long;
    userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom[marketIndex][false][
      user
    ] = _userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom_short;
  }

  function setEquilibriumOffset(uint32 marketIndex, int256 _balanceIncentiveCurve_equilibriumOffset)
    external
  {
    balanceIncentiveCurve_equilibriumOffset[marketIndex] = _balanceIncentiveCurve_equilibriumOffset;
  }

  ///////////////////////////////////////////////////////
  //////////// Functions for Experimentation ////////////
  ///////////////////////////////////////////////////////

  function getRequiredAmountOfBitShiftForSafeExponentiationPerfect(uint256 number, uint256 exponent)
    external
    pure
    returns (uint256 amountOfBitShiftRequired)
  {
    uint256 targetMaxNumberSizeBinaryDigits = 257 / exponent;

    // Note this can be optimised, this gets a quick easy to compute safe upper bound, not the actuall upper bound.
    uint256 targetMaxNumber = 2**targetMaxNumberSizeBinaryDigits;

    while (number >> amountOfBitShiftRequired > targetMaxNumber) {
      ++amountOfBitShiftRequired;
    }
  }
}
