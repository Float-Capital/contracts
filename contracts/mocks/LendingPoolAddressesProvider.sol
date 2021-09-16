// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.3;

import "../interfaces/aave/ILendingPoolAddressesProvider.sol";

contract LendingPoolAddressesProviderMock is ILendingPoolAddressesProvider {
  address public lendingPool;

  function setLendingPool(address _lendingPool) external {
    lendingPool = _lendingPool;
  }

  function getLendingPool() external view override returns (address) {
    return lendingPool;
  }
}
