// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.10;

interface ILongShort {
  /*╔════════════════════════════╗
    ║           EVENTS           ║
    ╚════════════════════════════╝*/

  event Upgrade(uint256 version);
  event LongShortV1(address admin, address tokenFactory, address staker);

  event SystemStateUpdated(
    uint32 marketIndex,
    uint256 updateIndex,
    int256 underlyingAssetPrice,
    uint256 longValue,
    uint256 shortValue,
    uint256 longPrice,
    uint256 shortPrice
  );

  event SyntheticMarketCreated(
    uint32 marketIndex,
    address longTokenAddress,
    address shortTokenAddress,
    address paymentToken,
    int256 initialAssetPrice,
    string name,
    string symbol,
    address oracleAddress,
    address yieldManagerAddress
  );

  event NextPriceRedeem(
    uint32 marketIndex,
    bool isLong,
    uint256 synthRedeemed,
    address user,
    uint256 oracleUpdateIndex
  );

  event NextPriceSyntheticPositionShift(
    uint32 marketIndex,
    bool isShiftFromLong,
    uint256 synthShifted,
    address user,
    uint256 oracleUpdateIndex
  );

  event NextPriceDeposit(
    uint32 marketIndex,
    bool isLong,
    uint256 depositAdded,
    address user,
    uint256 oracleUpdateIndex
  );

  event NextPriceDepositAndStake(
    uint32 marketIndex,
    bool isLong,
    uint256 amountToStake,
    address user,
    uint256 oracleUpdateIndex
  );

  event OracleUpdated(uint32 marketIndex, address oldOracleAddress, address newOracleAddress);

  event NewMarketLaunchedAndSeeded(uint32 marketIndex, uint256 initialSeed, uint256 marketLeverage);

  event ExecuteNextPriceSettlementsUser(address user, uint32 marketIndex);

  event MarketFundingRateMultiplerChanged(uint32 marketIndex, uint256 fundingRateMultiplier_e18);

  function syntheticTokens(uint32, bool) external view returns (address);

  function assetPrice(uint32) external view returns (int256);

  function oracleManagers(uint32) external view returns (address);

  function latestMarket() external view returns (uint32);

  function marketUpdateIndex(uint32) external view returns (uint256);

  function batched_amountPaymentToken_deposit(uint32, bool) external view returns (uint256);

  function batched_amountSyntheticToken_redeem(uint32, bool) external view returns (uint256);

  function batched_amountSyntheticToken_toShiftAwayFrom_marketSide(uint32, bool)
    external
    view
    returns (uint256);

  function get_syntheticToken_priceSnapshot(uint32, uint256)
    external
    view
    returns (uint256, uint256);

  function get_syntheticToken_priceSnapshot_side(
    uint32,
    bool,
    uint256
  ) external view returns (uint256);

  function marketSideValueInPaymentToken(uint32 marketIndex)
    external
    view
    returns (uint128 marketSideValueInPaymentTokenLong, uint128 marketSideValueInPaymentTokenShort);

  function updateSystemState(uint32 marketIndex) external;

  function updateSystemStateMulti(uint32[] calldata marketIndex) external;

  function getUsersConfirmedButNotSettledSynthBalance(
    address user,
    uint32 marketIndex,
    bool isLong
  ) external view returns (uint256 confirmedButNotSettledBalance);

  function executeOutstandingNextPriceSettlementsUser(address user, uint32 marketIndex) external;

  function shiftPositionNextPrice(
    uint32 marketIndex,
    uint256 amountSyntheticTokensToShift,
    bool isShiftFromLong
  ) external;

  function shiftPositionFromLongNextPrice(uint32 marketIndex, uint256 amountSyntheticTokensToShift)
    external;

  function shiftPositionFromShortNextPrice(uint32 marketIndex, uint256 amountSyntheticTokensToShift)
    external;

  function getAmountSyntheticTokenToMintOnTargetSide(
    uint32 marketIndex,
    uint256 amountSyntheticTokenShiftedFromOneSide,
    bool isShiftFromLong,
    uint256 priceSnapshotIndex
  ) external view returns (uint256 amountSynthShiftedToOtherSide);

  function mintLongNextPrice(uint32 marketIndex, uint256 amount) external;

  function mintShortNextPrice(uint32 marketIndex, uint256 amount) external;

  function mintAndStakeNextPrice(
    uint32 marketIndex,
    uint256 amount,
    bool isLong
  ) external;

  function redeemLongNextPrice(uint32 marketIndex, uint256 amount) external;

  function redeemShortNextPrice(uint32 marketIndex, uint256 amount) external;
}
