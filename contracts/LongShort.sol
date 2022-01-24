// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.10;

import "@openzeppelin/contracts/token/ERC20/IERC20.sol";
import "@openzeppelin/contracts/token/ERC20/utils/SafeERC20.sol";
import "@openzeppelin/contracts/utils/math/Math.sol";
import "@openzeppelin/contracts/utils/math/SafeCast.sol";

import "./interfaces/ITokenFactory.sol";
import "./interfaces/ISyntheticToken.sol";
import "./interfaces/IStaker.sol";
import "./interfaces/ILongShort.sol";
import "./interfaces/IYieldManager.sol";
import "./interfaces/IOracleManager.sol";
import "./abstract/AccessControlledAndUpgradeable.sol";
import "./GEMS.sol";

// import "hardhat/console.sol";

/**
 **** visit https://float.capital *****
 */

/// @title Core logic of Float Protocal markets
/// @author float.capital
/// @notice visit https://float.capital for more info
/// @dev All functions in this file are currently `virtual`. This is NOT to encourage inheritance.
/// It is merely for convenince when unit testing.
/// @custom:auditors This contract balances long and short sides.
contract LongShort is ILongShort, AccessControlledAndUpgradeable {
  //Using Open Zeppelin safe transfer library for token transfers
  using SafeERC20 for IERC20;

  /*╔═════════════════════════════╗
    ║          VARIABLES          ║
    ╚═════════════════════════════╝*/

  /* ══════ CONFIG ══════ */
  // NOTE: since these are constants, their use in `if` statements is optimized away by the compiler.
  bool public constant HAS_LEGACY_DATA = true;

  /* ══════ Fixed-precision constants ══════ */
  /// @notice this is the address that permanently locked initial liquidity for markets is held by.
  /// These tokens will never move so market can never have zero liquidity on a side.
  /// @dev f10a7 spells float in hex - for fun - important part is that the private key for this address in not known.
  address public constant PERMANENT_INITIAL_LIQUIDITY_HOLDER =
    0xf10A7_F10A7_f10A7_F10a7_F10A7_f10a7_F10A7_f10a7;

  uint256 public constant SECONDS_IN_A_YEAR_e18 = 315576e20;

  /// @dev an empty allocation of storage for use in future upgrades - inspiration from OZ:
  ///      https://github.com/OpenZeppelin/openzeppelin-contracts-upgradeable/blob/10f0f1a95b1b0fd5520351886bae7a03490f1056/contracts/token/ERC20/ERC20Upgradeable.sol#L361
  uint256[45] private __constantsGap;

  /* ══════ Global state ══════ */
  uint32 public override latestMarket;

  address public staker;
  address public tokenFactory;
  address public gems;

  uint256[45] private __globalStateGap;

  /* ══════ Market specific ══════ */
  mapping(uint32 => bool) public marketExists;

  mapping(uint32 => int256) public override assetPrice;
  mapping(uint32 => uint256) public override marketUpdateIndex;
  mapping(uint32 => uint256) public marketTreasurySplitGradient_e18;
  mapping(uint32 => uint256) public marketLeverage_e18;

  mapping(uint32 => address) public paymentTokens;
  mapping(uint32 => address) public yieldManagers;
  mapping(uint32 => address) public override oracleManagers;

  mapping(uint32 => uint256) public fundingRateMultiplier_e18;

  uint256[44] private __marketStateGap;

  /* ══════ Market + position (long/short) specific ══════ */
  mapping(uint32 => mapping(bool => address)) public override syntheticTokens;

  mapping(uint32 => mapping(bool => uint256)) public marketSideValueInPaymentTokenLEGACY;

  /// @notice synthetic token prices of a given market of a (long/short) at every previous price update
  mapping(uint32 => mapping(bool => mapping(uint256 => uint256)))
    public syntheticToken_priceSnapshotLEGACY;

  mapping(uint32 => mapping(bool => uint256)) public override batched_amountPaymentToken_deposit;
  mapping(uint32 => mapping(bool => uint256)) public override batched_amountSyntheticToken_redeem;
  mapping(uint32 => mapping(bool => uint256))
    public
    override batched_amountSyntheticToken_toShiftAwayFrom_marketSide;

  struct MarketSideValueInPaymentToken {
    // this has a maximum size of `2^128=3.4028237e+38` units of payment token which is amply sufficient for our markets
    uint128 value_long;
    uint128 value_short;
  }
  mapping(uint32 => MarketSideValueInPaymentToken) public override marketSideValueInPaymentToken;

  struct SynthPriceInPaymentToken {
    // this has a maximum size of `2^128=3.4028237e+38` units of payment token which is amply sufficient for our markets
    uint128 price_long;
    uint128 price_short;
  }
  mapping(uint32 => mapping(uint256 => SynthPriceInPaymentToken))
    public syntheticToken_priceSnapshot;

  uint256[43] private __marketPositonStateGap;

  /* ══════ User specific ══════ */
  mapping(uint32 => mapping(address => uint256)) public userNextPrice_currentUpdateIndex;

  mapping(uint32 => mapping(bool => mapping(address => uint256)))
    public userNextPrice_paymentToken_depositAmount;
  mapping(uint32 => mapping(bool => mapping(address => uint256)))
    public userNextPrice_syntheticToken_redeemAmount;
  mapping(uint32 => mapping(bool => mapping(address => uint256)))
    public userNextPrice_syntheticToken_toShiftAwayFrom_marketSide;

  /*╔═════════════════════════════╗
    ║          MODIFIERS          ║
    ╚═════════════════════════════╝*/

  // This is used for testing (as opposed to onlyRole)
  function adminOnlyModifierLogic() internal virtual {
    _checkRole(ADMIN_ROLE, msg.sender);
  }

  modifier adminOnly() {
    adminOnlyModifierLogic();
    _;
  }

  function requireMarketExistsModifierLogic(uint32 marketIndex) internal view virtual {
    require(marketExists[marketIndex], "market doesn't exist");
  }

  modifier requireMarketExists(uint32 marketIndex) {
    requireMarketExistsModifierLogic(marketIndex);
    _;
  }

  modifier updateSystemStateMarketAndExecuteOutstandingNextPriceSettlements(
    address user,
    uint32 marketIndex
  ) {
    _updateSystemStateInternal(marketIndex);
    _executeOutstandingNextPriceSettlements(user, marketIndex);
    _;
  }

  function gemCollectingModifierLogic() internal virtual {
    if (msg.sender != staker) {
      GEMS(gems).gm(msg.sender);
    }
  }

  modifier gemCollecting() {
    gemCollectingModifierLogic();
    _;
  }

  /*╔═════════════════════════════╗
    ║       CONTRACT SET-UP       ║
    ╚═════════════════════════════╝*/

  /// @notice Initializes the contract.
  /// @dev Calls OpenZeppelin's initializer modifier.
  /// @param _admin Address of the admin role.
  /// @param _tokenFactory Address of the contract which creates synthetic asset tokens.
  /// @param _staker Address of the contract which handles synthetic asset stakes.
  function initialize(
    address _admin,
    address _tokenFactory,
    address _staker,
    address _gems
  ) public virtual {
    require(
      _admin != address(0) &&
        _tokenFactory != address(0) &&
        _staker != address(0) &&
        _gems != address(0)
    );
    // The below function ensures that this contract can't be re-initialized!
    _AccessControlledAndUpgradeable_init(_admin);
    tokenFactory = _tokenFactory;
    staker = _staker;
    gems = _gems;

    emit LongShortV1(_admin, _tokenFactory, _staker);
  }

  /*╔═══════════════════╗
    ║       ADMIN       ║
    ╚═══════════════════╝*/

  /// @notice Update oracle for a market
  /// @dev Can only be called by the current admin.
  /// @param marketIndex An uint32 which uniquely identifies a market.
  /// @param _newOracleManager Address of the replacement oracle manager.
  function updateMarketOracle(uint32 marketIndex, address _newOracleManager) external adminOnly {
    // If not a oracle contract this would break things.. Test's arn't validating this
    // Ie require isOracle interface - ERC165
    address previousOracleManager = oracleManagers[marketIndex];
    oracleManagers[marketIndex] = _newOracleManager;
    emit OracleUpdated(marketIndex, previousOracleManager, _newOracleManager);
  }

  /// @notice changes the gradient of the line for determining the yield split between market and treasury.
  function changeMarketTreasurySplitGradient(
    uint32 marketIndex,
    uint256 _marketTreasurySplitGradient_e18
  ) external adminOnly {
    marketTreasurySplitGradient_e18[marketIndex] = _marketTreasurySplitGradient_e18;
  }

  function changeMarketFundingRateMultiplier(uint32 marketIndex, uint256 _fundingRateMultiplier_e18)
    external
    adminOnly
  {
    require(_fundingRateMultiplier_e18 <= 5e19, "not in range: funding rate <= 5000%");
    fundingRateMultiplier_e18[marketIndex] = _fundingRateMultiplier_e18;
    emit MarketFundingRateMultiplerChanged(marketIndex, _fundingRateMultiplier_e18);
  }

  /*╔═════════════════════════════╗
    ║       MARKET CREATION       ║
    ╚═════════════════════════════╝*/

  /// @notice Creates an entirely new long/short market tracking an underlying oracle price.
  ///  Make sure the synthetic names/symbols are unique.
  /// @dev This does not make the market active.
  /// The `initializeMarket` function was split out separately to this function to reduce costs.
  /// @param syntheticName Name of the synthetic asset
  /// @param syntheticSymbol Symbol for the synthetic asset
  /// @param _paymentToken The address of the erc20 token used to buy this synthetic asset
  /// this will likely always be DAI
  /// @param _oracleManager The address of the oracle manager that provides the price feed for this market
  /// @param _yieldManager The contract that manages depositing the paymentToken into a yield bearing protocol
  function createNewSyntheticMarket(
    string calldata syntheticName,
    string calldata syntheticSymbol,
    address _paymentToken,
    address _oracleManager,
    address _yieldManager
  ) external adminOnly {
    require(
      _paymentToken != address(0) && _oracleManager != address(0) && _yieldManager != address(0)
    );

    uint32 marketIndex = ++latestMarket;
    address _staker = staker;

    // Ensure new markets don't use the same yield manager
    IYieldManager(_yieldManager).initializeForMarket();

    // Create new synthetic long token.
    syntheticTokens[marketIndex][true] = ITokenFactory(tokenFactory).createSyntheticToken(
      string(abi.encodePacked("Float Long ", syntheticName)),
      string(abi.encodePacked("fl", syntheticSymbol)),
      _staker,
      marketIndex,
      true
    );

    // Create new synthetic short token.
    syntheticTokens[marketIndex][false] = ITokenFactory(tokenFactory).createSyntheticToken(
      string(abi.encodePacked("Float Short ", syntheticName)),
      string(abi.encodePacked("fs", syntheticSymbol)),
      _staker,
      marketIndex,
      false
    );

    // Initial market state.
    paymentTokens[marketIndex] = _paymentToken;
    yieldManagers[marketIndex] = _yieldManager;
    oracleManagers[marketIndex] = _oracleManager;
    assetPrice[marketIndex] = IOracleManager(oracleManagers[marketIndex]).updatePrice();

    emit SyntheticMarketCreated(
      marketIndex,
      syntheticTokens[marketIndex][true],
      syntheticTokens[marketIndex][false],
      _paymentToken,
      assetPrice[marketIndex],
      syntheticName,
      syntheticSymbol,
      _oracleManager,
      _yieldManager
    );
  }

  /// @notice Creates an entirely new long/short market tracking an underlying oracle price.
  ///  Uses already created synthetic tokens.
  /// @dev This does not make the market active.
  /// The `initializeMarket` function was split out separately to this function to reduce costs.
  /// @param syntheticName Name of the synthetic asset
  /// @param syntheticSymbol Symbol for the synthetic asset
  /// @param _longToken Address for the long token.
  /// @param _shortToken Address for the short token.
  /// @param _paymentToken The address of the erc20 token used to buy this synthetic asset
  /// this will likely always be DAI
  /// @param _oracleManager The address of the oracle manager that provides the price feed for this market
  /// @param _yieldManager The contract that manages depositing the paymentToken into a yield bearing protocol
  function createNewSyntheticMarketExternalSyntheticTokens(
    string calldata syntheticName,
    string calldata syntheticSymbol,
    address _longToken,
    address _shortToken,
    address _paymentToken,
    address _oracleManager,
    address _yieldManager
  ) external adminOnly {
    uint32 marketIndex = ++latestMarket;

    // Ensure new markets don't use the same yield manager
    IYieldManager(_yieldManager).initializeForMarket();

    // Assign new synthetic long token.
    syntheticTokens[marketIndex][true] = _longToken;

    // Assign new synthetic short token.
    syntheticTokens[marketIndex][false] = _shortToken;

    // Initial market state.
    paymentTokens[marketIndex] = _paymentToken;
    yieldManagers[marketIndex] = _yieldManager;
    oracleManagers[marketIndex] = _oracleManager;
    assetPrice[marketIndex] = IOracleManager(oracleManagers[marketIndex]).updatePrice();

    emit SyntheticMarketCreated(
      marketIndex,
      _longToken,
      _shortToken,
      _paymentToken,
      assetPrice[marketIndex],
      syntheticName,
      syntheticSymbol,
      _oracleManager,
      _yieldManager
    );
  }

  /// @notice Seeds a new market with initial capital.
  /// @dev Only called when initializing a market.
  /// @param initialMarketSeedForEachMarketSide Amount in wei for which to seed both sides of the market.
  /// @param marketIndex An uint32 which uniquely identifies a market.
  function _seedMarketInitially(uint256 initialMarketSeedForEachMarketSide, uint32 marketIndex)
    internal
    virtual
  {
    require(
      // You require at least 1e18 (1 payment token with 18 decimal places) of the underlying payment token to seed the market.
      initialMarketSeedForEachMarketSide >= 1e18,
      "Insufficient market seed"
    );

    uint256 amountToLockInYieldManager = initialMarketSeedForEachMarketSide * 2;
    _transferPaymentTokensFromUserToYieldManager(marketIndex, amountToLockInYieldManager);
    IYieldManager(yieldManagers[marketIndex]).depositPaymentToken(amountToLockInYieldManager);

    ISyntheticToken(syntheticTokens[marketIndex][true]).mint(
      PERMANENT_INITIAL_LIQUIDITY_HOLDER,
      initialMarketSeedForEachMarketSide
    );
    ISyntheticToken(syntheticTokens[marketIndex][false]).mint(
      PERMANENT_INITIAL_LIQUIDITY_HOLDER,
      initialMarketSeedForEachMarketSide
    );

    marketSideValueInPaymentToken[marketIndex] = MarketSideValueInPaymentToken(
      SafeCast.toUint128(initialMarketSeedForEachMarketSide),
      SafeCast.toUint128(initialMarketSeedForEachMarketSide)
    );
  }

  /// @notice Sets a market as active once it has already been setup by createNewSyntheticMarket.
  /// @dev Seperated from createNewSyntheticMarket due to gas considerations.
  /// @param marketIndex An int32 which uniquely identifies the market.
  /// @param kInitialMultiplier Linearly decreasing multiplier for Float token issuance for the market when staking synths.
  /// @param kPeriod Time which kInitialMultiplier will last
  /// @param unstakeFee_e18 Base 1e18 percentage fee levied when unstaking for the market.
  /// @param balanceIncentiveCurve_exponent Sets the degree to which Float token issuance differs
  /// for market sides in unbalanced markets. See Staker.sol
  /// @param balanceIncentiveCurve_equilibriumOffset An offset to account for naturally imbalanced markets
  /// when Float token issuance should differ for market sides. See Staker.sol
  /// @param initialMarketSeedForEachMarketSide Amount of payment token that will be deposited in each market side to seed the market.
  function initializeMarket(
    uint32 marketIndex,
    uint256 kInitialMultiplier,
    uint256 kPeriod,
    uint256 unstakeFee_e18,
    uint256 initialMarketSeedForEachMarketSide,
    uint256 balanceIncentiveCurve_exponent,
    int256 balanceIncentiveCurve_equilibriumOffset,
    uint256 _marketTreasurySplitGradient_e18,
    uint256 marketLeverage
  ) external adminOnly {
    require(
      kInitialMultiplier != 0 &&
        unstakeFee_e18 != 0 &&
        initialMarketSeedForEachMarketSide != 0 &&
        balanceIncentiveCurve_exponent != 0 &&
        _marketTreasurySplitGradient_e18 != 0
    );

    require(!marketExists[marketIndex], "already initialized");
    require(marketIndex <= latestMarket, "index too high");

    marketExists[marketIndex] = true;

    marketTreasurySplitGradient_e18[marketIndex] = _marketTreasurySplitGradient_e18;

    // Set this value to one initially - 0 is a null value and thus potentially bug prone.
    marketUpdateIndex[marketIndex] = 1;

    _seedMarketInitially(initialMarketSeedForEachMarketSide, marketIndex);

    require(marketLeverage <= 50e18 && marketLeverage >= 1e17, "Incorrect leverage");
    marketLeverage_e18[marketIndex] = marketLeverage;

    // Add new staker funds with fresh synthetic tokens.
    IStaker(staker).addNewStakingFund(
      marketIndex,
      syntheticTokens[marketIndex][true],
      syntheticTokens[marketIndex][false],
      kInitialMultiplier,
      kPeriod,
      unstakeFee_e18,
      balanceIncentiveCurve_exponent,
      balanceIncentiveCurve_equilibriumOffset
    );

    IStaker(staker).pushUpdatedMarketPricesToUpdateFloatIssuanceCalculations(
      marketIndex,
      1,
      1e18,
      1e18,
      initialMarketSeedForEachMarketSide,
      initialMarketSeedForEachMarketSide
    );

    emit NewMarketLaunchedAndSeeded(
      marketIndex,
      initialMarketSeedForEachMarketSide,
      marketLeverage
    );
  }

  /*╔══════════════════════════════╗
    ║       GETTER FUNCTIONS       ║
    ╚══════════════════════════════╝*/

  /// @notice Calculates the conversion rate from synthetic tokens to payment tokens.
  /// @dev Synth tokens have a fixed 18 decimals.
  /// @param amountPaymentTokenBackingSynth Amount of payment tokens in that token's lowest denomination.
  /// @param amountSyntheticToken Amount of synth token in wei.
  /// @return syntheticTokenPrice The calculated conversion rate in base 1e18.
  function _getSyntheticTokenPrice(
    uint256 amountPaymentTokenBackingSynth,
    uint256 amountSyntheticToken
  ) internal pure virtual returns (uint256 syntheticTokenPrice) {
    return (amountPaymentTokenBackingSynth * 1e18) / amountSyntheticToken;
  }

  /// @notice Converts synth token amounts to payment token amounts at a synth token price.
  /// @dev Price assumed base 1e18.
  /// @param amountSyntheticToken Amount of synth token in wei.
  /// @param syntheticTokenPriceInPaymentTokens The conversion rate from synth to payment tokens in base 1e18.
  /// @return amountPaymentToken The calculated amount of payment tokens in token's lowest denomination.
  function _getAmountPaymentToken(
    uint256 amountSyntheticToken,
    uint256 syntheticTokenPriceInPaymentTokens
  ) internal pure virtual returns (uint256 amountPaymentToken) {
    return (amountSyntheticToken * syntheticTokenPriceInPaymentTokens) / 1e18;
  }

  /// @notice Converts payment token amounts to synth token amounts at a synth token price.
  /// @dev  Price assumed base 1e18.
  /// @param amountPaymentTokenBackingSynth Amount of payment tokens in that token's lowest denomination.
  /// @param syntheticTokenPriceInPaymentTokens The conversion rate from synth to payment tokens in base 1e18.
  /// @return amountSyntheticToken The calculated amount of synthetic token in wei.
  function _getAmountSyntheticToken(
    uint256 amountPaymentTokenBackingSynth,
    uint256 syntheticTokenPriceInPaymentTokens
  ) internal pure virtual returns (uint256 amountSyntheticToken) {
    return (amountPaymentTokenBackingSynth * 1e18) / syntheticTokenPriceInPaymentTokens;
  }

  /**
  @notice Calculate the amount of target side synthetic tokens that are worth the same
          amount of payment tokens as X many synthetic tokens on origin side.
          The resulting equation comes from simplifying this function

            _getAmountSyntheticToken(
              _getAmountPaymentToken(
                amountOriginSynth,
                priceOriginSynth
              ),
              priceTargetSynth)

            Unpacking the function we get:
            ((amountOriginSynth * priceOriginSynth) / 1e18) * 1e18 / priceTargetSynth
              And simplifying this we get:
            (amountOriginSynth * priceOriginSynth) / priceTargetSynth
  @param amountSyntheticTokens_originSide Amount of synthetic tokens on origin side
  @param syntheticTokenPrice_originSide Price of origin side's synthetic token
  @param syntheticTokenPrice_targetSide Price of target side's synthetic token
  @return equivalentAmountSyntheticTokensOnTargetSide Amount of synthetic token on target side
  */
  function _getEquivalentAmountSyntheticTokensOnTargetSide(
    uint256 amountSyntheticTokens_originSide,
    uint256 syntheticTokenPrice_originSide,
    uint256 syntheticTokenPrice_targetSide
  ) internal pure virtual returns (uint256 equivalentAmountSyntheticTokensOnTargetSide) {
    equivalentAmountSyntheticTokensOnTargetSide =
      (amountSyntheticTokens_originSide * syntheticTokenPrice_originSide) /
      syntheticTokenPrice_targetSide;
  }

  function get_syntheticToken_priceSnapshot(uint32 marketIndex, uint256 priceSnapshotIndex)
    public
    view
    override
    returns (uint256 priceLong, uint256 priceShort)
  {
    priceLong = uint256(syntheticToken_priceSnapshot[marketIndex][priceSnapshotIndex].price_long);

    priceShort = uint256(syntheticToken_priceSnapshot[marketIndex][priceSnapshotIndex].price_short);

    if (HAS_LEGACY_DATA) {
      // In case price requested is part of legacy data-structure
      if (
        priceLong == 0 /* which means priceShort is also zero! */
      ) {
        priceLong = syntheticToken_priceSnapshotLEGACY[marketIndex][true][priceSnapshotIndex];
        priceShort = syntheticToken_priceSnapshotLEGACY[marketIndex][false][priceSnapshotIndex];
      }
    }
  }

  function get_syntheticToken_priceSnapshot_side(
    uint32 marketIndex,
    bool isLong,
    uint256 priceSnapshotIndex
  ) public view override returns (uint256 price) {
    if (isLong) {
      price = uint256(syntheticToken_priceSnapshot[marketIndex][priceSnapshotIndex].price_long);
    } else {
      price = uint256(syntheticToken_priceSnapshot[marketIndex][priceSnapshotIndex].price_short);
    }
    if (HAS_LEGACY_DATA) {
      // In case price requested is part of legacy data-structure
      if (price == 0) {
        price = syntheticToken_priceSnapshotLEGACY[marketIndex][isLong][priceSnapshotIndex];
      }
    }
  }

  /// @notice Given an executed next price shift from tokens on one market side to the other,
  /// determines how many other side tokens the shift was worth.
  /// @dev Intended for use primarily by Staker.sol
  /// @param marketIndex An uint32 which uniquely identifies a market.
  /// @param amountSyntheticToken_redeemOnOriginSide Amount of synth token in wei.
  /// @param isShiftFromLong Whether the token shift is from long to short (true), or short to long (false).
  /// @param priceSnapshotIndex Index which identifies which synth prices to use.
  /// @return amountSyntheticTokensToMintOnTargetSide The amount in wei of tokens for the other side that the shift was worth.
  function getAmountSyntheticTokenToMintOnTargetSide(
    uint32 marketIndex,
    uint256 amountSyntheticToken_redeemOnOriginSide,
    bool isShiftFromLong,
    uint256 priceSnapshotIndex
  ) public view virtual override returns (uint256 amountSyntheticTokensToMintOnTargetSide) {
    uint256 syntheticTokenPriceOnOriginSide;
    uint256 syntheticTokenPriceOnTargetSide;
    if (isShiftFromLong) {
      (
        syntheticTokenPriceOnOriginSide,
        syntheticTokenPriceOnTargetSide
      ) = get_syntheticToken_priceSnapshot(marketIndex, priceSnapshotIndex);
    } else {
      (
        syntheticTokenPriceOnTargetSide,
        syntheticTokenPriceOnOriginSide
      ) = get_syntheticToken_priceSnapshot(marketIndex, priceSnapshotIndex);
    }

    amountSyntheticTokensToMintOnTargetSide = _getEquivalentAmountSyntheticTokensOnTargetSide(
      amountSyntheticToken_redeemOnOriginSide,
      syntheticTokenPriceOnOriginSide,
      syntheticTokenPriceOnTargetSide
    );
  }

  /**
  @notice The amount of a synth token a user is owed following a batch execution.
    4 possible states for next price actions:
        - "Pending" - means the next price update hasn't happened or been enacted on by the updateSystemState function.
        - "Confirmed" - means the next price has been updated by the updateSystemState function. There is still
        -               outstanding (lazy) computation that needs to be executed per user in the batch.
        - "Settled" - there is no more computation left for the user.
        - "Non-existent" - user has no next price actions.
    This function returns a calculated value only in the case of 'confirmed' next price actions.
    It should return zero for all other types of next price actions.
  @dev Used in SyntheticToken.sol balanceOf to allow for automatic reflection of next price actions.
  @param user The address of the user for whom to execute the function for.
  @param marketIndex An uint32 which uniquely identifies a market.
  @param isLong Whether it is for the long synthetic asset or the short synthetic asset.
  @return confirmedButNotSettledBalance The amount in wei of tokens that the user is owed.
  */
  function getUsersConfirmedButNotSettledSynthBalance(
    address user,
    uint32 marketIndex,
    bool isLong
  )
    external
    view
    virtual
    override
    requireMarketExists(marketIndex)
    returns (uint256 confirmedButNotSettledBalance)
  {
    uint256 currentMarketUpdateIndex = marketUpdateIndex[marketIndex];
    uint256 userNextPrice_currentUpdateIndex_forMarket = userNextPrice_currentUpdateIndex[
      marketIndex
    ][user];
    if (
      userNextPrice_currentUpdateIndex_forMarket != 0 &&
      userNextPrice_currentUpdateIndex_forMarket <= currentMarketUpdateIndex
    ) {
      uint256 amountPaymentTokenDeposited = userNextPrice_paymentToken_depositAmount[marketIndex][
        isLong
      ][user];

      uint256 syntheticTokenPrice;
      uint256 syntheticTokenPriceOnOriginSideOfShift;

      if (isLong) {
        (
          syntheticTokenPrice,
          syntheticTokenPriceOnOriginSideOfShift
        ) = get_syntheticToken_priceSnapshot(
          marketIndex,
          userNextPrice_currentUpdateIndex_forMarket
        );
      } else {
        (
          syntheticTokenPriceOnOriginSideOfShift,
          syntheticTokenPrice
        ) = get_syntheticToken_priceSnapshot(
          marketIndex,
          userNextPrice_currentUpdateIndex_forMarket
        );
      }

      if (amountPaymentTokenDeposited > 0) {
        confirmedButNotSettledBalance = _getAmountSyntheticToken(
          amountPaymentTokenDeposited,
          syntheticTokenPrice
        );
      }

      uint256 amountSyntheticTokensToBeShiftedAwayFromOriginSide = userNextPrice_syntheticToken_toShiftAwayFrom_marketSide[
          marketIndex
        ][!isLong][user];

      if (amountSyntheticTokensToBeShiftedAwayFromOriginSide > 0) {
        confirmedButNotSettledBalance += _getEquivalentAmountSyntheticTokensOnTargetSide(
          amountSyntheticTokensToBeShiftedAwayFromOriginSide,
          syntheticTokenPriceOnOriginSideOfShift,
          syntheticTokenPrice
        );
      }
    }
  }

  /**
   @notice Calculates the percentage in base 1e18 of how much of the accrued yield
   for a market should be allocated to treasury.
   @dev For gas considerations also returns whether the long side is imbalanced.
   @dev For gas considerations totalValueLockedInMarket is passed as a parameter as the function
   calling this function has pre calculated the value
   @param longValue The current total payment token value of the long side of the market.
   @param shortValue The current total payment token value of the short side of the market.
   @param totalValueLockedInMarket Total payment token value of both sides of the market.
   @return isLongSideUnderbalanced Whether the long side initially had less value than the short side.
   @return treasuryYieldPercent_e18 The percentage in base 1e18 of how much of the accrued yield
   for a market should be allocated to treasury.
   */
  function _getYieldSplit(
    uint32 marketIndex,
    uint256 longValue,
    uint256 shortValue,
    uint256 totalValueLockedInMarket
  ) internal view virtual returns (bool isLongSideUnderbalanced, uint256 treasuryYieldPercent_e18) {
    isLongSideUnderbalanced = longValue < shortValue;
    uint256 imbalance;

    unchecked {
      if (isLongSideUnderbalanced) {
        imbalance = shortValue - longValue;
      } else {
        imbalance = longValue - shortValue;
      }
    }

    // marketTreasurySplitGradient_e18 may be adjusted to ensure yield is given
    // to the market at a desired rate e.g. if a market tends to become imbalanced
    // frequently then the gradient can be increased to funnel yield to the market
    // quicker.
    // See this equation in latex: https://ipfs.io/ipfs/QmXsW4cHtxpJ5BFwRcMSUw7s5G11Qkte13NTEfPLTKEx4x
    // Interact with this equation: https://www.desmos.com/calculator/pnl43tfv5b
    uint256 marketPercentCalculated_e18 = (imbalance *
      marketTreasurySplitGradient_e18[marketIndex]) / totalValueLockedInMarket;

    uint256 marketPercent_e18 = Math.min(marketPercentCalculated_e18, 1e18);

    unchecked {
      treasuryYieldPercent_e18 = 1e18 - marketPercent_e18;
    }
  }

  /*╔══════════════════════════════╗
    ║       HELPER FUNCTIONS       ║
    ╚══════════════════════════════╝*/

  /// @notice This calculates the value transfer from the overbalanced to underbalanced side (i.e. the funding rate)
  /// This is a further incentive measure to balanced markets. This may be present on some and not other synthetic markets.
  /// @param marketIndex The market for which to execute the function for.
  /// @param _fundingRateMultiplier_e18 A scalar base e18 for the funding rate.
  /// @param overbalancedValue Side with more liquidity.
  /// @param underbalancedValue Side with less liquidity.
  /// @return fundingAmount The amount the overbalanced side needs to pay the underbalanced.
  function _calculateFundingAmount(
    uint32 marketIndex,
    uint256 _fundingRateMultiplier_e18,
    uint256 overbalancedValue,
    uint256 underbalancedValue
  ) internal virtual returns (uint256 fundingAmount) {
    uint256 lastUpdateTimestamp = IStaker(staker).safe_getUpdateTimestamp(
      marketIndex,
      marketUpdateIndex[marketIndex]
    );

    /* 
    overBalanced * (1 - underBalanced/overBalanced)
      = overBalanced * (overBalanced - underBalanced)/overBalanced)
      = overBalanced - underBalanced
      = market imbalance
    
    funding amount = market imbalance * yearlyMaxFundingRate * (now - lastUpdate) / (365.25days in seconds base e18)
    */
    fundingAmount =
      ((overbalancedValue - underbalancedValue) *
        _fundingRateMultiplier_e18 *
        (block.timestamp - lastUpdateTimestamp)) /
      SECONDS_IN_A_YEAR_e18;
  }

  /// @notice First gets yield from the yield manager and allocates it to market and treasury.
  /// It then allocates the full market yield portion to the underbalanced side of the market.
  /// NB this function also adjusts the value of the long and short side based on the latest
  /// price of the underlying asset received from the oracle. This function should ideally be
  /// called everytime there is an price update from the oracle. We have built a bot that does this.
  /// The system is still perectly safe if not called every price update, the synthetic will just
  /// less closely track the underlying asset.
  /// @dev In one function as yield should be allocated before rebalancing.
  /// This prevents an attack whereby the user imbalances a side to capture all accrued yield.
  /// @param marketIndex The market for which to execute the function for.
  /// @param newAssetPrice The new asset price.
  /// @return longValue The value of the long side after rebalancing.
  /// @return shortValue The value of the short side after rebalancing.
  function _claimAndDistributeYieldThenRebalanceMarket(uint32 marketIndex, int256 newAssetPrice)
    internal
    virtual
    returns (uint256 longValue, uint256 shortValue)
  {
    MarketSideValueInPaymentToken
      storage currentMarketSideValueInPaymentToken = marketSideValueInPaymentToken[marketIndex];
    // Claiming and distributing the yield
    longValue = currentMarketSideValueInPaymentToken.value_long;
    shortValue = currentMarketSideValueInPaymentToken.value_short;
    uint256 totalValueLockedInMarket = longValue + shortValue;

    (bool isLongSideUnderbalanced, uint256 treasuryYieldPercent_e18) = _getYieldSplit(
      marketIndex,
      longValue,
      shortValue,
      totalValueLockedInMarket
    );

    uint256 marketAmount = IYieldManager(yieldManagers[marketIndex])
      .distributeYieldForTreasuryAndReturnMarketAllocation(
        totalValueLockedInMarket,
        treasuryYieldPercent_e18
      );

    // Take fee as simply 1% of notional over 1 year.
    // Value leaving long and short to treasury, this should be done carefully in yield manager!
    // order of where this is done is important.
    // This amount also potentially lumped together with funding rate fee + exposure fee ?
    // See what is simple and makes sense on the bottom line.

    if (marketAmount > 0) {
      if (isLongSideUnderbalanced) {
        longValue += marketAmount;
      } else {
        shortValue += marketAmount;
      }
    }

    int256 oldAssetPrice = assetPrice[marketIndex];

    // Adjusting value of long and short pool based on price movement
    // The side/position with less liquidity has 100% percent exposure to the price movement.
    // The side/position with more liquidity will have exposure < 100% to the price movement.
    // I.e. Imagine $100 in longValue and $50 shortValue
    // long side would have $50/$100 = 50% exposure to price movements based on the liquidity imbalance.
    // min(longValue, shortValue) = $50 , therefore if the price change was -10% then
    // $50 * 10% = $5 gained for short side and conversely $5 lost for long side.
    int256 underbalancedSideValue = int256(Math.min(longValue, shortValue));

    // send a piece of value change to the treasury?
    // Again this reduces the value of totalValueLockedInMarket which means yield manager needs to be alerted.
    // See this equation in latex: https://ipfs.io/ipfs/QmPeJ3SZdn1GfxqCD4GDYyWTJGPMSHkjPJaxrzk2qTTPSE
    // Interact with this equation: https://www.desmos.com/calculator/t8gr6j5vsq
    int256 valueChange = ((newAssetPrice - oldAssetPrice) *
      underbalancedSideValue *
      int256(marketLeverage_e18[marketIndex])) / (oldAssetPrice * 1e18);

    uint256 fundingRateMultiplier = fundingRateMultiplier_e18[marketIndex];
    if (fundingRateMultiplier > 0) {
      //  slow drip interest funding payment here.
      //  recheck yield hasn't tipped the market.
      if (longValue < shortValue) {
        valueChange += int256(
          _calculateFundingAmount(marketIndex, fundingRateMultiplier, shortValue, longValue)
        );
      } else {
        valueChange -= int256(
          _calculateFundingAmount(marketIndex, fundingRateMultiplier, longValue, shortValue)
        );
      }
    }

    if (valueChange < 0) {
      valueChange = -valueChange; // make value change positive

      // handle 'impossible' edge case where underlying price feed changes more than 100% downwards gracefully.
      if (uint256(valueChange) > longValue) {
        valueChange = (int256(longValue) * 99999) / 100000;
      }
      longValue -= uint256(valueChange);
      shortValue += uint256(valueChange);
    } else {
      // handle 'impossible' edge case where underlying price feed changes more than 100% upwards gracefully.
      if (uint256(valueChange) > shortValue) {
        valueChange = (int256(shortValue) * 99999) / 100000;
      }
      longValue += uint256(valueChange);
      shortValue -= uint256(valueChange);
    }
  }

  /*╔═══════════════════════════════╗
    ║     UPDATING SYSTEM STATE     ║
    ╚═══════════════════════════════╝*/

  /// @notice Updates the value of the long and short sides to account for latest oracle price updates
  /// and batches all next price actions.
  /// @dev To prevent front-running only executes on price change from an oracle.
  /// We assume the function will be called for each market at least once per price update.
  /// Note Even if not called on every price update, this won't affect security, it will only affect how closely
  /// the synthetic asset actually tracks the underlying asset.
  /// @param marketIndex The market index for which to update.
  function _updateSystemStateInternal(uint32 marketIndex)
    internal
    virtual
    requireMarketExists(marketIndex)
  {
    // If a negative int is return this should fail.
    int256 newAssetPrice = IOracleManager(oracleManagers[marketIndex]).updatePrice();

    uint256 currentMarketIndex = marketUpdateIndex[marketIndex];

    bool assetPriceHasChanged = assetPrice[marketIndex] != newAssetPrice;

    if (assetPriceHasChanged) {
      (
        uint256 newLongPoolValue,
        uint256 newShortPoolValue
      ) = _claimAndDistributeYieldThenRebalanceMarket(marketIndex, newAssetPrice);

      uint256 syntheticTokenPrice_inPaymentTokens_long = _getSyntheticTokenPrice(
        newLongPoolValue,
        ISyntheticToken(syntheticTokens[marketIndex][true]).totalSupply()
      );
      uint256 syntheticTokenPrice_inPaymentTokens_short = _getSyntheticTokenPrice(
        newShortPoolValue,
        ISyntheticToken(syntheticTokens[marketIndex][false]).totalSupply()
      );

      assetPrice[marketIndex] = newAssetPrice;

      currentMarketIndex++;
      marketUpdateIndex[marketIndex] = currentMarketIndex;

      syntheticToken_priceSnapshot[marketIndex][currentMarketIndex] = SynthPriceInPaymentToken(
        SafeCast.toUint128(syntheticTokenPrice_inPaymentTokens_long),
        SafeCast.toUint128(syntheticTokenPrice_inPaymentTokens_short)
      );

      (
        int256 long_changeInMarketValue_inPaymentToken,
        int256 short_changeInMarketValue_inPaymentToken
      ) = _batchConfirmOutstandingPendingActions(
          marketIndex,
          syntheticTokenPrice_inPaymentTokens_long,
          syntheticTokenPrice_inPaymentTokens_short
        );

      newLongPoolValue = uint256(
        int256(newLongPoolValue) + long_changeInMarketValue_inPaymentToken
      );
      newShortPoolValue = uint256(
        int256(newShortPoolValue) + short_changeInMarketValue_inPaymentToken
      );
      marketSideValueInPaymentToken[marketIndex] = MarketSideValueInPaymentToken(
        SafeCast.toUint128(newLongPoolValue),
        SafeCast.toUint128(newShortPoolValue)
      );

      IStaker(staker).pushUpdatedMarketPricesToUpdateFloatIssuanceCalculations(
        marketIndex,
        currentMarketIndex,
        syntheticTokenPrice_inPaymentTokens_long,
        syntheticTokenPrice_inPaymentTokens_short,
        newLongPoolValue,
        newShortPoolValue
      );

      emit SystemStateUpdated(
        marketIndex,
        currentMarketIndex,
        newAssetPrice,
        newLongPoolValue,
        newShortPoolValue,
        syntheticTokenPrice_inPaymentTokens_long,
        syntheticTokenPrice_inPaymentTokens_short
      );
    }
  }

  /// @notice Updates the state of a market to account for the latest oracle price update.
  /// @param marketIndex An uint32 which uniquely identifies a market.
  function updateSystemState(uint32 marketIndex) external override {
    _updateSystemStateInternal(marketIndex);
  }

  /// @notice Updates the state of multiples markets to account for their latest oracle price updates.
  /// @param marketIndexes An array of int32s which uniquely identify markets.
  function updateSystemStateMulti(uint32[] calldata marketIndexes) external override {
    uint256 length = marketIndexes.length;
    for (uint256 i = 0; i < length; i++) {
      _updateSystemStateInternal(marketIndexes[i]);
    }
  }

  /*╔═══════════════════════════╗
    ║          DEPOSIT          ║
    ╚═══════════════════════════╝*/

  /// @notice Transfers payment tokens for a market from msg.sender to this contract.
  /// @dev Tokens are transferred directly to this contract to be deposited by the yield manager in the batch to earn yield.
  ///      Since we check the return value of the transferFrom method, all payment tokens we use must conform to the ERC20 standard.
  /// @param marketIndex An uint32 which uniquely identifies a market.
  /// @param amount Amount of payment tokens in that token's lowest denominationto deposit.
  function _transferPaymentTokensFromUserToYieldManager(uint32 marketIndex, uint256 amount)
    internal
    virtual
  {
    IERC20(paymentTokens[marketIndex]).safeTransferFrom(
      msg.sender,
      yieldManagers[marketIndex],
      amount
    );
  }

  /*╔═══════════════════════════╗
    ║       MINT POSITION       ║
    ╚═══════════════════════════╝*/

  /// @notice Allows users to mint synthetic assets for a market. To prevent front-running these mints are executed on the next price update from the oracle.
  /// @dev Called by external functions to mint either long or short. If a user mints multiple times before a price update, these are treated as a single mint.
  /// @param marketIndex An uint32 which uniquely identifies a market.
  /// @param amount Amount of payment tokens in that token's lowest denominationfor which to mint synthetic assets at next price.
  /// @param isLong Whether the mint is for a long or short synth.
  function _mintNextPrice(
    uint32 marketIndex,
    uint256 amount,
    bool isLong
  )
    internal
    virtual
    updateSystemStateMarketAndExecuteOutstandingNextPriceSettlements(msg.sender, marketIndex)
    gemCollecting
  {
    require(amount > 0, "Mint amount == 0");
    _transferPaymentTokensFromUserToYieldManager(marketIndex, amount);

    batched_amountPaymentToken_deposit[marketIndex][isLong] += amount;
    userNextPrice_paymentToken_depositAmount[marketIndex][isLong][msg.sender] += amount;
    uint256 nextUpdateIndex = marketUpdateIndex[marketIndex] + 1;
    userNextPrice_currentUpdateIndex[marketIndex][msg.sender] = nextUpdateIndex;

    emit NextPriceDeposit(marketIndex, isLong, amount, msg.sender, nextUpdateIndex);
  }

  function mintAndStakeNextPrice(
    uint32 marketIndex,
    uint256 amount,
    bool isLong
  )
    external
    virtual
    override
    updateSystemStateMarketAndExecuteOutstandingNextPriceSettlements(staker, marketIndex)
    gemCollecting
  {
    require(amount > 0, "Mint amount must be greater than 0");
    _transferPaymentTokensFromUserToYieldManager(marketIndex, amount);

    batched_amountPaymentToken_deposit[marketIndex][isLong] += amount;
    userNextPrice_paymentToken_depositAmount[marketIndex][isLong][staker] += amount;
    uint256 nextUpdateIndex = marketUpdateIndex[marketIndex] + 1;
    userNextPrice_currentUpdateIndex[marketIndex][staker] = nextUpdateIndex;

    IStaker(staker).mintAndStakeNextPrice(marketIndex, amount, isLong, msg.sender);

    emit NextPriceDepositAndStake(marketIndex, isLong, amount, msg.sender, nextUpdateIndex);
  }

  /// @notice Allows users to mint long synthetic assets for a market. To prevent front-running these mints are executed on the next price update from the oracle.
  /// @param marketIndex An uint32 which uniquely identifies a market.
  /// @param amount Amount of payment tokens in that token's lowest denominationfor which to mint synthetic assets at next price.
  function mintLongNextPrice(uint32 marketIndex, uint256 amount) external override {
    _mintNextPrice(marketIndex, amount, true);
  }

  /// @notice Allows users to mint short synthetic assets for a market. To prevent front-running these mints are executed on the next price update from the oracle.
  /// @param marketIndex An uint32 which uniquely identifies a market.
  /// @param amount Amount of payment tokens in that token's lowest denominationfor which to mint synthetic assets at next price.
  function mintShortNextPrice(uint32 marketIndex, uint256 amount) external override {
    _mintNextPrice(marketIndex, amount, false);
  }

  /*╔═══════════════════════════╗
    ║      REDEEM POSITION      ║
    ╚═══════════════════════════╝*/

  /// @notice Allows users to redeem their synthetic tokens for payment tokens. To prevent front-running these redeems are executed on the next price update from the oracle.
  /// @dev Called by external functions to redeem either long or short. Payment tokens are actually transferred to the user when executeOutstandingNextPriceSettlements is called from a function call by the user.
  /// @param marketIndex An uint32 which uniquely identifies a market.
  /// @param tokens_redeem Amount in wei of synth tokens to redeem.
  /// @param isLong Whether this redeem is for a long or short synth.
  function _redeemNextPrice(
    uint32 marketIndex,
    uint256 tokens_redeem,
    bool isLong
  )
    internal
    virtual
    updateSystemStateMarketAndExecuteOutstandingNextPriceSettlements(msg.sender, marketIndex)
    gemCollecting
  {
    require(tokens_redeem > 0, "Redeem amount == 0");
    ISyntheticToken(syntheticTokens[marketIndex][isLong]).transferFrom(
      msg.sender,
      address(this),
      tokens_redeem
    );

    userNextPrice_syntheticToken_redeemAmount[marketIndex][isLong][msg.sender] += tokens_redeem;
    uint256 nextUpdateIndex = marketUpdateIndex[marketIndex] + 1;
    userNextPrice_currentUpdateIndex[marketIndex][msg.sender] = nextUpdateIndex;

    batched_amountSyntheticToken_redeem[marketIndex][isLong] += tokens_redeem;

    emit NextPriceRedeem(marketIndex, isLong, tokens_redeem, msg.sender, nextUpdateIndex);
  }

  /// @notice  Allows users to redeem long synthetic assets for a market. To prevent front-running these redeems are executed on the next price update from the oracle.
  /// @param marketIndex An uint32 which uniquely identifies a market.
  /// @param tokens_redeem Amount in wei of synth tokens to redeem at the next oracle price.
  function redeemLongNextPrice(uint32 marketIndex, uint256 tokens_redeem) external override {
    _redeemNextPrice(marketIndex, tokens_redeem, true);
  }

  /// @notice  Allows users to redeem short synthetic assets for a market. To prevent front-running these redeems are executed on the next price update from the oracle.
  /// @param marketIndex An uint32 which uniquely identifies a market.
  /// @param tokens_redeem Amount in wei of synth tokens to redeem at the next oracle price.
  function redeemShortNextPrice(uint32 marketIndex, uint256 tokens_redeem) external override {
    _redeemNextPrice(marketIndex, tokens_redeem, false);
  }

  /*╔═══════════════════════════╗
    ║       SHIFT POSITION      ║
    ╚═══════════════════════════╝*/

  /// @notice  Allows users to shift their position from one side of the market to the other in a single transaction. To prevent front-running these shifts are executed on the next price update from the oracle.
  /// @dev Called by external functions to shift either way. Intended for primary use by Staker.sol
  /// @param marketIndex An uint32 which uniquely identifies a market.
  /// @param amountSyntheticTokensToShift Amount in wei of synthetic tokens to shift from the one side to the other at the next oracle price update.
  /// @param isShiftFromLong Whether the token shift is from long to short (true), or short to long (false).
  function shiftPositionNextPrice(
    uint32 marketIndex,
    uint256 amountSyntheticTokensToShift,
    bool isShiftFromLong
  )
    public
    virtual
    override
    updateSystemStateMarketAndExecuteOutstandingNextPriceSettlements(msg.sender, marketIndex)
    gemCollecting
  {
    require(amountSyntheticTokensToShift > 0, "Shift amount == 0");
    ISyntheticToken(syntheticTokens[marketIndex][isShiftFromLong]).transferFrom(
      msg.sender,
      address(this),
      amountSyntheticTokensToShift
    );

    userNextPrice_syntheticToken_toShiftAwayFrom_marketSide[marketIndex][isShiftFromLong][
      msg.sender
    ] += amountSyntheticTokensToShift;
    uint256 nextUpdateIndex = marketUpdateIndex[marketIndex] + 1;
    userNextPrice_currentUpdateIndex[marketIndex][msg.sender] = nextUpdateIndex;

    batched_amountSyntheticToken_toShiftAwayFrom_marketSide[marketIndex][
      isShiftFromLong
    ] += amountSyntheticTokensToShift;

    emit NextPriceSyntheticPositionShift(
      marketIndex,
      isShiftFromLong,
      amountSyntheticTokensToShift,
      msg.sender,
      nextUpdateIndex
    );
  }

  /// @notice Allows users to shift their position from long to short in a single transaction. To prevent front-running these shifts are executed on the next price update from the oracle.
  /// @param marketIndex An uint32 which uniquely identifies a market.
  /// @param amountSyntheticTokensToShift Amount in wei of synthetic tokens to shift from long to short the next oracle price update.
  function shiftPositionFromLongNextPrice(uint32 marketIndex, uint256 amountSyntheticTokensToShift)
    external
    override
  {
    shiftPositionNextPrice(marketIndex, amountSyntheticTokensToShift, true);
  }

  /// @notice Allows users to shift their position from short to long in a single transaction. To prevent front-running these shifts are executed on the next price update from the oracle.
  /// @param marketIndex An uint32 which uniquely identifies a market.
  /// @param amountSyntheticTokensToShift Amount in wei of synthetic tokens to shift from the short to long at the next oracle price update.
  function shiftPositionFromShortNextPrice(uint32 marketIndex, uint256 amountSyntheticTokensToShift)
    external
    override
  {
    shiftPositionNextPrice(marketIndex, amountSyntheticTokensToShift, false);
  }

  /*╔════════════════════════════════╗
    ║     NEXT PRICE SETTLEMENTS     ║
    ╚════════════════════════════════╝*/

  /// @notice Transfers outstanding synth tokens from a next price mint to the user.
  /// @dev The outstanding synths should already be reflected for the user due to balanceOf in SyntheticToken.sol, this just does the accounting.
  /// @param marketIndex An uint32 which uniquely identifies a market.
  /// @param user The address of the user for whom to execute the function for.
  /// @param isLong Whether this is for the long or short synth for the market.
  function _executeOutstandingNextPriceMints(
    uint32 marketIndex,
    address user,
    bool isLong
  ) internal virtual {
    uint256 currentPaymentTokenDepositAmount = userNextPrice_paymentToken_depositAmount[
      marketIndex
    ][isLong][user];
    if (currentPaymentTokenDepositAmount > 0) {
      userNextPrice_paymentToken_depositAmount[marketIndex][isLong][user] = 0;
      uint256 amountSyntheticTokensToTransferToUser = _getAmountSyntheticToken(
        currentPaymentTokenDepositAmount,
        get_syntheticToken_priceSnapshot_side(
          marketIndex,
          isLong,
          userNextPrice_currentUpdateIndex[marketIndex][user]
        )
      );
      ISyntheticToken(syntheticTokens[marketIndex][isLong]).transfer(
        user,
        amountSyntheticTokensToTransferToUser
      );
    }
  }

  /// @notice Transfers outstanding payment tokens from a next price redemption to the user.
  /// @param marketIndex An uint32 which uniquely identifies a market.
  /// @param user The address of the user for whom to execute the function for.
  /// @param isLong Whether this is for the long or short synth for the market.
  function _executeOutstandingNextPriceRedeems(
    uint32 marketIndex,
    address user,
    bool isLong
  ) internal virtual {
    uint256 currentSyntheticTokenRedemptions = userNextPrice_syntheticToken_redeemAmount[
      marketIndex
    ][isLong][user];
    if (currentSyntheticTokenRedemptions > 0) {
      userNextPrice_syntheticToken_redeemAmount[marketIndex][isLong][user] = 0;
      uint256 amountPaymentToken_toRedeem = _getAmountPaymentToken(
        currentSyntheticTokenRedemptions,
        get_syntheticToken_priceSnapshot_side(
          marketIndex,
          isLong,
          userNextPrice_currentUpdateIndex[marketIndex][user]
        )
      );

      IYieldManager(yieldManagers[marketIndex]).transferPaymentTokensToUser(
        user,
        amountPaymentToken_toRedeem
      );
    }
  }

  /// @notice Transfers outstanding synth tokens from a next price position shift to the user.
  /// @dev The outstanding synths should already be reflected for the user due to balanceOf in SyntheticToken.sol, this just does the accounting.
  /// @param marketIndex An uint32 which uniquely identifies a market.
  /// @param user The address of the user for whom to execute the function for.
  /// @param isShiftFromLong Whether the token shift was from long to short (true), or short to long (false).
  function _executeOutstandingNextPriceTokenShifts(
    uint32 marketIndex,
    address user,
    bool isShiftFromLong
  ) internal virtual {
    uint256 syntheticToken_toShiftAwayFrom_marketSide = userNextPrice_syntheticToken_toShiftAwayFrom_marketSide[
        marketIndex
      ][isShiftFromLong][user];
    if (syntheticToken_toShiftAwayFrom_marketSide > 0) {
      uint256 syntheticToken_toShiftTowardsTargetSide = getAmountSyntheticTokenToMintOnTargetSide(
        marketIndex,
        syntheticToken_toShiftAwayFrom_marketSide,
        isShiftFromLong,
        userNextPrice_currentUpdateIndex[marketIndex][user]
      );

      userNextPrice_syntheticToken_toShiftAwayFrom_marketSide[marketIndex][isShiftFromLong][
        user
      ] = 0;

      require(
        ISyntheticToken(syntheticTokens[marketIndex][!isShiftFromLong]).transfer(
          user,
          syntheticToken_toShiftTowardsTargetSide
        )
      );
    }
  }

  /// @notice After markets have been batched updated on a new oracle price, transfers any owed tokens to a user from their next price actions for that update to that user.
  /// @dev Once the market has updated for the next price, should be guaranteed (through modifiers) to execute for a user before user initiation of new next price actions.
  /// @param user The address of the user for whom to execute the function.
  /// @param marketIndex An uint32 which uniquely identifies a market.
  function _executeOutstandingNextPriceSettlements(address user, uint32 marketIndex)
    internal
    virtual
  {
    uint256 userCurrentUpdateIndex = userNextPrice_currentUpdateIndex[marketIndex][user];
    if (userCurrentUpdateIndex != 0 && userCurrentUpdateIndex <= marketUpdateIndex[marketIndex]) {
      //
      /*
        Hello!
      */
      //
      _executeOutstandingNextPriceMints(marketIndex, user, true);
      _executeOutstandingNextPriceMints(marketIndex, user, false);
      _executeOutstandingNextPriceRedeems(marketIndex, user, true);
      _executeOutstandingNextPriceRedeems(marketIndex, user, false);
      _executeOutstandingNextPriceTokenShifts(marketIndex, user, true);
      _executeOutstandingNextPriceTokenShifts(marketIndex, user, false);

      userNextPrice_currentUpdateIndex[marketIndex][user] = 0;

      emit ExecuteNextPriceSettlementsUser(user, marketIndex);
    }
  }

  /// @notice After markets have been batched updated on a new oracle price, transfers any owed tokens to a user from their next price actions for that update to that user.
  /// @param user The address of the user for whom to execute the function.
  /// @param marketIndex An uint32 which uniquely identifies a market.
  function executeOutstandingNextPriceSettlementsUser(address user, uint32 marketIndex)
    external
    override
  {
    _executeOutstandingNextPriceSettlements(user, marketIndex);
  }

  /// @notice Executes outstanding next price settlements for a user for multiple markets.
  /// @param user The address of the user for whom to execute the function.
  /// @param marketIndexes An array of int32s which each uniquely identify a market.
  function executeOutstandingNextPriceSettlementsUserMulti(
    address user,
    uint32[] calldata marketIndexes
  ) external {
    uint256 length = marketIndexes.length;
    for (uint256 i = 0; i < length; i++) {
      _executeOutstandingNextPriceSettlements(user, marketIndexes[i]);
    }
  }

  /*╔═══════════════════════════════════════════╗
    ║   BATCHED NEXT PRICE SETTLEMENT ACTIONS   ║
    ╚═══════════════════════════════════════════╝*/

  /// @notice Either transfers funds from the yield manager to this contract if redeems > deposits,
  /// and vice versa. The yield manager handles depositing and withdrawing the funds from a yield market.
  /// @dev When all batched next price actions are handled the total value in the market can either increase or decrease based on the value of mints and redeems.
  /// @param marketIndex An uint32 which uniquely identifies a market.
  /// @param totalPaymentTokenValueChangeForMarket An int256 which indicates the magnitude and direction of the change in market value.
  function _handleTotalPaymentTokenValueChangeForMarketWithYieldManager(
    uint32 marketIndex,
    int256 totalPaymentTokenValueChangeForMarket
  ) internal virtual {
    if (totalPaymentTokenValueChangeForMarket > 0) {
      IYieldManager(yieldManagers[marketIndex]).depositPaymentToken(
        uint256(totalPaymentTokenValueChangeForMarket)
      );
    } else if (totalPaymentTokenValueChangeForMarket < 0) {
      // NB there will be issues here if not enough liquidity exists to withdraw
      // Boolean should be returned from yield manager and think how to appropriately handle this
      IYieldManager(yieldManagers[marketIndex]).removePaymentTokenFromMarket(
        uint256(-totalPaymentTokenValueChangeForMarket)
      );
    }
  }

  /// @notice Given a desired change in synth token supply, either mints or burns tokens to achieve that desired change.
  /// @dev When all batched next price actions are executed total supply for a synth can either increase or decrease.
  /// @param marketIndex An uint32 which uniquely identifies a market.
  /// @param isLong Whether this function should execute for the long or short synth for the market.
  /// @param changeInSyntheticTokensTotalSupply The amount in wei by which synth token supply should change.
  function _handleChangeInSyntheticTokensTotalSupply(
    uint32 marketIndex,
    bool isLong,
    int256 changeInSyntheticTokensTotalSupply
  ) internal virtual {
    if (changeInSyntheticTokensTotalSupply > 0) {
      ISyntheticToken(syntheticTokens[marketIndex][isLong]).mint(
        address(this),
        uint256(changeInSyntheticTokensTotalSupply)
      );
    } else if (changeInSyntheticTokensTotalSupply < 0) {
      ISyntheticToken(syntheticTokens[marketIndex][isLong]).burn(
        uint256(-changeInSyntheticTokensTotalSupply)
      );
    }
  }

  /**
  @notice Performs all batched next price actions on an oracle price update.
  @dev Mints or burns all synthetic tokens for this contract.

    After this function is executed all user actions in that batch are confirmed and can be settled individually by
      calling _executeOutstandingNexPriceSettlements for a given user.

    The maths here is safe from rounding errors since it always over estimates on the batch with division.
      (as an example (5/3) + (5/3) = 2 but (5+5)/3 = 10/3 = 3, so the batched action would mint one more)
  @param marketIndex An uint32 which uniquely identifies a market.
  @param syntheticTokenPrice_inPaymentTokens_long The long synthetic token price for this oracle price update.
  @param syntheticTokenPrice_inPaymentTokens_short The short synthetic token price for this oracle price update.
  @return long_changeInMarketValue_inPaymentToken The total value change for the long side after all batched actions are executed.
  @return short_changeInMarketValue_inPaymentToken The total value change for the short side after all batched actions are executed.
  */
  function _batchConfirmOutstandingPendingActions(
    uint32 marketIndex,
    uint256 syntheticTokenPrice_inPaymentTokens_long,
    uint256 syntheticTokenPrice_inPaymentTokens_short
  )
    internal
    virtual
    returns (
      int256 long_changeInMarketValue_inPaymentToken,
      int256 short_changeInMarketValue_inPaymentToken
    )
  {
    int256 changeInSupply_syntheticToken_long;
    int256 changeInSupply_syntheticToken_short;

    // NOTE: the only reason we are reusing amountForCurrentAction_workingVariable for all actions (redeemLong, redeemShort, mintLong, mintShort, shiftFromLong, shiftFromShort) is to reduce stack usage
    uint256 amountForCurrentAction_workingVariable = batched_amountPaymentToken_deposit[
      marketIndex
    ][true];

    // Handle batched deposits LONG
    if (amountForCurrentAction_workingVariable > 0) {
      long_changeInMarketValue_inPaymentToken = int256(amountForCurrentAction_workingVariable);

      batched_amountPaymentToken_deposit[marketIndex][true] = 0;

      changeInSupply_syntheticToken_long = int256(
        _getAmountSyntheticToken(
          amountForCurrentAction_workingVariable,
          syntheticTokenPrice_inPaymentTokens_long
        )
      );
    }

    // Handle batched deposits SHORT
    amountForCurrentAction_workingVariable = batched_amountPaymentToken_deposit[marketIndex][false];
    if (amountForCurrentAction_workingVariable > 0) {
      short_changeInMarketValue_inPaymentToken = int256(amountForCurrentAction_workingVariable);

      batched_amountPaymentToken_deposit[marketIndex][false] = 0;

      changeInSupply_syntheticToken_short = int256(
        _getAmountSyntheticToken(
          amountForCurrentAction_workingVariable,
          syntheticTokenPrice_inPaymentTokens_short
        )
      );
    }

    // Handle shift tokens from LONG to SHORT
    amountForCurrentAction_workingVariable = batched_amountSyntheticToken_toShiftAwayFrom_marketSide[
      marketIndex
    ][true];

    if (amountForCurrentAction_workingVariable > 0) {
      int256 paymentTokenValueChangeForShiftToShort = int256(
        _getAmountPaymentToken(
          amountForCurrentAction_workingVariable,
          syntheticTokenPrice_inPaymentTokens_long
        )
      );

      long_changeInMarketValue_inPaymentToken -= paymentTokenValueChangeForShiftToShort;
      short_changeInMarketValue_inPaymentToken += paymentTokenValueChangeForShiftToShort;

      changeInSupply_syntheticToken_long -= int256(amountForCurrentAction_workingVariable);
      changeInSupply_syntheticToken_short += int256(
        _getEquivalentAmountSyntheticTokensOnTargetSide(
          amountForCurrentAction_workingVariable,
          syntheticTokenPrice_inPaymentTokens_long,
          syntheticTokenPrice_inPaymentTokens_short
        )
      );

      batched_amountSyntheticToken_toShiftAwayFrom_marketSide[marketIndex][true] = 0;
    }

    // Handle shift tokens from SHORT to LONG
    amountForCurrentAction_workingVariable = batched_amountSyntheticToken_toShiftAwayFrom_marketSide[
      marketIndex
    ][false];
    if (amountForCurrentAction_workingVariable > 0) {
      int256 paymentTokenValueChangeForShiftToLong = int256(
        _getAmountPaymentToken(
          amountForCurrentAction_workingVariable,
          syntheticTokenPrice_inPaymentTokens_short
        )
      );

      short_changeInMarketValue_inPaymentToken -= paymentTokenValueChangeForShiftToLong;
      long_changeInMarketValue_inPaymentToken += paymentTokenValueChangeForShiftToLong;

      changeInSupply_syntheticToken_short -= int256(amountForCurrentAction_workingVariable);
      changeInSupply_syntheticToken_long += int256(
        _getEquivalentAmountSyntheticTokensOnTargetSide(
          amountForCurrentAction_workingVariable,
          syntheticTokenPrice_inPaymentTokens_short,
          syntheticTokenPrice_inPaymentTokens_long
        )
      );

      batched_amountSyntheticToken_toShiftAwayFrom_marketSide[marketIndex][false] = 0;
    }

    // Handle batched redeems LONG
    amountForCurrentAction_workingVariable = batched_amountSyntheticToken_redeem[marketIndex][true];
    if (amountForCurrentAction_workingVariable > 0) {
      long_changeInMarketValue_inPaymentToken -= int256(
        _getAmountPaymentToken(
          amountForCurrentAction_workingVariable,
          syntheticTokenPrice_inPaymentTokens_long
        )
      );
      changeInSupply_syntheticToken_long -= int256(amountForCurrentAction_workingVariable);

      batched_amountSyntheticToken_redeem[marketIndex][true] = 0;
    }

    // Handle batched redeems SHORT
    amountForCurrentAction_workingVariable = batched_amountSyntheticToken_redeem[marketIndex][
      false
    ];
    if (amountForCurrentAction_workingVariable > 0) {
      short_changeInMarketValue_inPaymentToken -= int256(
        _getAmountPaymentToken(
          amountForCurrentAction_workingVariable,
          syntheticTokenPrice_inPaymentTokens_short
        )
      );
      changeInSupply_syntheticToken_short -= int256(amountForCurrentAction_workingVariable);

      batched_amountSyntheticToken_redeem[marketIndex][false] = 0;
    }

    // Batch settle payment tokens
    _handleTotalPaymentTokenValueChangeForMarketWithYieldManager(
      marketIndex,
      long_changeInMarketValue_inPaymentToken + short_changeInMarketValue_inPaymentToken
    );
    // Batch settle synthetic tokens
    _handleChangeInSyntheticTokensTotalSupply(
      marketIndex,
      true,
      changeInSupply_syntheticToken_long
    );
    _handleChangeInSyntheticTokensTotalSupply(
      marketIndex,
      false,
      changeInSupply_syntheticToken_short
    );
  }

  // Upgrade helper:
  function upgradeToUsingCompactValueAndPriceSnapshots() public {
    // If this function has alreday been called this value will not be 0!
    if (marketSideValueInPaymentToken[1].value_long == 0) {
      for (uint32 market = 1; market <= latestMarket; market++) {
        marketSideValueInPaymentToken[market] = MarketSideValueInPaymentToken(
          SafeCast.toUint128(marketSideValueInPaymentTokenLEGACY[market][true]),
          SafeCast.toUint128(marketSideValueInPaymentTokenLEGACY[market][false])
        );
      }
    }
    emit Upgrade(1);
  }
}
