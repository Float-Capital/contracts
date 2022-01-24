// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.10;

// Source: https://compound.finance/docs/ctokens

interface CErc20 {
  function mint(uint256) external returns (uint256);

  function exchangeRateCurrent() external returns (uint256);

  function supplyRatePerBlock() external returns (uint256);

  function redeem(uint256) external returns (uint256);

  function redeemUnderlying(uint256) external returns (uint256);

  function balanceOfUnderlying(address) external returns (uint256);
}
