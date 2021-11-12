const { LONGSHORT, STAKER } = require("../helper-hardhat-config");

module.exports = async ({ deployments }) => {
  const { deploy } = deployments;
  // const { deployer, admin } = await getNamedAccounts();
  const accounts = await ethers.getSigners();

  const deployer = accounts[0].address;

  const longShort = await deployments.get(LONGSHORT);
  const staker = await deployments.get(STAKER);

  console.log("Deploying contracts with the account:", deployer);
  console.log(deployer);
  const keeper = await deploy("Keeper", {
    contract: "KeeperExperiment",
    from: deployer,
    log: true,
    proxy: {
      proxyContract: "UUPSProxy",
      execute: {
        methodName: "initialize",
        args: [
          deployer,
          longShort.address,
          staker.address,
        ],
      },
    },
  });
  const keeperContract = await ethers.getContractAt(
    "KeeperExperiment",
    keeper.address
  );

  // 43200 = every 12 hours
  // 3600 = every hour
  await keeperContract.updateHeartbeatThresholdForMarket(1, 43200) // in times of extremely low volotility and no user interaction only update every hour
  await keeperContract.updatePercentChangeThresholdForMarket(
    1,
    "25000000000000000" // 2.5*10^15 is a price change of 0.25%
  )
  console.log("market 1 configured")

  await keeperContract.updateHeartbeatThresholdForMarket(2, 3600) // in times of extremely low volotility and no user interaction only update every hour
  await keeperContract.updatePercentChangeThresholdForMarket(
    2,
    "2500000000000000" // 2.5*10^15 is a price change of 0.25%
  )
  console.log("market 2 configured")
};
module.exports.tags = ["keeper"];
