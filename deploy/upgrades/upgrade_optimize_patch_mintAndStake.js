const { network } = require("hardhat");
const {
  STAKER, LONGSHORT,
} = require("../../helper-hardhat-config");

let networkToUse = network.name;

if (!!process.env.HARDHAT_FORK) {
  networkToUse = process.env.HARDHAT_FORK;
}

module.exports = async ({ getNamedAccounts, deployments }) => {
  const { deploy } = deployments;
  const { deployer, admin } = await getNamedAccounts();
  console.log("deployer", deployer)
  console.log("admin", admin)

  const staker = await deploy(STAKER, {
    from: deployer,
    log: true,
    isUups: true,
    uupsAdmin: admin,
    proxy: {
      proxyContract: "UUPSProxy",
      methodName: 'upgradeToUsingCompactStakerState',
    },
    args: [],
  });
  const longShort = await deploy(LONGSHORT, {
    from: deployer,
    log: true,
    isUups: true,
    uupsAdmin: admin,
    proxy: {
      proxyContract: "UUPSProxy",
      methodName: 'upgradeToUsingCompactValueAndPriceSnapshots',
    },
    args: [],
  });
};

module.exports.tags = ["mint-and-stake-patch"];
