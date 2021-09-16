// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.3;

interface IStaker {
  /*╔════════════════════════════╗
    ║           EVENTS           ║
    ╚════════════════════════════╝*/

  event StakerV1(
    address admin,
    address floatTreasury,
    address floatCapital,
    address floatToken,
    uint256 floatPercentage
  );

  event MarketAddedToStaker(
    uint32 marketIndex,
    uint256 exitFee_e18,
    uint256 period,
    uint256 multiplier,
    uint256 balanceIncentiveExponent,
    int256 balanceIncentiveEquilibriumOffset,
    uint256 safeExponentBitShifting
  );

  event AccumulativeIssuancePerStakedSynthSnapshotCreated(
    uint32 marketIndex,
    uint256 accumulativeFloatIssuanceSnapshotIndex,
    uint256 accumulativeLong,
    uint256 accumulativeShort
  );

  event StakeAdded(address user, address token, uint256 amount, uint256 lastMintIndex);

  event StakeWithdrawn(address user, address token, uint256 amount);

  // Note: the `amountFloatMinted` isn't strictly needed by the graph, but it is good to add it to validate calculations are accurate.
  event FloatMinted(address user, uint32 marketIndex, uint256 amountFloatMinted);

  event MarketLaunchIncentiveParametersChanges(
    uint32 marketIndex,
    uint256 period,
    uint256 multiplier
  );

  event StakeWithdrawalFeeUpdated(uint32 marketIndex, uint256 stakeWithdralFee);

  event BalanceIncentiveParamsUpdated(
    uint32 marketIndex,
    uint256 balanceIncentiveExponent,
    int256 balanceIncentiveCurve_equilibriumOffset,
    uint256 safeExponentBitShifting
  );

  event FloatPercentageUpdated(uint256 floatPercentage);

  event NextPriceStakeShift(
    address user,
    uint32 marketIndex,
    uint256 amount,
    bool isShiftFromLong,
    uint256 userShiftIndex
  );

  // only for graph validation
  event StakeShifted(
    address user,
    uint32 marketIndex,
    uint256 newAmountStakedLong,
    uint256 newAmountStakedShort
  );

  function userAmountStaked(address, address) external view returns (uint256);

  function addNewStakingFund(
    uint32 marketIndex,
    address longTokenAddress,
    address shortTokenAddress,
    uint256 kInitialMultiplier,
    uint256 kPeriod,
    uint256 unstakeFee_e18,
    uint256 _balanceIncentiveCurve_exponent,
    int256 _balanceIncentiveCurve_equilibriumOffset
  ) external;

  function pushUpdatedMarketPricesToUpdateFloatIssuanceCalculations(
    uint32 marketIndex,
    uint256 marketUpdateIndex,
    uint256 longTokenPrice,
    uint256 shortTokenPrice,
    uint256 longValue,
    uint256 shortValue
  ) external;

  function stakeFromUser(address from, uint256 amount) external;

  function shiftTokens(
    uint256 amountSyntheticTokensToShift,
    uint32 marketIndex,
    bool isShiftFromLong
  ) external;
}
