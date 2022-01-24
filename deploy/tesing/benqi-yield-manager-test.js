const { network } = require("hardhat");
const avalancheDaiAddress = "0xd586E7F844cEa2F87f50152665BCbc2C279D8d70";

const benqiDaiCToken = "0x835866d37AFB8CB8F8334dCCdaf66cf01832Ff5D";

let networkToUse = network.name;

if (!!process.env.HARDHAT_FORK) {
  networkToUse = process.env.HARDHAT_FORK;
}

module.exports = async ({ getNamedAccounts, deployments }) => {
  const { deploy } = deployments;
  const accounts = await ethers.getSigners();

  const deployer = accounts[0].address;
  const admin = accounts[1].address;

  let paymentTokenAddress;

  if (networkToUse === "avalanche") {
    paymentTokenAddress = avalancheDaiAddress;
  } else {
    throw new Error("Wrong network")
  }

  let benqiYieldManager = await deploy("YieldManagerBenqiTest", {
    from: deployer,
    log: true,
    proxy: {
      proxyContract: "UUPSProxy",
      execute: {
        methodName: "initialize",
        args: [
          deployer,
          deployer,
          paymentTokenAddress,
          benqiDaiCToken,
          deployer
        ],
      },
    },
  });

  console.log("benqi yield manager deployed at", benqiYieldManager.address)
};
module.exports.tags = ["benqi-test"];
