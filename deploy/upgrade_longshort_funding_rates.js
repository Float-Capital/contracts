const { network } = require("hardhat");
const {
  LONGSHORT,
} = require("../helper-hardhat-config");

let networkToUse = network.name;

if (!!process.env.HARDHAT_FORK) {
  networkToUse = process.env.HARDHAT_FORK;
}

module.exports = async ({ getNamedAccounts, deployments }) => {
  const { deploy } = deployments;
  const { deployer, admin } = await getNamedAccounts();

  const longShort = await deploy(LONGSHORT, {
    from: deployer,
    log: true,
    proxy: {
      proxyContract: "UUPSProxy",
      initializer: false,
    },
  });

  const longShortContract = (await ethers.getContractAt(LONGSHORT, longShort.address)).connect(admin);

  longShortContract.changeMarketFundingRateMultiplier(2, "2000000000000000000" /*200%*/)
  longShortContract.changeMarketFundingRateMultiplier(3, "2000000000000000000" /*200%*/)
};

module.exports.tags = ["all", "funding-rate"];
