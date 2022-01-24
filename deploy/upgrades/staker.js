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

  let contractName = STAKER
  if (networkToUse == "polygon") {
    contractName = "StakerPolygon";
  } else if (networkToUse === "avalanche") {
    contractName = "StakerAvalanche";
  }

  const staker = await deploy(STAKER, {
    contractName,
    from: deployer,
    log: true,
    isUups: true,
    uupsAdmin: admin,
    proxy: {
      proxyContract: "UUPSProxy",
      initializer: false
    },
    args: [],
  });
};

module.exports.tags = ["upgrade-staker"];
