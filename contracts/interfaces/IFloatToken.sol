// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.3;

interface IFloatToken {
  function mint(address to, uint256 amount) external;

  function transfer(address, uint256) external returns (bool);

  function totalSupply() external view returns (uint256);

  function burnFrom(address account, uint256 amount) external virtual;
}
