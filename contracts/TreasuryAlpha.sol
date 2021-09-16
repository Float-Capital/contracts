// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.3;

import "@openzeppelin/contracts/token/ERC20/IERC20.sol";
import "@openzeppelin/contracts/token/ERC20/utils/SafeERC20.sol";

import "./abstract/AccessControlledAndUpgradeable.sol";
import "./interfaces/IFloatToken.sol";
import "./interfaces/ILongShort.sol";

import "@openzeppelin/contracts/utils/math/Math.sol";

/** This contract implementation is purely for the alpha, allowing the burning of FLT tokens
  for a proportional share of the value held in the treasury. In contrast, the beta launch will be
  rely on a more robust governance mechanism to vote on the buying and buring of FLT tokens using
  treasury funds.*/

/** @title Treasury Contract */
contract TreasuryAlpha is AccessControlledAndUpgradeable {
  //Using Open Zeppelin safe transfer library for token transfers
  using SafeERC20 for IERC20;

  address public paymentToken;
  address public floatToken;
  address public longShort;
  // An aproximation of what the FLT price should be according to the yield at the time.
  uint256 public basePrice;
  bool public redemptionsActivated;

  event BasePriceUpdated(uint256 newBasePrice);

  function initialize(
    address _admin,
    address _paymentToken,
    address _floatToken,
    address _longShort
  ) external initializer {
    _AccessControlledAndUpgradeable_init(_admin);
    paymentToken = _paymentToken;
    floatToken = _floatToken;
    longShort = _longShort;
  }

  function onlyAdminModifierLogic() internal virtual {
    _checkRole(ADMIN_ROLE, msg.sender);
  }

  modifier onlyAdmin() {
    onlyAdminModifierLogic();
    _;
  }

  modifier redemptionsActive() {
    require(redemptionsActivated, "redemptions haven't been activated");
    _;
  }

  function _getValueLockedInTreasury() internal view returns (uint256) {
    return IERC20(paymentToken).balanceOf(address(this));
  }

  function _getFloatTokenSupply() internal view returns (uint256) {
    return IFloatToken(floatToken).totalSupply();
  }

  function updateBasePrice(uint256 newBasePrice) public onlyAdmin {
    // What should the minimum for this value be? 0.2 DAI per token seems reasonable if we are targetting 0.5 DAI as the normal price (and say Aave yield is poor or similar)
    require(newBasePrice > 2e17, "base price too low");

    basePrice = newBasePrice;
    emit BasePriceUpdated(newBasePrice);
  }

  function activateRedemptions() public onlyAdmin {
    redemptionsActivated = true;
  }

  function burnFloatForShareOfTreasury(uint256 amountOfFloatToBurn) external redemptionsActive {
    uint256 priceAccordingToTreasuryAndSupply = (_getValueLockedInTreasury() * 1e18) /
      _getFloatTokenSupply();
    // In normal operation the `priceAccordingToTreasuryAndSupply` value will be an over-estimation favouring people who withdraw early. Thus typically the 'basePrice' will be a bit lower to prevent this in typical cases.
    // We take the min of these two values so it is impossible for us to run with the money.
    uint256 priceToUse = Math.min(priceAccordingToTreasuryAndSupply, basePrice);

    uint256 amountToRecieve = (priceToUse * amountOfFloatToBurn) / 1e18;

    IFloatToken(floatToken).burnFrom(msg.sender, amountOfFloatToBurn); // Can modify the core FLT token if wanted to remove the need for this step. // Currently requires user to approve treasury contract.
    IERC20(paymentToken).safeTransfer(msg.sender, amountToRecieve);
  }

  function convertSynthsToPaymentTokenNextPriceLong(uint32 marketIndex, uint256 tokens_redeem)
    external
  {
    ILongShort(longShort).redeemLongNextPrice(marketIndex, tokens_redeem);
  }

  function convertSynthsToPaymentTokenNextPriceShort(uint32 marketIndex, uint256 tokens_redeem)
    external
  {
    ILongShort(longShort).redeemShortNextPrice(marketIndex, tokens_redeem);
  }
}
