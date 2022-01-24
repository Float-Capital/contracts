const { ethers } = require("hardhat");

// https://polygonscan.com/address/0x4cE90F28C6357A7d3F47D680723d18AF3684cD00
let ohmV2FeedAddress =
  "0x4cE90F28C6357A7d3F47D680723d18AF3684cD00";

const {
  LONGSHORT,
} = require("../../helper-hardhat-config");

let networkToUse = network.name;

if (!!process.env.HARDHAT_FORK) {
  networkToUse = process.env.HARDHAT_FORK;
}

module.exports = async (hardhatDeployArguments) => {
  console.log("setup contracts");
  const { getNamedAccounts, deployments: { deploy } } = hardhatDeployArguments;
  const { deployer, admin, discountSigner } = await getNamedAccounts();

  const loadedAccounts = await ethers.getSigners();

  const adminSigner = loadedAccounts[1];

  const LongShort = await deployments.get(LONGSHORT);
  const longShort = await ethers.getContractAt(LONGSHORT, LongShort.address);

  if (networkToUse == "polygon") {
    let syntheticTokenLong =
      await deploy(
        "SL2OHMv2-deprecated",
        {
          "from": deployer,
          "log": true,
          "contract": "DeprecatedSyntheticTokenUpgradeable",
          "proxy": {
            "proxyContract": "UUPSProxy",
          },
          isUups: true,
          uupsAdmin: deployer,
        },
      );

    let syntheticTokenShort =
      await deploy(
        "SS2OHMv2-deprecated",
        {
          "from": deployer,
          "log": true,
          "contract": "DeprecatedSyntheticTokenUpgradeable",
          "proxy": {
            "proxyContract": "UUPSProxy",
          },
          isUups: true,
          uupsAdmin: deployer,
        },
      );

    let oracleManager =
      await deploy(
        "OracleManagerOHMv2deprecated",
        {
          "from": deployer,
          "log": true,
          "contract": "DeprecatedOracleManagerChainlink",
          "args": [admin, ohmV2FeedAddress, longShort.address],
        },
      );

    console.log("admin", admin);
    console.log("new oracle address", oracleManager.address);
    await longShort.connect(adminSigner).updateMarketOracle(7, oracleManager.address);

    const longAddress = await longShort.syntheticTokens(7, true);
    const shortAddress = await longShort.syntheticTokens(7, false);
    console.log("should equal", longAddress, syntheticTokenLong.address)
    console.log("should equal", shortAddress, syntheticTokenShort.address)

    await longShort.updateSystemState(7);
  } else {
    throw new Error("Only works on polygon at the moment")
  }
  console.log("after test txs");
};
module.exports.tags = ["deprecate-ohm"];
