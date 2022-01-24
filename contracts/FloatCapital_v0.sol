// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.10;

import "./abstract/AccessControlledAndUpgradeable.sol";

/** @title Float Capital Contract */
contract FloatCapital_v0 is AccessControlledAndUpgradeable {
  /*╔═════════════════════════════╗
    ║       CONTRACT SETUP        ║
    ╚═════════════════════════════╝*/

  function initialize(address _admin) public initializer {
    _AccessControlledAndUpgradeable_init(_admin);
  }

  /** A percentage of float token to accrue here for project
     development */
}
