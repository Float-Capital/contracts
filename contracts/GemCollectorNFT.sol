// SPDX-License-Identifier: BUSL-1.1

pragma solidity 0.8.10;

import "@openzeppelin/contracts-upgradeable/token/ERC721/presets/ERC721PresetMinterPauserAutoIdUpgradeable.sol";
import "@openzeppelin/contracts-upgradeable/utils/CountersUpgradeable.sol";
import "@openzeppelin/contracts-upgradeable/proxy/utils/UUPSUpgradeable.sol";

import "./GEMS.sol";

/** @title GemCollectorNFT */
contract GemCollectorNFT is ERC721PresetMinterPauserAutoIdUpgradeable, UUPSUpgradeable {
  using CountersUpgradeable for CountersUpgradeable.Counter;
  CountersUpgradeable.Counter public tokenIds;
  CountersUpgradeable.Counter public levelIds;

  bytes32 public constant UPGRADER_ROLE = keccak256("UPGRADER_ROLE");

  address public gems;

  struct TokenData {
    string tokenUri;
    uint256 minRequiredGemsToMintToken;
  }

  mapping(uint256 => TokenData) public levelIdToTokenData;

  mapping(uint256 => string) private tokenURIs;

  // users has minted already the nft with that levelId
  mapping(address => mapping(uint256 => bool)) public hasMinted;

  mapping(uint256 => uint256) private tokenIDtoLevelID;

  event TokenAdded(uint256 levelId, string tokenURI, uint256 minGems);
  event NFTMinted(address user, address receiver, uint256 levelId, uint256 tokenId);

  function initializeNFT(
    address _admin,
    address _gems,
    string memory name,
    string memory symbol
  ) external virtual initializer {
    __ERC721PresetMinterPauserAutoId_init(name, symbol, "");
    __UUPSUpgradeable_init();

    renounceRole(DEFAULT_ADMIN_ROLE, msg.sender);
    renounceRole(MINTER_ROLE, msg.sender);

    _setupRole(DEFAULT_ADMIN_ROLE, _admin);
    _setupRole(MINTER_ROLE, _admin);
    _setupRole(PAUSER_ROLE, _admin);
    _setupRole(UPGRADER_ROLE, msg.sender);

    gems = _gems;
  }

  function addToken(string calldata tokenUri, uint256 minGems) external {
    require(hasRole(DEFAULT_ADMIN_ROLE, msg.sender), "user doesnt have admin control");
    require(minGems > 1e18, "Gems min requirement too small");
    require(bytes(tokenUri).length > 0, "Empty tokenUri");

    levelIds.increment();
    uint256 levelId = levelIds.current();

    levelIdToTokenData[levelId].tokenUri = tokenUri;
    levelIdToTokenData[levelId].minRequiredGemsToMintToken = minGems;

    emit TokenAdded(levelId, tokenUri, minGems);
  }

  function mintNFT(uint256 levelId, address receiver) external returns (uint256) {
    require(levelId != 0, "level id doesn't exist");
    require(levelId <= levelIds.current(), "level id doesn't exist");
    require(!hasMinted[msg.sender][levelId], "Already minted this nft");
    require(
      GEMS(gems).gems(msg.sender) >= levelIdToTokenData[levelId].minRequiredGemsToMintToken,
      "Not enough gems to mint this tokenId"
    );
    hasMinted[msg.sender][levelId] = true;

    tokenIds.increment();

    uint256 itemId = tokenIds.current();
    _mint(receiver, itemId);

    tokenIDtoLevelID[itemId] = levelId;

    emit NFTMinted(msg.sender, receiver, levelId, itemId);
    return itemId;
  }

  function tokenURI(uint256 tokenId) public view override returns (string memory) {
    require(_exists(tokenId), "URI query for nonexistent token");
    return levelIdToTokenData[tokenIDtoLevelID[tokenId]].tokenUri;
  }

  /// @notice Authorizes an upgrade to a new address.
  /// @dev Can only be called by addresses wih UPGRADER_ROLE
  function _authorizeUpgrade(address) internal override onlyRole(UPGRADER_ROLE) {}
}
