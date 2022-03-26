// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.10;

import "@openzeppelin/contracts-upgradeable/token/ERC20/presets/ERC20PresetMinterPauserUpgradeable.sol";
import "@openzeppelin/contracts/token/ERC20/utils/SafeERC20.sol";

import "../../util/SafeCastExtended.sol";
import "../../abstract/AccessControlledAndUpgradeable.sol";

import "../../interfaces/IFloatToken.sol";
import "../../interfaces/ILongShort.sol";
import "../../interfaces/IStaker.sol";
import "../../interfaces/ISyntheticToken.sol";
import "../../GEMS.sol";

// import "hardhat/console.sol";

contract Staker is IStaker, AccessControlledAndUpgradeable {
  using SafeERC20 for IERC20;
  /*╔═════════════════════════════╗
    ║          VARIABLES          ║
    ╚═════════════════════════════╝*/

  /* ══════ CONFIG ══════ */

  bytes32 public constant DISCOUNT_ROLE = keccak256("DISCOUNT_ROLE");

  /* ══════ Fixed-precision constants ══════ */
  uint256 public constant FLOAT_ISSUANCE_FIXED_DECIMAL = 3e44;

  /* ══════ Global state ══════ */
  address public floatCapital;
  address public floatTreasury;
  uint256 public floatPercentage;

  address public longShort;
  address public floatToken;

  address public gems;
  uint256[45] private __globalStateGap;

  /* ══════ Market specific ══════ */
  mapping(uint32 => uint256) public marketLaunchIncentive_period; // seconds
  mapping(uint32 => uint256) public marketLaunchIncentive_multipliers; // e18 scale
  mapping(uint32 => uint256) public marketUnstakeFee_e18;
  mapping(uint32 => uint256) public balanceIncentiveCurve_exponent;
  mapping(uint32 => int256) public balanceIncentiveCurve_equilibriumOffset;
  mapping(uint32 => uint256) public safeExponentBitShifting;

  mapping(uint32 => mapping(bool => address)) public syntheticTokens;
  uint256[45] private __marketStateGap;

  mapping(address => uint32) public marketIndexOfToken;
  mapping(address => uint32) public userNonce;
  uint256[45] private __synthStateGap;

  /* ══════ Reward specific ══════ */
  mapping(uint32 => uint256) public override latestRewardIndex; // This is synced to be the same as LongShort
  mapping(uint32 => mapping(uint256 => AccumulativeIssuancePerStakedSynthSnapshotLEGACY))
    public accumulativeFloatPerSyntheticTokenSnapshotsLEGACY;
  struct AccumulativeIssuancePerStakedSynthSnapshotLEGACY {
    uint256 timestamp;
    uint256 accumulativeFloatPerSyntheticToken_long;
    uint256 accumulativeFloatPerSyntheticToken_short;
  }

  uint256 public constant ACCUMULATIVE_FLT_ISSUANCE_SAFTEY_BIT_SHIFTING = 80;

  struct AccumulativeIssuancePerStakedSynthSnapshot {
    uint32 timestamp; // Large enough to last until Feb 07, 2106!
    uint112 accumulativeFloatPerSyntheticToken_long;
    uint112 accumulativeFloatPerSyntheticToken_short;
  }
  mapping(uint32 => mapping(uint256 => AccumulativeIssuancePerStakedSynthSnapshot))
    public accumulativeFloatPerSyntheticTokenSnapshots;

  uint256[44] private __rewardStateGap;
  /* ══════ User specific ══════ */
  mapping(uint32 => mapping(address => uint256)) public userIndexOfLastClaimedReward;
  mapping(address => mapping(address => uint256)) public override userAmountStaked;
  uint256[45] private __userStateGap;

  /* ══════ Next price action management specific ══════ */
  /// @dev marketIndex => usersAddress => stakedActionIndex
  mapping(uint32 => mapping(address => uint256)) public userNextPrice_stakedActionIndex;

  /// @dev marketIndex => usersAddress => amountUserRequestedToShiftAwayFromLongOnNextUpdate
  mapping(uint32 => mapping(bool => mapping(address => uint256)))
    public userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom;

  /// @dev marketIndex => usersAddress => stakedActionIndex
  mapping(uint32 => mapping(bool => mapping(address => uint256)))
    public userNextPrice_paymentToken_depositAmount;

  /*╔═════════════════════════════╗
    ║          MODIFIERS          ║
    ╚═════════════════════════════╝*/

  function onlyAdminModifierLogic() internal virtual {
    _checkRole(ADMIN_ROLE, msg.sender);
  }

  modifier onlyAdmin() {
    onlyAdminModifierLogic();
    _;
  }

  function onlyValidSyntheticModifierLogic(address _synth) internal virtual {
    require(marketIndexOfToken[_synth] != 0, "not valid synth");
  }

  modifier onlyValidSynthetic(address _synth) {
    onlyValidSyntheticModifierLogic(_synth);
    _;
  }

  function onlyLongShortModifierLogic() internal virtual {
    require(msg.sender == address(longShort), "not LongShort");
  }

  modifier onlyLongShort() {
    onlyLongShortModifierLogic();
    _;
  }

  function _updateUsersStakedPosition_mintAccumulatedFloatAndExecuteOutstandingNextPriceActions(
    uint32 marketIndex,
    address user
  ) internal virtual {
    uint256 userCurrentIndexOfLastClaimedReward = userIndexOfLastClaimedReward[marketIndex][user];
    uint256 userCurrentStakedActionIndex = userNextPrice_stakedActionIndex[marketIndex][user];
    // TODO: future optimization issues #2219
    //       Almost all functions that call this modifier also load the latest reward index.
    //       Save some gas by passing this value in and reading from storage only once.
    //       Similar optimizations possible for other users
    uint256 currentRewardIndex = latestRewardIndex[marketIndex];
    if (
      (userCurrentIndexOfLastClaimedReward != 0 &&
        userCurrentIndexOfLastClaimedReward < currentRewardIndex) ||
      (userCurrentStakedActionIndex != 0 && userCurrentStakedActionIndex <= currentRewardIndex)
    ) {
      _mintAccumulatedFloatAndExecuteOutstandingActions(marketIndex, user);
    }
  }

  modifier updateUsersStakedPosition_mintAccumulatedFloatAndExecuteOutstandingNextPriceActions(
    uint32 marketIndex,
    address user
  ) {
    _updateUsersStakedPosition_mintAccumulatedFloatAndExecuteOutstandingNextPriceActions(
      marketIndex,
      user
    );
    _;
  }

  modifier gemCollecting(address user) {
    GEMS(gems).gm(user);
    _;
  }

  /*╔═════════════════════════════════════════════════════╗
    ║          NETWORK SPECIFIC CONFIG FUNCTIONS          ║
    ╚═════════════════════════════════════════════════════╝*/

  /// @dev This contract uses legacy data.
  function HAS_LEGACY_DATA() internal pure virtual returns (bool) {
    return true;
  }

  /// @dev The this contract has markets with active flt issuance multipliers and intends on using them in the future.
  function HAS_FLT_ISSUANCE_MULTIPLIER() internal pure virtual returns (bool) {
    return true;
  }

  /// @dev The NFT discounts are enabled.
  function NFT_DISCOUNTS_ENABLED() internal pure virtual returns (bool) {
    return true;
  }

  /*╔═════════════════════════════╗
    ║       CONTRACT SET-UP       ║
    ╚═════════════════════════════╝*/

  /**
  @notice Initializes the contract.
  @dev Calls OpenZeppelin's initializer modifier.
  @param _admin Address of the admin role.
  @param _longShort Address of the LongShort contract, a deployed LongShort.sol
  @param _floatToken Address of the Float token earned by staking.
  @param _floatTreasury Address of the treasury contract for managing fees.
  @param _floatCapital Address of the contract which earns a fixed percentage of Float.
  @param _floatPercentage Determines the float percentage that gets minted for Float Capital, base 1e18.
  */
  function initialize(
    address _admin,
    address _longShort,
    address _floatToken,
    address _floatTreasury,
    address _floatCapital,
    address _discountSigner,
    uint256 _floatPercentage,
    address _gems
  ) public virtual {
    // The below function ensures that this contract can't be re-initialized!
    _AccessControlledAndUpgradeable_init(_admin);

    require(
      _admin != address(0) &&
        _longShort != address(0) &&
        _floatToken != address(0) &&
        _floatTreasury != address(0) &&
        _floatCapital != address(0) &&
        _gems != address(0) &&
        _floatPercentage != 0
    );

    floatCapital = _floatCapital;
    floatTreasury = _floatTreasury;
    longShort = _longShort;
    floatToken = _floatToken;
    gems = _gems;

    _setupRole(DISCOUNT_ROLE, _discountSigner);

    _changeFloatPercentage(_floatPercentage);

    emit StakerV1(_admin, _floatTreasury, _floatCapital, _floatToken, _floatPercentage);
  }

  /*╔═══════════════════╗
    ║       ADMIN       ║
    ╚═══════════════════╝*/

  /// @dev Logic for changeFloatPercentage
  function _changeFloatPercentage(uint256 newFloatPercentage) internal virtual {
    require(newFloatPercentage <= 1e18 && newFloatPercentage > 0); // less than or equal to 100% and greater than 0%
    floatPercentage = newFloatPercentage;
  }

  /**
  @notice Changes percentage of float that is minted for float capital.
  @param newFloatPercentage The new float percentage in base 1e18.
  */
  function changeFloatPercentage(uint256 newFloatPercentage) external onlyAdmin {
    _changeFloatPercentage(newFloatPercentage);
    emit FloatPercentageUpdated(newFloatPercentage);
  }

  /// @dev Logic for changeUnstakeFee
  function _changeUnstakeFee(uint32 marketIndex, uint256 newMarketUnstakeFee_e18) internal virtual {
    require(newMarketUnstakeFee_e18 <= 5e16); // Explicitly stating 5% fee as the max fee possible.
    marketUnstakeFee_e18[marketIndex] = newMarketUnstakeFee_e18;
  }

  /**
  @notice Changes unstake fee for a market
  @param marketIndex Identifies the market.
  @param newMarketUnstakeFee_e18 The new unstake fee.
  */
  function changeUnstakeFee(uint32 marketIndex, uint256 newMarketUnstakeFee_e18)
    external
    onlyAdmin
  {
    _changeUnstakeFee(marketIndex, newMarketUnstakeFee_e18);
    emit StakeWithdrawalFeeUpdated(marketIndex, newMarketUnstakeFee_e18);
  }

  /// @dev Logic for changeBalanceIncentiveExponent
  function _changeBalanceIncentiveParameters(
    uint32 marketIndex,
    uint256 _balanceIncentiveCurve_exponent,
    int256 _balanceIncentiveCurve_equilibriumOffset,
    uint256 _safeExponentBitShifting
  ) internal virtual {
    // Unreasonable that we would ever shift this more than 90% either way
    require(
      _balanceIncentiveCurve_equilibriumOffset > -9e17 &&
        _balanceIncentiveCurve_equilibriumOffset < 9e17,
      "balanceIncentiveCurve_equilibriumOffset out of bounds"
    );
    require(_balanceIncentiveCurve_exponent > 0, "balanceIncentiveCurve_exponent out of bounds");
    require(_safeExponentBitShifting < 100, "safeExponentBitShifting out of bounds");

    (uint128 lockedLong, uint128 lockedShort) = ILongShort(longShort).marketSideValueInPaymentToken(
      marketIndex
    );

    uint256 totalLocked = uint256(lockedLong) + uint256(lockedShort);

    // SafeMATH will revert here if this value is too big.
    (((totalLocked * 500) >> _safeExponentBitShifting)**_balanceIncentiveCurve_exponent);
    // Required to ensure at least 3 digits of precision.
    require(
      totalLocked >> _safeExponentBitShifting > 100,
      "bit shifting too large for total locked"
    );

    balanceIncentiveCurve_exponent[marketIndex] = _balanceIncentiveCurve_exponent;
    balanceIncentiveCurve_equilibriumOffset[marketIndex] = _balanceIncentiveCurve_equilibriumOffset;
    safeExponentBitShifting[marketIndex] = _safeExponentBitShifting;
  }

  /**
  @notice Changes the balance incentive exponent for a market
  @param marketIndex Identifies the market.
  @param _balanceIncentiveCurve_exponent The new exponent for the curve.
  @param _balanceIncentiveCurve_equilibriumOffset The new offset.
  @param _safeExponentBitShifting The new bitshifting applied to the curve.
  */
  function changeBalanceIncentiveParameters(
    uint32 marketIndex,
    uint256 _balanceIncentiveCurve_exponent,
    int256 _balanceIncentiveCurve_equilibriumOffset,
    uint256 _safeExponentBitShifting
  ) external onlyAdmin {
    _changeBalanceIncentiveParameters(
      marketIndex,
      _balanceIncentiveCurve_exponent,
      _balanceIncentiveCurve_equilibriumOffset,
      _safeExponentBitShifting
    );

    emit BalanceIncentiveParamsUpdated(
      marketIndex,
      _balanceIncentiveCurve_exponent,
      _balanceIncentiveCurve_equilibriumOffset,
      _safeExponentBitShifting
    );
  }

  /*╔═════════════════════════════╗
    ║        STAKING SETUP        ║
    ╚═════════════════════════════╝*/

  /**
  @notice Sets this contract to track staking for a market in LongShort.sol
  @param marketIndex Identifies the market.
  @param longToken Address of the long token for the market.
  @param shortToken Address of the short token for the market.
  @param kInitialMultiplier Initial boost on float generation for the market.
  @param kPeriod Period which the boost should last.
  @param unstakeFee_e18 Percentage of tokens that are levied on unstaking in base 1e18.
  @param _balanceIncentiveCurve_exponent Exponent for balance curve (see _calculateFloatPerSecond)
  @param _balanceIncentiveCurve_equilibriumOffset Offset for balance curve (see _calculateFloatPerSecond)
  */
  function addNewStakingFund(
    uint32 marketIndex,
    address longToken,
    address shortToken,
    uint256 kInitialMultiplier,
    uint256 kPeriod,
    uint256 unstakeFee_e18,
    uint256 _balanceIncentiveCurve_exponent,
    int256 _balanceIncentiveCurve_equilibriumOffset
  ) external override onlyLongShort {
    require(kInitialMultiplier >= 1e18, "kInitialMultiplier must be >= 1e18");

    // a safe initial default value
    uint256 initialSafeExponentBitShifting = 50;

    marketIndexOfToken[longToken] = marketIndex;
    marketIndexOfToken[shortToken] = marketIndex;

    accumulativeFloatPerSyntheticTokenSnapshots[marketIndex][0].timestamp = uint32(block.timestamp);

    syntheticTokens[marketIndex][true] = longToken;
    syntheticTokens[marketIndex][false] = shortToken;
    _changeBalanceIncentiveParameters(
      marketIndex,
      _balanceIncentiveCurve_exponent,
      _balanceIncentiveCurve_equilibriumOffset,
      initialSafeExponentBitShifting
    );

    if (HAS_FLT_ISSUANCE_MULTIPLIER()) {
      marketLaunchIncentive_period[marketIndex] = kPeriod;
      marketLaunchIncentive_multipliers[marketIndex] = kInitialMultiplier;
    } else {
      kPeriod = 0;
      kInitialMultiplier = 1e18;
    }

    _changeUnstakeFee(marketIndex, unstakeFee_e18);

    emit MarketAddedToStaker(
      marketIndex,
      unstakeFee_e18,
      kPeriod,
      kInitialMultiplier,
      _balanceIncentiveCurve_exponent,
      _balanceIncentiveCurve_equilibriumOffset,
      initialSafeExponentBitShifting
    );

    emit AccumulativeIssuancePerStakedSynthSnapshotCreated(marketIndex, 0, 0, 0);
  }

  /*╔═════════════════════════════════════════════════════════════════════════╗
    ║    GLOBAL FLT REWARD ACCUMULATION CALCULATION AND TRACKING FUNCTIONS    ║
    ╚═════════════════════════════════════════════════════════════════════════╝*/

  /**
  @notice Returns the K factor parameters for the given market with sensible
  defaults if they haven't been set yet.
  @param marketIndex The market to change the parameters for.
  @return period The period for which the k factor applies for in seconds.
  @return multiplier The multiplier on Float generation in this period.
  */
  function _getMarketLaunchIncentiveParameters(uint32 marketIndex)
    internal
    view
    virtual
    returns (uint256 period, uint256 multiplier)
  {
    period = marketLaunchIncentive_period[marketIndex]; // seconds TODO change name to contain seconds
    multiplier = marketLaunchIncentive_multipliers[marketIndex]; // 1e18 TODO change name to contain E18

    if (multiplier < 1e18) {
      multiplier = 1e18; // multiplier of 1 by default
    }
  }

  /**
  @notice Returns the extent to which a markets float generation should be adjusted
  based on the market's launch incentive parameters. Should start at multiplier
  then linearly change to 1e18 over time.
  @param marketIndex Identifies the market.
  @return k The calculated modifier for float generation.
  */
  function _getKValue(uint32 marketIndex) internal view virtual returns (uint256) {
    if (HAS_FLT_ISSUANCE_MULTIPLIER()) {
      // Parameters controlling the float issuance multiplier.
      (uint256 kPeriod, uint256 kInitialMultiplier) = _getMarketLaunchIncentiveParameters(
        marketIndex
      );

      // Sanity check - under normal circumstances, the multipliers should
      // *never* be set to a value < 1e18, as there are guards against this.
      assert(kInitialMultiplier >= 1e18);

      uint256 initialTimestamp = uint256(
        accumulativeFloatPerSyntheticTokenSnapshots[marketIndex][0].timestamp
      );

      if (block.timestamp - initialTimestamp < kPeriod) {
        return
          kInitialMultiplier -
          (((kInitialMultiplier - 1e18) * (block.timestamp - initialTimestamp)) / kPeriod);
      }
    }

    return 1e18;
  }

  /*
  @notice Computes the number of float tokens a user earns per second for
  every long/short synthetic token they've staked. The returned value has
  a fixed decimal scale of 1e42 (!!!) for numerical stability. The return
  values are float per second per synthetic token (hence the requirement
  to multiply by price)
  @dev to see below math in latex form see:
  https://ipfs.io/ipfs/QmRWbr8P1F588XqRTzm7wCsRPu8DcDVPWGriBach4f22Fq/staker-fps.pdf
  to interact with the equations see https://www.desmos.com/calculator/optkaxyihr
  @param marketIndex The market referred to.
  @param longPrice Price of the synthetic long token in units of payment token
  @param shortPrice Price of the synthetic short token in units of payment token
  @param longValue Amount of payment token in the long side of the market
  @param shortValue Amount of payment token in the short side of the market
  @return longFloatPerSecond Float token per second per long synthetic token
  @return shortFloatPerSecond Float token per second per short synthetic token
   */
  function _calculateFloatPerSecond(
    uint32 marketIndex,
    uint256 longPrice,
    uint256 shortPrice,
    uint256 longValue,
    uint256 shortValue
  ) internal view virtual returns (uint256 longFloatPerSecond, uint256 shortFloatPerSecond) {
    // A float issuance multiplier that starts high and decreases linearly
    // over time to a value of 1. This incentivises users to stake early.
    uint256 k = _getKValue(marketIndex);

    uint256 totalLocked = (longValue + shortValue);

    // we need to scale this number by the totalLocked so that the offset remains consistent accross market size

    int256 equilibriumOffsetMarketScaled = (balanceIncentiveCurve_equilibriumOffset[marketIndex] *
      int256(totalLocked)) / 2e18;

    uint256 safetyBitShifting = safeExponentBitShifting[marketIndex];

    // Float is scaled by the percentage of the total market value held in
    // the opposite position. This incentivises users to stake on the
    // weaker position.
    if (int256(shortValue) - (2 * equilibriumOffsetMarketScaled) < int256(longValue)) {
      if (equilibriumOffsetMarketScaled >= int256(shortValue)) {
        // edge case: imbalanced far past the equilibrium offset - full rewards go to short token
        //            extremely unlikely to happen in practice
        return (0, k * shortPrice);
      }

      uint256 numerator = (uint256(int256(shortValue) - equilibriumOffsetMarketScaled) >>
        (safetyBitShifting - 1))**balanceIncentiveCurve_exponent[marketIndex];

      uint256 denominator = ((totalLocked >> safetyBitShifting) **
        balanceIncentiveCurve_exponent[marketIndex]);

      // NOTE: `x * 5e17` == `(x * 1e18) / 2`
      uint256 longRewardUnscaled = (numerator * 5e17) / denominator;
      uint256 shortRewardUnscaled = 1e18 - longRewardUnscaled;

      return (
        (longRewardUnscaled * k * longPrice) / 1e18,
        (shortRewardUnscaled * k * shortPrice) / 1e18
      );
    } else {
      if (-equilibriumOffsetMarketScaled >= int256(longValue)) {
        // edge case: imbalanced far past the equilibrium offset - full rewards go to long token
        //            extremely unlikely to happen in practice
        return (k * longPrice, 0);
      }

      uint256 numerator = (uint256(int256(longValue) + equilibriumOffsetMarketScaled) >>
        (safetyBitShifting - 1))**balanceIncentiveCurve_exponent[marketIndex];

      uint256 denominator = ((totalLocked >> safetyBitShifting) **
        balanceIncentiveCurve_exponent[marketIndex]);

      // NOTE: `x * 5e17` == `(x * 1e18) / 2`
      uint256 shortRewardUnscaled = (numerator * 5e17) / denominator;
      uint256 longRewardUnscaled = 1e18 - shortRewardUnscaled;

      return (
        (longRewardUnscaled * k * longPrice) / 1e18,
        (shortRewardUnscaled * k * shortPrice) / 1e18
      );
    }
  }

  /**
  @notice Computes new cumulative sum of 'r' value since last accumulativeIssuancePerStakedSynthSnapshotLEGACY. We use
  cumulative 'r' value to avoid looping during issuance. Note that the
  cumulative sum is kept in 1e42 scale (!!!) to avoid numerical issues.
  @param shortValue The value locked in the short side of the market.
  @param longValue The value locked in the long side of the market.
  @param shortPrice The price of the short token as defined in LongShort.sol
  @param longPrice The price of the long token as defined in LongShort.sol
  @param marketIndex An identifier for the market.
  @return longCumulativeRates The long cumulative sum.
  @return shortCumulativeRates The short cumulative sum.
  */
  function _calculateNewCumulativeIssuancePerStakedSynth(
    uint32 marketIndex,
    uint256 previousMarketUpdateIndex,
    uint256 longPrice,
    uint256 shortPrice,
    uint256 longValue,
    uint256 shortValue
  ) internal view virtual returns (uint256 longCumulativeRates, uint256 shortCumulativeRates) {
    // Compute the current 'r' value for float issuance per second.
    (uint256 longFloatPerSecond, uint256 shortFloatPerSecond) = _calculateFloatPerSecond(
      marketIndex,
      longPrice,
      shortPrice,
      longValue,
      shortValue
    );

    AccumulativeIssuancePerStakedSynthSnapshot
      storage latestAccumulativeIssuancePerStakedSynthSnapshot = accumulativeFloatPerSyntheticTokenSnapshots[
        marketIndex
      ][previousMarketUpdateIndex];

    // Compute time since last accumulativeIssuancePerStakedSynthSnapshot for the given token.
    uint256 timeDelta = block.timestamp -
      uint256(latestAccumulativeIssuancePerStakedSynthSnapshot.timestamp);
    // Compute new cumulative 'r' value total.
    return (
      (uint256(
        latestAccumulativeIssuancePerStakedSynthSnapshot.accumulativeFloatPerSyntheticToken_long
      ) << ACCUMULATIVE_FLT_ISSUANCE_SAFTEY_BIT_SHIFTING) + (timeDelta * longFloatPerSecond),
      (uint256(
        latestAccumulativeIssuancePerStakedSynthSnapshot.accumulativeFloatPerSyntheticToken_short
      ) << ACCUMULATIVE_FLT_ISSUANCE_SAFTEY_BIT_SHIFTING) + (timeDelta * shortFloatPerSecond)
    );
  }

  /**
  @notice Adds new accumulativeIssuancePerStakedSynthSnapshots for the given long/short tokens. Called by the
  ILongShort contract whenever there is a state change for a market.
  @param marketIndex An identifier for the market.
  @param marketUpdateIndex Current update index in the LongShort contract for this market.
  @param shortValue The value locked in the short side of the market.
  @param longValue The value locked in the long side of the market.
  @param shortPrice The price of the short token as defined in LongShort.sol
  @param longPrice The price of the long token as defined in LongShort.sol
  */
  function pushUpdatedMarketPricesToUpdateFloatIssuanceCalculations(
    uint32 marketIndex,
    uint256 marketUpdateIndex,
    uint256 longPrice,
    uint256 shortPrice,
    uint256 longValue,
    uint256 shortValue
  ) external override onlyLongShort {
    (
      uint256 newLongAccumulativeValue,
      uint256 newShortAccumulativeValue
    ) = _calculateNewCumulativeIssuancePerStakedSynth(
        marketIndex,
        marketUpdateIndex - 1,
        longPrice,
        shortPrice,
        longValue,
        shortValue
      );

    accumulativeFloatPerSyntheticTokenSnapshots[marketIndex][
      marketUpdateIndex
    ] = AccumulativeIssuancePerStakedSynthSnapshot(
      // Set timestamp on new accumulativeIssuancePerStakedSynthSnapshot.
      uint32(block.timestamp),
      // Set cumulative 'r' value on new accumulativeIssuancePerStakedSynthSnapshot.
      SafeCastExtended.toUint112(
        newLongAccumulativeValue >> ACCUMULATIVE_FLT_ISSUANCE_SAFTEY_BIT_SHIFTING
      ),
      SafeCastExtended.toUint112(
        newShortAccumulativeValue >> ACCUMULATIVE_FLT_ISSUANCE_SAFTEY_BIT_SHIFTING
      )
    );

    // Update latest index to point to new accumulativeIssuancePerStakedSynthSnapshot.
    latestRewardIndex[marketIndex] = marketUpdateIndex;

    emit AccumulativeIssuancePerStakedSynthSnapshotCreated(
      marketIndex,
      marketUpdateIndex,
      newLongAccumulativeValue,
      newShortAccumulativeValue
    );
  }

  /*╔═══════════════════════════════════╗
    ║    USER REWARD STATE FUNCTIONS    ║
    ╚═══════════════════════════════════╝*/

  /// @dev Calculates the accumulated float in a specific range of staker snapshots
  function _calculateAccumulatedFloatInRange(
    uint32 marketIndex,
    uint256 amountStakedLong,
    uint256 amountStakedShort,
    uint256 rewardIndexFrom,
    uint256 rewardIndexTo
  ) internal view virtual returns (uint256 floatReward) {
    AccumulativeIssuancePerStakedSynthSnapshot
      storage accumulativeFloatPerSyntheticTokenSnapshotTo = accumulativeFloatPerSyntheticTokenSnapshots[
        marketIndex
      ][rewardIndexTo];
    AccumulativeIssuancePerStakedSynthSnapshot
      storage accumulativeFloatPerSyntheticTokenSnapshotFrom = accumulativeFloatPerSyntheticTokenSnapshots[
        marketIndex
      ][rewardIndexFrom];
    if (amountStakedLong > 0) {
      uint256 accumulativeLongStartingPoint = uint256(
        accumulativeFloatPerSyntheticTokenSnapshotFrom.accumulativeFloatPerSyntheticToken_long
      );
      uint256 accumulativeLongEndingPoint = uint256(
        accumulativeFloatPerSyntheticTokenSnapshotTo.accumulativeFloatPerSyntheticToken_long
      );
      if (HAS_LEGACY_DATA()) {
        if (accumulativeLongStartingPoint == 0) {
          // If this values is zero it must be still using the legacy snapshot syntax.
          accumulativeLongStartingPoint =
            accumulativeFloatPerSyntheticTokenSnapshotsLEGACY[marketIndex][rewardIndexFrom]
              .accumulativeFloatPerSyntheticToken_long >>
            ACCUMULATIVE_FLT_ISSUANCE_SAFTEY_BIT_SHIFTING;
        }
        if (accumulativeLongEndingPoint == 0) {
          // If this values is zero it must be still using the legacy snapshot syntax.
          accumulativeLongEndingPoint =
            accumulativeFloatPerSyntheticTokenSnapshotsLEGACY[marketIndex][rewardIndexTo]
              .accumulativeFloatPerSyntheticToken_long >>
            ACCUMULATIVE_FLT_ISSUANCE_SAFTEY_BIT_SHIFTING;
        }
      }

      uint256 accumDeltaLong = (accumulativeLongEndingPoint - accumulativeLongStartingPoint) <<
        ACCUMULATIVE_FLT_ISSUANCE_SAFTEY_BIT_SHIFTING;
      floatReward += (accumDeltaLong * amountStakedLong) / FLOAT_ISSUANCE_FIXED_DECIMAL;
    }

    if (amountStakedShort > 0) {
      uint256 accumulativeShortStartingPoint = uint256(
        accumulativeFloatPerSyntheticTokenSnapshotFrom.accumulativeFloatPerSyntheticToken_short
      );
      uint256 accumulativeShortEndingPoint = uint256(
        accumulativeFloatPerSyntheticTokenSnapshotTo.accumulativeFloatPerSyntheticToken_short
      );
      if (HAS_LEGACY_DATA()) {
        if (accumulativeShortStartingPoint == 0) {
          // If this values is zero it must be still using the legacy snapshot syntax.
          accumulativeShortStartingPoint =
            accumulativeFloatPerSyntheticTokenSnapshotsLEGACY[marketIndex][rewardIndexFrom]
              .accumulativeFloatPerSyntheticToken_short >>
            ACCUMULATIVE_FLT_ISSUANCE_SAFTEY_BIT_SHIFTING;
        }
        if (accumulativeShortEndingPoint == 0) {
          // If this values is zero it must be still using the legacy snapshot syntax.
          accumulativeShortEndingPoint =
            accumulativeFloatPerSyntheticTokenSnapshotsLEGACY[marketIndex][rewardIndexTo]
              .accumulativeFloatPerSyntheticToken_short >>
            ACCUMULATIVE_FLT_ISSUANCE_SAFTEY_BIT_SHIFTING;
        }
      }

      uint256 accumDeltaShort = (accumulativeShortEndingPoint - accumulativeShortStartingPoint) <<
        ACCUMULATIVE_FLT_ISSUANCE_SAFTEY_BIT_SHIFTING;
      floatReward += (accumDeltaShort * amountStakedShort) / FLOAT_ISSUANCE_FIXED_DECIMAL;
    }
  }

  /**
  @notice Calculates float owed to the user since the user last minted float for a market.
  @param marketIndex Identifier for the market which the user staked in.
  @param user The address of the user.
  @return floatReward The amount of float owed.
   */
  function _calculateAccumulatedFloatAndExecuteOutstandingActions(uint32 marketIndex, address user)
    internal
    virtual
    returns (uint256 floatReward)
  {
    address longToken = syntheticTokens[marketIndex][true];
    address shortToken = syntheticTokens[marketIndex][false];

    uint256 amountStakedLong = userAmountStaked[longToken][user];
    uint256 amountStakedShort = userAmountStaked[shortToken][user];

    uint256 usersLastRewardIndex = userIndexOfLastClaimedReward[marketIndex][user];

    uint256 currentRewardIndex = latestRewardIndex[marketIndex];

    // Don't do the calculation and return zero immediately if there is no change
    if (usersLastRewardIndex == currentRewardIndex) {
      return 0;
    }

    uint256 usersNextPriceActionIndex = userNextPrice_stakedActionIndex[marketIndex][user];
    // if there is a change in the users tokens held due to a token shift (or possibly another action in the future)
    if (usersNextPriceActionIndex > 0 && usersNextPriceActionIndex <= currentRewardIndex) {
      // calculate FLT reward up-until the next price action
      floatReward = _calculateAccumulatedFloatInRange(
        marketIndex,
        amountStakedLong,
        amountStakedShort,
        usersLastRewardIndex,
        usersNextPriceActionIndex
      );

      // Update the users balances

      uint256 amountForAction_userShiftOrMint = userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom[
          marketIndex
        ][true][user];
      // Handle shifts from LONG side:
      if (amountForAction_userShiftOrMint > 0) {
        amountStakedShort += ILongShort(longShort).getAmountSyntheticTokenToMintOnTargetSide(
          marketIndex,
          amountForAction_userShiftOrMint,
          true,
          usersNextPriceActionIndex
        );

        amountStakedLong -= amountForAction_userShiftOrMint;
        userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom[marketIndex][true][user] = 0;
      }

      amountForAction_userShiftOrMint = userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom[
        marketIndex
      ][false][user];
      // Handle shifts from SHORT side:
      if (amountForAction_userShiftOrMint > 0) {
        amountStakedLong += ILongShort(longShort).getAmountSyntheticTokenToMintOnTargetSide(
          marketIndex,
          amountForAction_userShiftOrMint,
          false,
          usersNextPriceActionIndex
        );

        amountStakedShort -= amountForAction_userShiftOrMint;
        userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom[marketIndex][false][user] = 0;
      }

      //Handle user next price mint and stake
      amountForAction_userShiftOrMint = userNextPrice_paymentToken_depositAmount[marketIndex][true][
        user
      ];
      // Handle mints from LONG side:
      if (amountForAction_userShiftOrMint > 0) {
        uint256 syntheticTokenValue = ILongShort(longShort).get_syntheticToken_priceSnapshot_side(
          marketIndex,
          true,
          usersNextPriceActionIndex
        );

        amountStakedLong += (amountForAction_userShiftOrMint * 1e18) / syntheticTokenValue;
        userNextPrice_paymentToken_depositAmount[marketIndex][true][user] = 0;
      }

      amountForAction_userShiftOrMint = userNextPrice_paymentToken_depositAmount[marketIndex][
        false
      ][user];
      // Handle mints from short side:
      if (amountForAction_userShiftOrMint > 0) {
        uint256 syntheticTokenValue = ILongShort(longShort).get_syntheticToken_priceSnapshot_side(
          marketIndex,
          false,
          usersNextPriceActionIndex
        );

        amountStakedShort += (amountForAction_userShiftOrMint * 1e18) / syntheticTokenValue;
        userNextPrice_paymentToken_depositAmount[marketIndex][false][user] = 0;
      }

      // Save the users updated staked amounts
      userAmountStaked[longToken][user] = amountStakedLong;
      userAmountStaked[shortToken][user] = amountStakedShort;

      floatReward += _calculateAccumulatedFloatInRange(
        marketIndex,
        amountStakedLong,
        amountStakedShort,
        usersNextPriceActionIndex,
        currentRewardIndex
      );

      userNextPrice_stakedActionIndex[marketIndex][user] = 0;
    } else {
      floatReward = _calculateAccumulatedFloatInRange(
        marketIndex,
        amountStakedLong,
        amountStakedShort,
        usersLastRewardIndex,
        currentRewardIndex
      );
    }
    // NOTE: We always set this value, even if the user doesn't have any FLT due to them.
    //       This gets rid of the possibility of a user claming FLT unfairly retro-actively.
    userIndexOfLastClaimedReward[marketIndex][user] = currentRewardIndex;
  }

  /**
  @notice Mints float for a user.
  @dev Mints a fixed percentage for Float capital.
  @param user The address of the user.
  @param floatToMint The amount of float to mint.
   */
  function _mintFloat(address user, uint256 floatToMint) internal virtual {
    IFloatToken(floatToken).mint(user, floatToMint);
    IFloatToken(floatToken).mint(floatCapital, (floatToMint * floatPercentage) / 1e18);
  }

  /**
  @notice Mints float owed to a user for a market since they last minted.
  @param marketIndex An identifier for the market.
  @param user The address of the user.
   */
  function _mintAccumulatedFloatAndExecuteOutstandingActions(uint32 marketIndex, address user)
    internal
    virtual
  {
    uint256 floatToMint = _calculateAccumulatedFloatAndExecuteOutstandingActions(marketIndex, user);

    if (floatToMint > 0) {
      _mintFloat(user, floatToMint);
      emit FloatMinted(user, marketIndex, floatToMint);
    }
  }

  /**
  @notice Mints float owed to a user for multiple markets, since they last minted for those markets.
  @param marketIndexes Identifiers for the markets.
  @param user The address of the user.
   */
  function _mintAccumulatedFloatAndExecuteOutstandingActionsMulti(
    uint32[] calldata marketIndexes,
    address user
  ) internal virtual {
    uint256 floatTotal = 0;
    uint256 length = marketIndexes.length;
    for (uint256 i = 0; i < length; i++) {
      uint256 floatToMint = _calculateAccumulatedFloatAndExecuteOutstandingActions(
        marketIndexes[i],
        user
      );

      if (floatToMint > 0) {
        floatTotal += floatToMint;

        emit FloatMinted(user, marketIndexes[i], floatToMint);
      }
    }
    if (floatTotal > 0) {
      _mintFloat(user, floatTotal);
    }
  }

  /**
  @notice Mints outstanding float for msg.sender.
  @param marketIndexes Identifiers for the markets for which to mint float.
   */
  function claimFloatCustom(uint32[] calldata marketIndexes) external gemCollecting(msg.sender) {
    ILongShort(longShort).updateSystemStateMulti(marketIndexes);
    _mintAccumulatedFloatAndExecuteOutstandingActionsMulti(marketIndexes, msg.sender);
  }

  /**
  @notice Mints outstanding float on behalf of another user.
  @param marketIndexes Identifiers for the markets for which to mint float.
  @param user The address of the user.
   */
  function claimFloatCustomFor(uint32[] calldata marketIndexes, address user) external {
    // Unbounded loop - users are responsible for paying their own gas costs on these and it doesn't effect the rest of the system.
    // No need to impose limit.
    ILongShort(longShort).updateSystemStateMulti(marketIndexes);
    _mintAccumulatedFloatAndExecuteOutstandingActionsMulti(marketIndexes, user);
  }

  /*╔═══════════════════════╗
    ║        STAKING        ║
    ╚═══════════════════════╝*/

  /**
  @notice A user with synthetic tokens stakes by calling stake on the token
  contract which calls this function. We need to first update the
  state of the LongShort contract for this market before staking to correctly calculate user rewards.
  @param amount Amount to stake.
  @param from Address to stake for.
  */
  function stakeFromUser(address from, uint256 amount)
    external
    virtual
    override
    onlyValidSynthetic(msg.sender)
    gemCollecting(from)
  {
    require(amount > 0, "Stake amount must be greater than 0");
    uint32 marketIndex = marketIndexOfToken[msg.sender];

    ILongShort(longShort).updateSystemState(marketIndex);

    _updateUsersStakedPosition_mintAccumulatedFloatAndExecuteOutstandingNextPriceActions(
      marketIndex,
      from
    );

    uint256 currentRewardIndex = latestRewardIndex[marketIndex];

    userAmountStaked[msg.sender][from] += amount;

    // NOTE: Users retroactively earn a little bit of FLT because they start earning from the previous update index.
    userIndexOfLastClaimedReward[marketIndex][from] = currentRewardIndex;

    emit StakeAdded(from, msg.sender, amount, currentRewardIndex);
  }

  /*╔═══════════════════════════╗
    ║       MINT & STAKE        ║
    ╚═══════════════════════════╝*/

  function mintAndStakeNextPrice(
    uint32 marketIndex,
    uint256 amount,
    bool isLong,
    address user
  )
    external
    virtual
    override
    onlyLongShort
    updateUsersStakedPosition_mintAccumulatedFloatAndExecuteOutstandingNextPriceActions(
      marketIndex,
      user
    )
  {
    userNextPrice_paymentToken_depositAmount[marketIndex][isLong][user] += amount;

    userNextPrice_stakedActionIndex[marketIndex][user] = latestRewardIndex[marketIndex] + 1;
  }

  /**
  @notice Allows users to shift their staked tokens from one side of the market to
  the other at the next price.
  @param amountSyntheticTokensToShift Amount of tokens to shift.
  @param marketIndex Identifier for the market.
  @param isShiftFromLong Whether the shift is from long to short or short to long.
  */
  function shiftTokens(
    uint256 amountSyntheticTokensToShift,
    uint32 marketIndex,
    bool isShiftFromLong
  )
    external
    virtual
    override
    updateUsersStakedPosition_mintAccumulatedFloatAndExecuteOutstandingNextPriceActions(
      marketIndex,
      msg.sender
    )
    gemCollecting(msg.sender)
  {
    require(amountSyntheticTokensToShift > 0, "No zero shifts.");

    ILongShort(longShort).checkIfUserIsEligibleToTrade(msg.sender, marketIndex, isShiftFromLong);
    ILongShort(longShort).setUserTradeTimer(msg.sender, marketIndex, !isShiftFromLong);

    address token = syntheticTokens[marketIndex][isShiftFromLong];
    uint256 totalAmountForNextShift = amountSyntheticTokensToShift +
      userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom[marketIndex][isShiftFromLong][
        msg.sender
      ];

    require(
      userAmountStaked[token][msg.sender] >= totalAmountForNextShift,
      "Not enough tokens to shift"
    );

    ILongShort(longShort).shiftPositionNextPrice(
      marketIndex,
      amountSyntheticTokensToShift,
      isShiftFromLong
    );

    userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom[marketIndex][isShiftFromLong][
      msg.sender
    ] = totalAmountForNextShift;

    uint256 userRewardIndex = latestRewardIndex[marketIndex] + 1;
    userNextPrice_stakedActionIndex[marketIndex][msg.sender] = userRewardIndex;

    emit NextPriceStakeShift(
      msg.sender,
      marketIndex,
      amountSyntheticTokensToShift,
      isShiftFromLong,
      userRewardIndex
    );
  }

  /*╔════════════════════════════╗
    ║    WITHDRAWAL & MINTING    ║
    ╚════════════════════════════╝*/

  /**
  @notice Internal logic for withdrawing stakes.
  @dev Mint user any outstanding float before withdrawing.
  @param marketIndex Market index of token.
  @param amount Amount to withdraw.
  @param token Synthetic token that was staked.
  */
  function _withdraw(
    uint32 marketIndex,
    address token,
    uint256 amount
  ) internal virtual gemCollecting(msg.sender) {
    require(amount > 0, "Withdraw amount must be greater than 0");
    uint256 amountFees = (amount * marketUnstakeFee_e18[marketIndex]) / 1e18;

    ISyntheticToken(token).transfer(floatTreasury, amountFees);
    ISyntheticToken(token).transfer(msg.sender, amount - amountFees);

    emit StakeWithdrawn(msg.sender, token, amount);
    emit StakeWithdrawnWithFees(msg.sender, token, amount, amountFees);
  }

  function _withdrawPrepLogic(
    uint32 marketIndex,
    bool isWithdrawFromLong,
    uint256 amount,
    address token
  ) internal {
    ILongShort(longShort).updateSystemState(marketIndex);
    _mintAccumulatedFloatAndExecuteOutstandingActions(marketIndex, msg.sender);

    uint256 currentAmountStaked = userAmountStaked[token][msg.sender];
    // If this value is greater than zero they have pending nextPriceShifts; don't allow user to shit these reserved tokens.
    uint256 amountToShiftForThisToken = userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom[
      marketIndex
    ][isWithdrawFromLong][msg.sender];

    unchecked {
      require(currentAmountStaked >= amount + amountToShiftForThisToken, "not enough to withdraw");
      userAmountStaked[token][msg.sender] = currentAmountStaked - amount;
    }
  }

  /**
  @notice Withdraw function. Allows users to unstake.
  @param amount Amount to withdraw.
  @param marketIndex Market index of staked synthetic token
  @param isWithdrawFromLong is synthetic token to be withdrawn long or short
  */
  function withdraw(
    uint32 marketIndex,
    bool isWithdrawFromLong,
    uint256 amount
  ) external {
    address token = syntheticTokens[marketIndex][isWithdrawFromLong];
    _withdrawPrepLogic(marketIndex, isWithdrawFromLong, amount, token);
    _withdraw(marketIndex, token, amount);
  }

  /**
  @notice Allows users to withdraw their entire stake for a token.
  @param marketIndex Market index of staked synthetic token
  @param isWithdrawFromLong is synthetic token to be withdrawn long or short
  */
  function withdrawAll(uint32 marketIndex, bool isWithdrawFromLong) external {
    ILongShort(longShort).updateSystemState(marketIndex);
    _mintAccumulatedFloatAndExecuteOutstandingActions(marketIndex, msg.sender);

    address token = syntheticTokens[marketIndex][isWithdrawFromLong];

    uint256 userAmountStakedBeforeWithdrawal = userAmountStaked[token][msg.sender];

    uint256 amountToShiftForThisToken = userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom[
      marketIndex
    ][isWithdrawFromLong][msg.sender];
    userAmountStaked[token][msg.sender] = amountToShiftForThisToken;

    _withdraw(marketIndex, token, userAmountStakedBeforeWithdrawal - amountToShiftForThisToken);
  }

  function _hasher(
    uint32 marketIndex,
    bool isWithdrawFromLong,
    address user,
    uint256 withdrawAmount,
    uint256 expiry,
    uint256 nonce,
    uint256 discountWithdrawFee
  ) internal pure returns (bytes32) {
    return
      keccak256(
        abi.encodePacked(
          "\x19Ethereum Signed Message:\n32",
          keccak256(
            abi.encodePacked(
              marketIndex,
              isWithdrawFromLong,
              user,
              withdrawAmount,
              expiry,
              nonce,
              discountWithdrawFee
            )
          )
        )
      );
  }

  function withdrawWithVoucher(
    uint32 marketIndex,
    bool isWithdrawFromLong,
    uint256 withdrawAmount,
    uint256 expiry,
    uint256 nonce,
    uint256 discountWithdrawFee,
    uint8 v,
    bytes32 r,
    bytes32 s
  ) external gemCollecting(msg.sender) {
    // we don't want to allow cross-chain re-use of vouchers - disabling on Ava for now.
    if (!NFT_DISCOUNTS_ENABLED()) {
      revert("DISABLED");
    } else {
      require(withdrawAmount > 0, "Withdraw amount must be greater than 0");
      address discountSigner = ecrecover(
        _hasher(
          marketIndex,
          isWithdrawFromLong,
          msg.sender,
          withdrawAmount,
          expiry,
          nonce,
          discountWithdrawFee
        ),
        v,
        r,
        s
      );
      require(hasRole(DISCOUNT_ROLE, discountSigner), "invalid signature");

      require(block.timestamp < expiry, "coupon expired");
      require(userNonce[msg.sender] == nonce, "invalid nonce");
      require(discountWithdrawFee < marketUnstakeFee_e18[marketIndex], "bad discount fee");
      userNonce[msg.sender] = userNonce[msg.sender] + 1;

      address token = syntheticTokens[marketIndex][isWithdrawFromLong];

      _withdrawPrepLogic(marketIndex, isWithdrawFromLong, withdrawAmount, token);

      uint256 amountFees = (withdrawAmount * discountWithdrawFee) / 1e18;
      ISyntheticToken(token).transfer(floatTreasury, amountFees);
      ISyntheticToken(token).transfer(msg.sender, withdrawAmount - amountFees);
      emit StakeWithdrawn(msg.sender, token, withdrawAmount);
      emit StakeWithdrawnWithFees(msg.sender, token, withdrawAmount, amountFees);
    }
  }

  function safe_getUpdateTimestamp(uint32 marketIndex, uint256 updateIndex)
    external
    view
    override
    returns (uint256 timestamp)
  {
    timestamp = uint256(
      accumulativeFloatPerSyntheticTokenSnapshots[marketIndex][updateIndex].timestamp
    );

    if (HAS_LEGACY_DATA()) {
      if (timestamp == 0) {
        timestamp = accumulativeFloatPerSyntheticTokenSnapshotsLEGACY[marketIndex][updateIndex]
          .timestamp;
      }
    }
  }

  // Upgrade helper:
  function upgradeToUsingCompactStakerState() public {
    // If this function has alreday been called this value will not be 0!
    if (accumulativeFloatPerSyntheticTokenSnapshots[1][0].timestamp == 0) {
      uint256 latestMarket = ILongShort(longShort).latestMarket();
      for (uint32 market = 1; market <= latestMarket; market++) {
        // This value is used to calculate the kvalue multiplier (only important for markets with an active multipiler).
        accumulativeFloatPerSyntheticTokenSnapshots[market][0].timestamp = uint32(
          accumulativeFloatPerSyntheticTokenSnapshotsLEGACY[market][0].timestamp
        );

        uint256 marketLatestUpdateIndex = latestRewardIndex[market];
        accumulativeFloatPerSyntheticTokenSnapshots[market][
          marketLatestUpdateIndex
        ] = AccumulativeIssuancePerStakedSynthSnapshot(
          uint32(
            accumulativeFloatPerSyntheticTokenSnapshotsLEGACY[market][marketLatestUpdateIndex]
              .timestamp
          ),
          SafeCastExtended.toUint112(
            accumulativeFloatPerSyntheticTokenSnapshotsLEGACY[market][marketLatestUpdateIndex]
              .accumulativeFloatPerSyntheticToken_long >>
              ACCUMULATIVE_FLT_ISSUANCE_SAFTEY_BIT_SHIFTING
          ),
          SafeCastExtended.toUint112(
            accumulativeFloatPerSyntheticTokenSnapshotsLEGACY[market][marketLatestUpdateIndex]
              .accumulativeFloatPerSyntheticToken_short >>
              ACCUMULATIVE_FLT_ISSUANCE_SAFTEY_BIT_SHIFTING
          )
        );
      }
    }

    emit Upgrade(1);
  }

  function fixMissSetUserIndexOfLastClaimedReward(uint32 marketIndex, address user) public {
    uint256 currentUserStakedActionIndex = userNextPrice_stakedActionIndex[marketIndex][user];
    if (
      userIndexOfLastClaimedReward[marketIndex][user] > currentUserStakedActionIndex &&
      currentUserStakedActionIndex != 0
    ) {
      userIndexOfLastClaimedReward[marketIndex][user] = currentUserStakedActionIndex;
    }
    /*
    affected users Polygon:
      0xd8e6abf84f42cd280542a150258185be33eacd00
      0x2eb4d3d21ba9666dcf573f55a02b9574840c30f5

    transactions:
      0x234c4b8767dd217cf7c5bf5e4f2ee5b90ab7ccbf7e3320a304b5741182bf315e
      0xf0d6bdcc51fa7f0f64b95e0a8825041d9eb5b9b137c83f3db65d5c733c3211ba
      0xfc22c8ef28cfd6e2262842c44423739dee2b5e91906e8c737f01df968e2658eb
      0x3742f54509c161f1e46affb293bd712e7eb9dd5b2e4fca1f2ece5ef85409efb9
      0x3f48a62f5b219e7ad1b08c0886d14bb3d92b82e8249b4392566335d19c5d3e7c
      0xbc8686a3e90c866bd0802739a144ebdd1e6c45dcf49501090320982e041d264b
      0xf2050f06c59f7e65020f823484c308703038395cd24a52c7f5dde4b5e470be29
      0x7812c16bfd9cdae226978f989e19e7a5b2f860cdcf8638235e988d33a1182efd
      0x0969130dd9b657f95569d1d660be2fc32f0273769eb774489c587ada36c5927a
      0x2c8b8f8f2e86e2ea8736298dbd22f4fc9eb0a8353bad99b4705e07a685d93c08

    no affected users on avalanche.
    */
  }
}
