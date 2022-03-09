const { network } = require("hardhat");
const {
  TASK_COMPILE_SOLIDITY_LOG_NOTHING_TO_COMPILE,
} = require("hardhat/builtin-tasks/task-names");
const { json } = require("hardhat/internal/core/params/argumentTypes");
const { STAKER, LONGSHORT } = require("../../helper-hardhat-config");

const SLOW_TRADE_SYNTHETIC_TOKEN = "SlowTradeSyntheticTokenUpgradeable";

let networkToUse = network.name;

if (!!process.env.HARDHAT_FORK) {
  networkToUse = process.env.HARDHAT_FORK;
}

let syntheticMarketSymbols = [];
switch (networkToUse) {
  case "polygon":
    syntheticMarketSymbols = ["2AXS", "2CVI", "2LINK", "2OHMv2", "3TH", "F3"];
    break;
  case "avalanche":
    syntheticMarketSymbols = ["2JOE", "2QI", "2SPELL", "Avax2"];
    break;
  case "mumbai":
    syntheticMarketSymbols = ["BTC", "ETH", "MATIC", "SAND"];
    break;
}
module.exports = async ({ getNamedAccounts, deployments }) => {
  if (
    networkToUse != "polygon" &&
    networkToUse != "mumbai" &&
    networkToUse != "avalanche"
  ) {
    throw new Error("Only run this code on polygon, mumbai or avalanche");
  }
  const { deploy } = deployments;
  const { deployer, admin } = await getNamedAccounts();
  console.log("deployer", deployer);
  console.log("admin", admin);

  // Upgrade LongShort
  let longShortContractName = LONGSHORT;
  if (networkToUse == "polygon") {
    longShortContractName = "LongShortPolygon";
  } else if (networkToUse === "avalanche") {
    longShortContractName = "LongShortAvalanche";
  } else if (networkToUse === "mumbai") {
    longShortContractName = "LongShortTesting";
  }
  const longShort = await deploy(LONGSHORT, {
    contract: longShortContractName,
    from: deployer,
    log: true,
    isUups: true,
    uupsAdmin: admin,
    proxy: {
      proxyContract: "UUPSProxy",
      initializer: false,
    },
    args: [],
  });

  // Upgrade Staker
  let stakerContractName = STAKER;
  if (networkToUse == "polygon" || networkToUse == "mumbai") {
    stakerContractName = "StakerPolygon";
  } else if (networkToUse === "avalanche") {
    stakerContractName = "StakerAvalanche";
  }

  const staker = await deploy(STAKER, {
    contract: stakerContractName,
    from: deployer,
    log: true,
    isUups: true,
    uupsAdmin: admin,
    proxy: {
      proxyContract: "UUPSProxy",
      initializer: false,
    },
    args: [],
  });

  for (let i = 0; i < syntheticMarketSymbols.length; i++) {
    // Polygon deployment needned some assistance!
    if (networkToUse == "polygon") {
      const slowTradeSynthImplementation =
        "0xA21100a883a22c3c4fFaf7F21D1aF15CBD5b965D"; // https://polygonscan.com/address/0xA21100a883a22c3c4fFaf7F21D1aF15CBD5b965D#code
      const marketSymbol = syntheticMarketSymbols[i];

      const shortSynthName = "SS" + marketSymbol;
      const longSynthName = "SL" + marketSymbol;
      const shortSynthAddress = (await deployments.get(shortSynthName)).address;
      const longSynthAddress = (await deployments.get(longSynthName)).address;

      const shortShortContract = await ethers.getContractAt(
        "UUPSUpgradeable",
        shortSynthAddress
      );
      const longShortContract = await ethers.getContractAt(
        "UUPSUpgradeable",
        longSynthAddress
      );

      console.log(
        `synth token addresses (${marketSymbol}) [long - short]`,
        "https://polygonscan.com/address/" + longSynthAddress,
        "https://polygonscan.com/address/" + shortSynthAddress,
      );

      await shortShortContract.upgradeTo(slowTradeSynthImplementation);
      console.log("upgraded short")
      await longShortContract.upgradeTo(slowTradeSynthImplementation);
      console.log("upgraded long")
    } else {
      await deploy("SS" + marketSymbol, {
        contract: SLOW_TRADE_SYNTHETIC_TOKEN,
        from: deployer,
        log: true,
        isUups: true,
        uupsAdmin: deployer,
        proxy: {
          proxyContract: "UUPSProxy",
          initializer: false,
        },
        args: [],
      })
        .then((result) =>
          console.log(marketSymbol, "upgraded the short market")
        )
        .catch((error) =>
          console.log("Error deploying " + marketSymbol + " short", error)
        );

      await deploy("SL" + marketSymbol, {
        contract: SLOW_TRADE_SYNTHETIC_TOKEN,
        from: deployer,
        log: true,
        isUups: true,
        uupsAdmin: deployer,
        proxy: {
          proxyContract: "UUPSProxy",
          initializer: false,
        },
        args: [],
      })
        .then((result) => console.log(marketSymbol, "upgraded the long market"))
        .catch((error) =>
          console.log("Error deploying " + marketSymbol + " long", error)
        );
    }
  }
};

module.exports.tags = ["upgrade-longshort-staker-mias"];
