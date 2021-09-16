// SPDX-License-Identifier: BUSL-1.1

pragma solidity ^0.8.0;

import "@openzeppelin/contracts/token/ERC20/extensions/draft-ERC20Permit.sol";
import "@openzeppelin/contracts-upgradeable/token/ERC20/ERC20Upgradeable.sol";
import "@openzeppelin/contracts-upgradeable/token/ERC20/extensions/ERC20BurnableUpgradeable.sol";

contract StrategyToken is ERC20Upgradeable, ERC20BurnableUpgradeable {
  constructor(string memory name, string memory symbol) {
    __ERC20_init(name, symbol);
  }

  function mint(address _recipient, uint256 _amount) external {
    _mint(_recipient, _amount);
  }
}
