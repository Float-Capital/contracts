const { network } = require("hardhat");

const { GEMS, GEMS_COLLECTOR_NFT } = require("../../helper-hardhat-config");

const { addGemsNfts } = require("../../deployTests/helpers/DeployHelpers");

const delay = (ms) => new Promise((res) => setTimeout(res, ms));

module.exports = async (hardhatDeployArguments) => {
  const { getNamedAccounts, deployments } = hardhatDeployArguments;
  const { deploy } = deployments;
  // const { deployer, admin, user1, user2, user3 } = await getNamedAccounts();
  const accounts = await ethers.getSigners();

  const deployer = accounts[0].address;
  const admin = accounts[1].address;

  let Gems;
  try {
    Gems = await deployments.get(GEMS);
  } catch {
    // for testing deployment locally without running all scripts
    Gems = { address: "0x0000000000000000000000000000000000000000" };
  }

  const GemCollectorNFT = await deploy(GEMS_COLLECTOR_NFT, {
    from: deployer,
    proxy: {
      proxyContract: "UUPSProxy",
      execute: {
        methodName: "initializeNFT",
        args: [admin, Gems.address, "Float Capital Gems", "FCG"],
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
  // event TokenAdded(uint256 levelId, string tokenURI, uint256 minGems);
  gemCollectorNFT.on("TokenAdded", (levelId, tokenURI, minGems) => {
    console.log("levelId");
    console.log(levelId.toString());
    console.log("minGems");
    console.log(minGems.toString());
    console.log("tokenURI");
    console.log(tokenURI.toString());
  });

  const initialNfts = [
    {
      tokenURI: "https://float.capital/nfts/gems/ape",
      minGems: "250000000000000000000",
    },
    {
      tokenURI: "https://float.capital/nfts/gems/chad",
      minGems: "2500000000000000000000",
    },
    {
      tokenURI: "https://float.capital/nfts/gems/degen",
      minGems: "10000000000000000000000",
    },
  ];

  for (let i = 0; i < initialNfts.length; i++) {
    await addGemsNfts(
      gemCollectorNFT.connect(admin),
      initialNfts[i].tokenURI,
      initialNfts[i].minGems
    );
  }

  // Need for event listener to pick up on event emitted
  await delay(10000);
};

module.exports.tags = ["deployGemsNFT"];
