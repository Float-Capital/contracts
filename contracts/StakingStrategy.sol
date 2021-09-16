// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.3;

import "@openzeppelin/contracts-upgradeable/proxy/utils/Initializable.sol";
import "@openzeppelin/contracts-upgradeable/proxy/utils/UUPSUpgradeable.sol";
import "@openzeppelin/contracts-upgradeable/access/AccessControlUpgradeable.sol";

import "./interfaces/ILongShort.sol";
import "./interfaces/ISyntheticToken.sol";
import "./interfaces/IStaker.sol";
import "./StrategyToken.sol";

/** @title ILO Contract */
contract StakingStrategy is Initializable, UUPSUpgradeable, AccessControlUpgradeable {
  bytes32 public constant UPGRADER_ROLE = keccak256("UPGRADER_ROLE");

  address public longShort;
  address public staker;
  StrategyToken public strategyToken;

  /*╔═════════════════════════════╗
    ║       CONTRACT SETUP        ║
    ╚═════════════════════════════╝*/

  function initialize(
    string calldata name,
    string calldata symbol,
    address _longShort,
    address _staker
  ) external initializer {
    __AccessControl_init();
    __UUPSUpgradeable_init();

    longShort = _longShort;
    strategyToken = new StrategyToken(name, symbol);
    staker = _staker;

    _setupRole(UPGRADER_ROLE, msg.sender);
    grantRole(DEFAULT_ADMIN_ROLE, msg.sender);
  }

  function _authorizeUpgrade(address newImplementation) internal override onlyRole(UPGRADER_ROLE) {}

  /*╔═════════════════════════════╗
    ║           DEPOSIT           ║
    ╚═════════════════════════════╝*/
  function depositLongAndShortTokens(
    uint32 marketIndex,
    uint256 amountToken,
    bool isLong
  ) public {
    ILongShort _longShort = ILongShort(longShort);
    address longTokenAddress = _longShort.syntheticTokens(marketIndex, true);
    address shortTokenAddress = _longShort.syntheticTokens(marketIndex, false);

    //TODO
    //can maybe add this method to longShort contract?
    (uint256 longTokenPrice, uint256 shortTokenPrice) = _getLongAndShortTokenPrice(marketIndex);

    (uint256 longShortRatio, uint256 beforeBalanceOfContract) = _getContractStakedBalanceAndRatio(
      marketIndex,
      longTokenAddress,
      shortTokenAddress,
      longTokenPrice,
      shortTokenPrice
    );
    (uint256 _amountStakedLong, uint256 _amountStakedShort) = _getTotalLongAndShortTokensStaked(
      marketIndex,
      longTokenAddress,
      shortTokenAddress
    );
    uint256 amountOfTokensToMatchRatio;

    if (isLong) {
      amountOfTokensToMatchRatio = amountToken / (longShortRatio);
    } else {
      amountOfTokensToMatchRatio = amountToken * longShortRatio;
    }

    //TODO
    //two transfers - can optimise
    ISyntheticToken(_longShort.syntheticTokens(marketIndex, isLong)).transferFrom(
      msg.sender,
      address(this),
      amountToken
    );
    ISyntheticToken(_longShort.syntheticTokens(marketIndex, !isLong)).transferFrom(
      msg.sender,
      address(this),
      amountOfTokensToMatchRatio
    );

    ISyntheticToken(_longShort.syntheticTokens(marketIndex, isLong)).stake(amountToken);
    ISyntheticToken(_longShort.syntheticTokens(marketIndex, !isLong)).stake(
      amountOfTokensToMatchRatio
    );

    (uint256 finalLongShortRatio, uint256 totalValueStaked) = _getContractStakedBalanceAndRatio(
      marketIndex,
      longTokenAddress,
      shortTokenAddress,
      longTokenPrice,
      shortTokenPrice
    );

    if (finalLongShortRatio != 1) {
      _performShiftingStrategy(
        marketIndex,
        totalValueStaked,
        longTokenAddress,
        shortTokenAddress,
        longTokenPrice,
        shortTokenPrice
      );
    }

    //TODO can seperate this into it's own method?
    uint256 shares;

    if (strategyToken.totalSupply() == 0) {
      shares = totalValueStaked;
    } else {
      // Users deposited amount * cost per share
      //cost per share =
      shares =
        (((totalValueStaked - beforeBalanceOfContract)) * beforeBalanceOfContract) /
        strategyToken.totalSupply();
    }

    strategyToken.mint(msg.sender, shares);
  }

  /*╔═════════════════════════════╗
    ║       HELPER FUNCTIONS      ║
    ╚═════════════════════════════╝*/

  //Retrieves the number of long and short tokens staked by this contract
  function _getTotalLongAndShortTokensStaked(
    uint32 marketIndex,
    address longTokenAddress,
    address shortTokenAddress
  ) internal view returns (uint256, uint256) {
    IStaker _staker = IStaker(staker);
    uint256 amountStakedLong = _staker.userAmountStaked(longTokenAddress, address(this));
    uint256 amountStakedShort = _staker.userAmountStaked(shortTokenAddress, address(this));

    return (amountStakedLong, amountStakedShort);
  }

  //TODO calculate the shares for the user
  function _calculateUserShares() internal returns (uint256) {
    // add the logic for share distribution here
  }

  //Gets the $ balance of the staked tokens
  function _getContractStakedBalanceAndRatio(
    uint32 marketIndex,
    address longTokenAddress,
    address shortTokenAddress,
    uint256 longTokenPrice,
    uint256 shortTokenPrice
  ) internal view returns (uint256, uint256) {
    uint256 marketUpdateIndex = ILongShort(longShort).marketUpdateIndex(marketIndex);
    (uint256 _amountStakedLong, uint256 _amountStakedShort) = _getTotalLongAndShortTokensStaked(
      marketIndex,
      longTokenAddress,
      shortTokenAddress
    );
    uint256 longSideValue = (_amountStakedLong * longTokenPrice) / 1e18;
    uint256 shortSideValue = (_amountStakedShort * shortTokenPrice) / 1e18;

    uint256 longShortRatio = longSideValue / shortSideValue;
    uint256 contractBalance = (longSideValue + shortSideValue);

    return (longShortRatio, contractBalance);
  }

  function _getLongAndShortTokenPrice(uint32 marketIndex)
    internal
    view
    returns (uint256 longTokenPrice, uint256 shortTokenPrice)
  {
    uint256 marketUpdateIndex = ILongShort(longShort).marketUpdateIndex(marketIndex);
    uint256 longTokenPrice = ILongShort(longShort).syntheticToken_priceSnapshot(
      marketIndex,
      true,
      marketUpdateIndex
    );
    uint256 shortTokenPrice = ILongShort(longShort).syntheticToken_priceSnapshot(
      marketIndex,
      false,
      marketUpdateIndex
    );
  }

  /*╔═════════════════════════════╗
    ║           STRATEGY          ║
    ╚═════════════════════════════╝*/

  //Perform the shifting strategy of this contract to ensure 50/50 balance of long short token values
  function _performShiftingStrategy(
    uint32 marketIndex,
    uint256 totalValueStaked,
    address longTokenAddress,
    address shortTokenAddress,
    uint256 longTokenPrice,
    uint256 shortTokenPrice
  ) internal {
    (uint256 _amountStakedLong, uint256 _amountStakedShort) = _getTotalLongAndShortTokensStaked(
      marketIndex,
      longTokenAddress,
      shortTokenAddress
    );

    //50 - 50 split, can aim to make this resuable
    uint256 desiredAmountOfLongTokens = ((totalValueStaked / 2) * 1e18) / longTokenPrice;
    uint256 desiredAmountOfShortTokens = ((totalValueStaked / 2) * 1e18) / shortTokenPrice;

    //check that no shifting occurs if the distribution of long and short tokens are already correct
    if (_amountStakedLong > desiredAmountOfLongTokens) {
      //Shift to short side
      IStaker(staker).shiftTokens(_amountStakedLong - desiredAmountOfLongTokens, marketIndex, true);
    } else if (_amountStakedShort > desiredAmountOfShortTokens) {
      //shift to long side
      IStaker(staker).shiftTokens(
        _amountStakedShort - desiredAmountOfShortTokens,
        marketIndex,
        false
      );
    }
  }

  /// @notice Shifts tokens to either the long or the short position for the market to maintain 50/50 split in $ value
  /// @param marketIndex An uint32 which uniquely identifies a market.
  function performShiftingStrategy(uint32 marketIndex) external {
    address longTokenAddress = ILongShort(longShort).syntheticTokens(marketIndex, true);
    address shortTokenAddress = ILongShort(longShort).syntheticTokens(marketIndex, false);
    (uint256 longTokenPrice, uint256 shortTokenPrice) = _getLongAndShortTokenPrice(marketIndex);

    (uint256 longShortRatio, uint256 totalValueStaked) = _getContractStakedBalanceAndRatio(
      marketIndex,
      longTokenAddress,
      shortTokenAddress,
      longTokenPrice,
      shortTokenPrice
    );

    if (longShortRatio != 1) {
      _performShiftingStrategy(
        marketIndex,
        totalValueStaked,
        longTokenAddress,
        shortTokenAddress,
        longTokenPrice,
        shortTokenPrice
      );
    }
  }
}
