// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.3;

interface ITokenFactory {
  function createSyntheticToken(
    string calldata syntheticName,
    string calldata syntheticSymbol,
    address staker,
    uint32 marketIndex,
    bool isLong
  ) external returns (address);
}
