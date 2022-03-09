// SPDX-License-Identifier: AGPL-3.0
pragma solidity 0.8.10;

import {IPoolAddressesProvider} from "./IPoolAddressesProvider.sol";
import {DataTypes} from "./DataTypes.sol";

/**
 * @title IPool
 * @author Aave
 * @notice Defines the basic interface for an Aave Pool - Aave V3.
 **/
interface IPool {
  // event MintUnbacked(
  //   address indexed reserve,
  //   address user,
  //   address indexed onBehalfOf,
  //   uint256 amount,
  //   uint16 indexed referralCode
  // );

  // event BackUnbacked(address indexed reserve, address indexed backer, uint256 amount, uint256 fee);

  // event Supply(
  //   address indexed reserve,
  //   address user,
  //   address indexed onBehalfOf,
  //   uint256 amount,
  //   uint16 indexed referralCode
  // );

  // event Withdraw(address indexed reserve, address indexed user, address indexed to, uint256 amount);

  // event Borrow(
  //   address indexed reserve,
  //   address user,
  //   address indexed onBehalfOf,
  //   uint256 amount,
  //   DataTypes.InterestRateMode interestRateMode,
  //   uint256 borrowRate,
  //   uint16 indexed referralCode
  // );

  // event Repay(
  //   address indexed reserve,
  //   address indexed user,
  //   address indexed repayer,
  //   uint256 amount,
  //   bool useATokens
  // );

  // event SwapBorrowRateMode(
  //   address indexed reserve,
  //   address indexed user,
  //   DataTypes.InterestRateMode interestRateMode
  // );

  // event IsolationModeTotalDebtUpdated(address indexed asset, uint256 totalDebt);

  // event UserEModeSet(address indexed user, uint8 categoryId);

  // event ReserveUsedAsCollateralEnabled(address indexed reserve, address indexed user);

  // event ReserveUsedAsCollateralDisabled(address indexed reserve, address indexed user);

  // event RebalanceStableBorrowRate(address indexed reserve, address indexed user);

  // event FlashLoan(
  //   address indexed target,
  //   address initiator,
  //   address indexed asset,
  //   uint256 amount,
  //   DataTypes.InterestRateMode interestRateMode,
  //   uint256 premium,
  //   uint16 indexed referralCode
  // );

  // event LiquidationCall(
  //   address indexed collateralAsset,
  //   address indexed debtAsset,
  //   address indexed user,
  //   uint256 debtToCover,
  //   uint256 liquidatedCollateralAmount,
  //   address liquidator,
  //   bool receiveAToken
  // );

  // event ReserveDataUpdated(
  //   address indexed reserve,
  //   uint256 liquidityRate,
  //   uint256 stableBorrowRate,
  //   uint256 variableBorrowRate,
  //   uint256 liquidityIndex,
  //   uint256 variableBorrowIndex
  // );

  // event MintedToTreasury(address indexed reserve, uint256 amountMinted);

  // function mintUnbacked(
  //   address asset,
  //   uint256 amount,
  //   address onBehalfOf,
  //   uint16 referralCode
  // ) external;

  // function backUnbacked(
  //   address asset,
  //   uint256 amount,
  //   uint256 fee
  // ) external;

  // function supply(
  //   address asset,
  //   uint256 amount,
  //   address onBehalfOf,
  //   uint16 referralCode
  // ) external;

  // function supplyWithPermit(
  //   address asset,
  //   uint256 amount,
  //   address onBehalfOf,
  //   uint16 referralCode,
  //   uint256 deadline,
  //   uint8 permitV,
  //   bytes32 permitR,
  //   bytes32 permitS
  // ) external;

  /**
   * @notice Withdraws an `amount` of underlying asset from the reserve, burning the equivalent aTokens owned
   * E.g. User has 100 aUSDC, calls withdraw() and receives 100 USDC, burning the 100 aUSDC
   * @param asset The address of the underlying asset to withdraw
   * @param amount The underlying amount to be withdrawn
   *   - Send the value type(uint256).max in order to withdraw the whole aToken balance
   * @param to The address that will receive the underlying, same as msg.sender if the user
   *   wants to receive it on his own wallet, or a different address if the beneficiary is a
   *   different wallet
   * @return The final amount withdrawn
   **/
  function withdraw(
    address asset,
    uint256 amount,
    address to
  ) external returns (uint256);

  // function borrow(
  //   address asset,
  //   uint256 amount,
  //   uint256 interestRateMode,
  //   uint16 referralCode,
  //   address onBehalfOf
  // ) external;

  // function repay(
  //   address asset,
  //   uint256 amount,
  //   uint256 interestRateMode,
  //   address onBehalfOf
  // ) external returns (uint256);

  // function repayWithPermit(
  //   address asset,
  //   uint256 amount,
  //   uint256 interestRateMode,
  //   address onBehalfOf,
  //   uint256 deadline,
  //   uint8 permitV,
  //   bytes32 permitR,
  //   bytes32 permitS
  // ) external returns (uint256);

  // function repayWithATokens(
  //   address asset,
  //   uint256 amount,
  //   uint256 interestRateMode
  // ) external returns (uint256);

  // function swapBorrowRateMode(address asset, uint256 interestRateMode) external;

  // function rebalanceStableBorrowRate(address asset, address user) external;

  // function setUserUseReserveAsCollateral(address asset, bool useAsCollateral) external;

  // function liquidationCall(
  //   address collateralAsset,
  //   address debtAsset,
  //   address user,
  //   uint256 debtToCover,
  //   bool receiveAToken
  // ) external;

  // function flashLoan(
  //   address receiverAddress,
  //   address[] calldata assets,
  //   uint256[] calldata amounts,
  //   uint256[] calldata interestRateModes,
  //   address onBehalfOf,
  //   bytes calldata params,
  //   uint16 referralCode
  // ) external;

  // function flashLoanSimple(
  //   address receiverAddress,
  //   address asset,
  //   uint256 amount,
  //   bytes calldata params,
  //   uint16 referralCode
  // ) external;

  // function getUserAccountData(address user)
  //   external
  //   view
  //   returns (
  //     uint256 totalCollateralBase,
  //     uint256 totalDebtBase,
  //     uint256 availableBorrowsBase,
  //     uint256 currentLiquidationThreshold,
  //     uint256 ltv,
  //     uint256 healthFactor
  //   );

  // function initReserve(
  //   address asset,
  //   address aTokenAddress,
  //   address stableDebtAddress,
  //   address variableDebtAddress,
  //   address interestRateStrategyAddress
  // ) external;

  // function dropReserve(address asset) external;

  // function setReserveInterestRateStrategyAddress(address asset, address rateStrategyAddress)
  //   external;

  // function setConfiguration(address asset, DataTypes.ReserveConfigurationMap calldata configuration)
  //   external;

  // function getConfiguration(address asset)
  //   external
  //   view
  //   returns (DataTypes.ReserveConfigurationMap memory);

  // function getUserConfiguration(address user)
  //   external
  //   view
  //   returns (DataTypes.UserConfigurationMap memory);

  // function getReserveNormalizedIncome(address asset) external view returns (uint256);

  // function getReserveNormalizedVariableDebt(address asset) external view returns (uint256);

  // function getReserveData(address asset) external view returns (DataTypes.ReserveData memory);

  // function finalizeTransfer(
  //   address asset,
  //   address from,
  //   address to,
  //   uint256 amount,
  //   uint256 balanceFromBefore,
  //   uint256 balanceToBefore
  // ) external;

  // function getReservesList() external view returns (address[] memory);

  // function ADDRESSES_PROVIDER() external view returns (IPoolAddressesProvider);

  // function updateBridgeProtocolFee(uint256 bridgeProtocolFee) external;

  // function updateFlashloanPremiums(
  //   uint128 flashLoanPremiumTotal,
  //   uint128 flashLoanPremiumToProtocol
  // ) external;

  //function configureEModeCategory(uint8 id, DataTypes.EModeCategory memory config) external;

  //function getEModeCategoryData(uint8 id) external view returns (DataTypes.EModeCategory memory);

  // function setUserEMode(uint8 categoryId) external;

  // function getUserEMode(address user) external view returns (uint256);

  // function resetIsolationModeTotalDebt(address asset) external;

  // function MAX_STABLE_RATE_BORROW_SIZE_PERCENT() external view returns (uint256);

  // function FLASHLOAN_PREMIUM_TOTAL() external view returns (uint128);

  // function BRIDGE_PROTOCOL_FEE() external view returns (uint256);

  // function FLASHLOAN_PREMIUM_TO_PROTOCOL() external view returns (uint128);

  // function MAX_NUMBER_RESERVES() external view returns (uint16);

  // function mintToTreasury(address[] calldata assets) external;

  // function rescueTokens(
  //   address token,
  //   address to,
  //   uint256 amount
  // ) external;

  /**
   * @notice Supplies an `amount` of underlying asset into the reserve, receiving in return overlying aTokens.
   * - E.g. User supplies 100 USDC and gets in return 100 aUSDC
   * @dev Deprecated: Use the `supply` function instead
   * @param asset The address of the underlying asset to supply
   * @param amount The amount to be supplied
   * @param onBehalfOf The address that will receive the aTokens, same as msg.sender if the user
   *   wants to receive them on his own wallet, or a different address if the beneficiary of aTokens
   *   is a different wallet
   * @param referralCode Code used to register the integrator originating the operation, for potential rewards.
   *   0 if the action is executed directly by the user, without any middle-man
   **/
  function deposit(
    address asset,
    uint256 amount,
    address onBehalfOf,
    uint16 referralCode
  ) external;
}
