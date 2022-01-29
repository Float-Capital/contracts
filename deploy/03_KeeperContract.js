const { config } = require("dotenv");
const { LONGSHORT, STAKER } = require("../helper-hardhat-config");

let networkToUse = network.name;

if (!!process.env.HARDHAT_FORK) {
  networkToUse = process.env.HARDHAT_FORK;
}

const oneHour = 3600; // 3600 = every hour
const twoHours = 7200; // 7200 = every 2 hours
const fourHours = 14400; // 7200 = every 2 hours
const sixHours = 21600; // 21600 = every 6 hours
const twelveHours = 43200; // 43200 = every 12 hours

const quarterPercent = "2500000000000000" // 2.5*10^15 is a price change of 0.25%
const halfPercent = "5000000000000000" // 5*10^15 is a price change of 0.5%


const pollygonConfig = [
  {
    marketIndex: 1,
    heartbeat: fourHours, deviationThreshold: halfPercent
  },
  {
    marketIndex: 2,
    heartbeat: fourHours, deviationThreshold: halfPercent
  },
  {
    marketIndex: 3,
    heartbeat: fourHours, deviationThreshold: halfPercent
  },
  {
    marketIndex: 4,
    heartbeat: fourHours, deviationThreshold: halfPercent
  },
  {
    marketIndex: 5,
    heartbeat: fourHours, deviationThreshold: halfPercent
  },
  {
    marketIndex: 6,
    heartbeat: fourHours, deviationThreshold: halfPercent
  },
  {
    marketIndex: 7,
    heartbeat: fourHours, deviationThreshold: halfPercent
  },
  {
    marketIndex: 8,
    heartbeat: fourHours, deviationThreshold: halfPercent
  },
  {
    marketIndex: 9,
    heartbeat: fourHours, deviationThreshold: halfPercent
  },
  {
    marketIndex: 10,
    heartbeat: fourHours, deviationThreshold: halfPercent
  }
]

const avalancheConfig = [
  {
    marketIndex: 1,
    heartbeat: sixHours, deviationThreshold: halfPercent
  },
  {
    marketIndex: 2,
    heartbeat: sixHours, deviationThreshold: halfPercent
  },
  {
    marketIndex: 3,
    heartbeat: sixHours, deviationThreshold: halfPercent
  },
  {
    marketIndex: 4,
    heartbeat: sixHours, deviationThreshold: halfPercent
  },
  {
    marketIndex: 5,
    heartbeat: sixHours, deviationThreshold: halfPercent
  }
]

let updateKeeperConfig = async (keeperContract, marketIndex, heartbeat, deviationThreshold) => {
  const currentHeartbeat = await keeperContract.updateTimeThresholdInSeconds(marketIndex)

  if (currentHeartbeat.eq(ethers.BigNumber.from(heartbeat))) {
    console.log("heartbeat set correctly for market", marketIndex)
  } else {
    console.log(`heartbeat incorrect for market ${marketIndex} - should be ${heartbeat.toString()} but it is currently ${currentHeartbeat.toString()}`)
    await keeperContract.updateHeartbeatThresholdForMarket(marketIndex, heartbeat)
  }

  const currentDeviationThreshold = await keeperContract.percentChangeThreshold(marketIndex)

  if (currentDeviationThreshold.eq(ethers.BigNumber.from(deviationThreshold))) {
    console.log("deviation threshold set correctly for market", marketIndex)
  } else {
    console.log(`deviation threshold incorrect for market ${marketIndex} - should be ${deviationThreshold.toString()} but it is currently ${currentDeviationThreshold.toString()}`)
    await keeperContract.updatePercentChangeThresholdForMarket(marketIndex, deviationThreshold)
  }
}

module.exports = async ({ deployments }) => {
  const { deploy } = deployments;
  // const { deployer, admin } = await getNamedAccounts();
  const accounts = await ethers.getSigners();

  const deployer = accounts[0].address;

  console.log("Deploying contracts with the account:", deployer);
  console.log(deployer);
  // const keeper = await deploy("Keeper", {
  //   contract: "KeeperExperiment",
  //   from: deployer,
  //   log: true,
  //   proxy: {
  //     proxyContract: "UUPSProxy",
  //     execute: {
  //       methodName: "initialize",
  //       args: [
  //         deployer,
  //         longShort.address,
  //         staker.address,
  //       ],
  //     },
  //   },
  // });
  const keeper = await deployments.get("Keeper");
  const keeperContract = await ethers.getContractAt(
    "KeeperExperiment",
    keeper.address
  );

  console.log("keeper address", keeper.address)

  if (networkToUse === "polygon") {
    for ({ marketIndex, heartbeat, deviationThreshold } of pollygonConfig) {
      await updateKeeperConfig(keeperContract, marketIndex, heartbeat, deviationThreshold)
    }
  } else if (networkToUse === "avalanche") {
    for ({ marketIndex, heartbeat, deviationThreshold } of avalancheConfig) {
      await updateKeeperConfig(keeperContract, marketIndex, heartbeat, deviationThreshold)
    }
  } else {
    throw new Error("keeper config not defined for this network")
  }
};
module.exports.tags = ["keeper"];
