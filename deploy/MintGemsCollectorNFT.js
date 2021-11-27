const { network } = require("hardhat");

const { GEMS, GEMS_COLLECTOR_NFT } = require("../helper-hardhat-config");

const {
  mintLongNextPriceWithSystemUpdate,
  mintGemsNft,
} = require("../deployTests/DeployHelpers");

const delay = (ms) => new Promise((res) => setTimeout(res, ms));

module.exports = async (hardhatDeployArguments) => {
  const { getNamedAccounts, deployments } = hardhatDeployArguments;
  const { deploy } = deployments;

  // const { deployer, admin, user1, user2, user3 } = await getNamedAccounts();
  const accounts = await ethers.getSigners();

  const deployer = accounts[0].address;
  const admin = accounts[1].address;
  const user1 = accounts[2].address;

  // If already deployed
  const GemCollectorNFT = await deployments.get(GEMS_COLLECTOR_NFT);

  console.log("Gem collector NFT: ", GemCollectorNFT.address);

  const gemCollectorNFT = await ethers.getContractAt(
    GEMS_COLLECTOR_NFT,
    GemCollectorNFT.address
  );

  // susceptible to race condition :/
  // event NFTMinted(address user, address receiver, uint256 levelId, uint256 tokenId);
  gemCollectorNFT.on("NFTMinted", (user, receiver, levelId, tokenId) => {
    console.log("user");
    console.log(user);
    console.log("receiver");
    console.log(receiver);
    console.log("levelId");
    console.log(levelId.toString());
    console.log("tokenId");
    console.log(tokenId.toString());
  });

  const levelId = 0; // APE

  await gemCollectorNFT.connect(accounts[2]).mintNFT(levelId, user1);

  // Need for event listener to pick up on event emitted
  await delay(5000);
};

module.exports.tags = ["mintGemsNFT"];
