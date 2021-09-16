// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.3;

import "@openzeppelin/contracts-upgradeable/token/ERC20/presets/ERC20PresetMinterPauserUpgradeable.sol";

import "./interfaces/IFloatToken.sol";

import "@openzeppelin/contracts-upgradeable/token/ERC20/ERC20Upgradeable.sol";
import "@openzeppelin/contracts-upgradeable/token/ERC20/extensions/ERC20BurnableUpgradeable.sol";
import "@openzeppelin/contracts-upgradeable/security/PausableUpgradeable.sol";
import "@openzeppelin/contracts-upgradeable/access/AccessControlUpgradeable.sol";
import "@openzeppelin/contracts-upgradeable/token/ERC20/extensions/draft-ERC20PermitUpgradeable.sol";
import "@openzeppelin/contracts-upgradeable/token/ERC20/extensions/ERC20VotesUpgradeable.sol";
import "@openzeppelin/contracts-upgradeable/proxy/utils/Initializable.sol";
import "@openzeppelin/contracts-upgradeable/proxy/utils/UUPSUpgradeable.sol";

/**
 @title FloatToken
 @notice The Float Token is the governance token for the Float Capital protocol
 */
contract FloatToken is
  IFloatToken,
  Initializable,
  ERC20Upgradeable,
  ERC20BurnableUpgradeable,
  PausableUpgradeable,
  AccessControlUpgradeable,
  ERC20PermitUpgradeable,
  ERC20VotesUpgradeable,
  UUPSUpgradeable
{
  bytes32 public constant PAUSER_ROLE = keccak256("PAUSER_ROLE");
  bytes32 public constant MINTER_ROLE = keccak256("MINTER_ROLE");
  bytes32 public constant UPGRADER_ROLE = keccak256("UPGRADER_ROLE");

  /**
   @notice Initialize the Float Token with relevant
   @dev This function is called `initialize` to differentiate it from `initialize(string,string)` in the parent contract which should NOT be called to initialize this contract. 
   @param name The name of the Float governance token
   @param symbol The ticker representing the token
   @param stakerAddress The staker contract that controls minting of the token
   */
  function initialize(
    string calldata name,
    string calldata symbol,
    address stakerAddress
  ) external initializer {
    __ERC20_init(name, symbol);
    __ERC20Burnable_init();
    __Pausable_init();
    __AccessControl_init();
    __ERC20Permit_init(name);
    __UUPSUpgradeable_init();

    renounceRole(DEFAULT_ADMIN_ROLE, msg.sender);
    renounceRole(MINTER_ROLE, msg.sender);

    _setupRole(DEFAULT_ADMIN_ROLE, stakerAddress);
    _setupRole(MINTER_ROLE, stakerAddress);
    _setupRole(PAUSER_ROLE, msg.sender);

    _setupRole(UPGRADER_ROLE, msg.sender);

    // Token starts as paused
    _pause();
  }

  /*╔═══════════════════════════════════════════════════════════════════╗
    ║    FUNCTIONS INHERITED BY ERC20PresetMinterPauserUpgradeable      ║
    ╚═══════════════════════════════════════════════════════════════════╝*/

  /** 
  @notice Mints an amount of Float tokens for an address.
  @dev Can only be called by addresses with a MINTER_ROLE. 
        This should correspond to the Staker contract.
  @param to The address for which to mint the tokens for.
  @param amount Amount of synthetic tokens to mint in wei.
  */
  function mint(address to, uint256 amount) external override(IFloatToken) onlyRole(MINTER_ROLE) {
    _mint(to, amount);
  }

  /**
   @notice modify token functionality so that a pausing this token doesn't affect minting
   @dev Pause functionality in the open zeppelin ERC20PresetMinterPauserUpgradeable comes from the below function.
    We override it to exclude anyone with the minter role (ie the Staker contract)
   @param from address tokens are being sent from
   @param to address tokens are being sent to
   @param amount amount of tokens being sent
   */
  function _beforeTokenTransfer(
    address from,
    address to,
    uint256 amount
  ) internal virtual override {
    require(!paused() || hasRole(MINTER_ROLE, _msgSender()), "Paused and not minter");

    super._beforeTokenTransfer(from, to, amount);
  }

  function pause() external onlyRole(PAUSER_ROLE) {
    _pause();
  }

  function unpause() external onlyRole(PAUSER_ROLE) {
    _unpause();
  }

  function _authorizeUpgrade(address newImplementation) internal override onlyRole(UPGRADER_ROLE) {}

  function _afterTokenTransfer(
    address from,
    address to,
    uint256 amount
  ) internal override(ERC20Upgradeable, ERC20VotesUpgradeable) {
    super._afterTokenTransfer(from, to, amount);
  }

  function _mint(address to, uint256 amount)
    internal
    override(ERC20Upgradeable, ERC20VotesUpgradeable)
  {
    super._mint(to, amount);
  }

  function _burn(address account, uint256 amount)
    internal
    override(ERC20Upgradeable, ERC20VotesUpgradeable)
  {
    super._burn(account, amount);
  }

  function totalSupply()
    public
    view
    virtual
    override(ERC20Upgradeable, IFloatToken)
    returns (uint256)
  {
    return ERC20Upgradeable.totalSupply();
  }

  function transfer(address recipient, uint256 amount)
    public
    virtual
    override(ERC20Upgradeable, IFloatToken)
    returns (bool)
  {
    return ERC20Upgradeable.transfer(recipient, amount);
  }

  function burnFrom(address account, uint256 amount)
    public
    virtual
    override(ERC20BurnableUpgradeable, IFloatToken)
  {
    ERC20BurnableUpgradeable.burnFrom(account, amount);
  }
}
