// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.3;

/**
@title SyntheticToken
@notice An ERC20 token that tracks or inversely tracks the price of an
        underlying asset with floating exposure.
*/
interface ISyntheticToken {
  // function MINTER_ROLE() external returns (bytes32);

  /// @notice Allows users to stake their synthetic tokens to earn Float.
  function stake(uint256) external;

  function mint(address, uint256) external;

  function totalSupply() external returns (uint256);

  function transferFrom(
    address,
    address,
    uint256
  ) external returns (bool);

  function transfer(address, uint256) external returns (bool);

  function burn(uint256 amount) external;
}
