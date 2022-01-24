const { network, ethers } = require("hardhat");
const {
  LONGSHORT,
} = require("../../helper-hardhat-config");

let networkToUse = network.name;

if (!!process.env.HARDHAT_FORK) {
  networkToUse = process.env.HARDHAT_FORK;
}

const threeHunderedPercentFundingRate = "3000000000000000000";
const twoHunderedPercentFundingRate = "2000000000000000000";

module.exports = async ({ getNamedAccounts, deployments }) => {
  const { deploy } = deployments;
  const { deployer, admin } = await getNamedAccounts();
  /* 
  const longShort = await deploy(LONGSHORT, {
    from: deployer,
    log: true,
    proxy: {
      proxyContract: "UUPSProxy",
      initializer: false,
    },
  });
   */
  const longShortAddress = (await deployments.get(LONGSHORT)).address;
  const longShortContract = (await ethers.getContractAt(LONGSHORT, longShortAddress)).connect(admin);

  await longShortContract.changeMarketFundingRateMultiplier(2, twoHunderedPercentFundingRate)
  await longShortContract.changeMarketFundingRateMultiplier(3, threeHunderedPercentFundingRate)

  const newFundingRate = await longShortContract.fundingRateMultiplier_e18(3);

  if (newFundingRate.toString() != threeHunderedPercentFundingRate) {
    throw new Error("there was a problem with the upgrade, funding rate not set correctly. current value is" + newFundingRate.toString())
  }
};

module.exports.tags = ["funding-rate"];
