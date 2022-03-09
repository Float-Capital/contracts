// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.10;

import "./SyntheticTokenUpgradeable.sol";

/**
@title SlowTradeSyntheticTokenUpgradeable
@notice Magic Internet Assets that are designed to be slow trading
*/
contract SlowTradeSyntheticTokenUpgradeable is SyntheticTokenUpgradeable {
  function _beforeTokenTransfer(
    address sender,
    address to,
    uint256 amount
  ) internal override {
    // Staker is able to move tokens any time (to stake/unstake), and longShort is able to receive tokens at anytime too (for a redeem)
    if (sender != staker && to != staker && sender != longShort && to != longShort) {
      ILongShort(longShort).checkIfUserIsEligibleToSendSynth(sender, marketIndex, isLong);
    }

    super._beforeTokenTransfer(sender, to, amount);
  }
}
