// SPDX-License-Identifier: BUSL-1.1

import "@openzeppelin/contracts-upgradeable/proxy/utils/Initializable.sol";
import "@openzeppelin/contracts-upgradeable/access/AccessControlUpgradeable.sol";
import "@openzeppelin/contracts-upgradeable/proxy/utils/UUPSUpgradeable.sol";

import "hardhat/console.sol";

abstract contract AccessControlledAndUpgradeable is
  Initializable,
  AccessControlUpgradeable,
  UUPSUpgradeable
{
  bytes32 public constant UPGRADER_ROLE = keccak256("UPGRADER_ROLE");
  bytes32 public constant ADMIN_ROLE = keccak256("ADMIN_ROLE");

  /// @notice Initializes the contract when called by parent initializers.
  /// @param initialAdmin The initial admin who will hold all roles.
  function _AccessControlledAndUpgradeable_init(address initialAdmin) internal initializer {
    __AccessControl_init();
    __UUPSUpgradeable_init();
    _AccessControlledAndUpgradeable_init_unchained(initialAdmin);
  }

  /// @notice Initializes the contract for contracts that already call both __AccessControl_init
  ///         and _UUPSUpgradeable_init when initializing.
  /// @param initialAdmin The initial admin who will hold all roles.
  function _AccessControlledAndUpgradeable_init_unchained(address initialAdmin) internal {
    require(initialAdmin != address(0));
    _setupRole(DEFAULT_ADMIN_ROLE, initialAdmin);
    _setupRole(ADMIN_ROLE, initialAdmin);
    _setupRole(UPGRADER_ROLE, initialAdmin);
  }

  /// @notice Authorizes an upgrade to a new address.
  /// @dev Can only be called by addresses wih UPGRADER_ROLE
  function _authorizeUpgrade(address) internal override onlyRole(UPGRADER_ROLE) {}
}
