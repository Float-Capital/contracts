// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.3;

import "./abstract/AccessControlledAndUpgradeable.sol";

/** Contract giving user GEMS*/

// Inspired by https://github.com/andrecronje/rarity/blob/main/rarity.sol

/** @title GEMS */
contract GEMS is AccessControlledAndUpgradeable {
  bytes32 public constant GEM_ROLE = keccak256("GEM_ROLE");

  uint256 constant gems_per_day = 250e18;
  uint256 constant DAY = 1 days;

  mapping(address => uint256) public gems;
  mapping(address => uint256) public streak;
  mapping(address => uint256) public lastAction;

  event GemsCollected(address user, uint256 gems, uint256 streak);

  function initialize(
    address _admin,
    address _longShort,
    address _staker
  ) external initializer {
    _AccessControlledAndUpgradeable_init(_admin);
    _setupRole(GEM_ROLE, _longShort);
    _setupRole(GEM_ROLE, _staker);
  }

  // Say gm and get gems by performing an action in LongShort or Staker
  function gm(address user) external {
    if (hasRole(GEM_ROLE, msg.sender)) {
      uint256 usersLastAction = lastAction[user];
      uint256 blocktimestamp = block.timestamp;

      if (blocktimestamp - usersLastAction >= DAY) {
        // Award gems
        gems[user] += gems_per_day;

        // Increment streak
        if (blocktimestamp - usersLastAction < 2 * DAY) {
          streak[user] += 1;
        } else {
          streak[user] = 1; // reset streak to 1
        }

        lastAction[user] = blocktimestamp;
      }
      emit GemsCollected(user, gems[user], streak[user]);
    }
  }
}
