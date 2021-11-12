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
  const { deployer, admin, user1, user2, user3 } = await getNamedAccounts();

  let Gems;
  try {
    Gems = await deployments.get(GEMS);
  } catch {
    // for testing deployment locally without running all scripts
    Gems = { address: "0x0000000000000000000000000000000000000000" };
  }

  // should recognise as deployed already and use that contract
  const GemCollectorNFT = await deploy(GEMS_COLLECTOR_NFT, {
    from: deployer,
    proxy: {
      proxyContract: "UUPSProxy",
      execute: {
        methodName: "initializeNFT",
        args: [admin, Gems.address, "Float Capital Gem Collector", "FCG"],
      },
    },
    log: true,
  });

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
    console.log(levelId);
    console.log("tokenId");
    console.log(tokenId);
  });

  const levelId = 0; // APE

  await mintGemsNft(gemCollectorNFT.connect(user1), levelId, user1);

  // Need for event listener to pick up on event emitted
  await delay(5000);
};

module.exports.tags = ["mintGemsNFT"];
