// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.10;

import "../SyntheticTokenUpgradeable.sol";

//This isn't a perfect solution:
//Still possible to stake - must monitor this
//Impact: low

interface IStakerWithDeprecatedMarket {
  function unstakeAllFromDeprecatedMarket(
    uint256,
    address[] memory,
    bool
  ) external view returns (uint256);
}

/**
@title DeprecatedSyntheticTokenUpgradeable
@notice deprecated synthetic token for deprecated markets
*/
contract DeprecatedSyntheticTokenUpgradeable is SyntheticTokenUpgradeable {
  function stake(uint256 amount) external override {
    revert("Market deprecated, staking disabled");
  }

  function unstakeAllFor(address[] memory user) external {
    //This function doesn't work currently, exists for future deprecations
    IStakerWithDeprecatedMarket(staker).unstakeAllFromDeprecatedMarket(marketIndex, user, isLong);
  }

  function name() public view override returns (string memory) {
    return string(abi.encodePacked("deprecated", super.name()));
  }

  /**
   * @dev Returns the symbol of the token, usually a shorter version of the
   * name.
   */
  function symbol() public view override returns (string memory) {
    return string(abi.encodePacked("deprecated", super.symbol()));
  }

  function _beforeTokenTransfer(
    address sender,
    address to,
    uint256 amount
  ) internal override {
    if (sender != longShort || to != longShort) {
      revert("Market deprecated, transfer disabled");
    }
  }
}
