// SPDX-License-Identifier: BUSL-1.1

pragma solidity ^0.8.0;

// This library is inspried by the "@openzeppelin/contracts/utils/SafeCast.sol" library and written to handle casts that aren't handled by that library.

library SafeCastExtended {
  function toUint112(uint256 value) internal pure returns (uint112) {
    require(value <= type(uint112).max, "SafeCast: value doesn't fit in 112 bits");
    return uint112(value);
  }
}
