// SPDX-License-Identifier: BUSL-1.1

pragma solidity ^0.8.0;

// This library is an inverse to the "SafeERC20" library. It ALWAYS returns false rather than reverting.

import "@openzeppelin/contracts/token/ERC20/IERC20.sol";

library NoRevertERC20 {
  function noRevertTransfer(
    IERC20 token,
    address to,
    uint256 value
  ) internal returns (bool transferSuccess) {
    try token.transfer(to, value) returns (bool wasTransferSuccessful) {
      return wasTransferSuccessful;
    } catch {
      return false;
    }
  }
}
