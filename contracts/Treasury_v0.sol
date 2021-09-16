// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.3;

import "@openzeppelin/contracts/token/ERC20/IERC20.sol";

import "./abstract/AccessControlledAndUpgradeable.sol";
import "./interfaces/IFloatToken.sol";
import "./interfaces/ILongShort.sol";

/** This contract implementation is purely for the alpha, allowing the burning of FLT tokens
  for a proportional share of the value held in the treasury. In contrast, the beta launch will be
  rely on a more robust governance mechanism to vote on the buying and buring of FLT tokens using
  treasury funds.*/

/** @title Treasury Contract */
contract Treasury_v0 is AccessControlledAndUpgradeable {
  address public paymentToken;
  address public floatToken;
  address public longShort;

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
