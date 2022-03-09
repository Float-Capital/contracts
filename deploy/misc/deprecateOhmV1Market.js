const { ethers } = require("hardhat");

// https://polygonscan.com/address/0xa8B05B6337040c0529919BDB51f6B40A684eb08C
let ohmV1FeedAddress =
  "0xa8B05B6337040c0529919BDB51f6B40A684eb08C";

let OHMv1MarketIndex = 3;

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
    // Deploy deprecated long token
    let syntheticTokenLong =
      await deploy(
        "SL2OHM",
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
    // Deploy deprecated short token
    let syntheticTokenShort =
      await deploy(
        "SS2OHM",
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
    // Deploy fixed (deprecated) oracle manager
    let oracleManager =
      await deploy(
        "OracleManager2OHM",
        {
          "from": deployer,
          "log": true,
          "contract": "DeprecatedOracleManagerChainlink",
          "args": [admin, ohmV1FeedAddress, longShort.address],
        },
      );
    let connectedLongShort = longShort.connect(adminSigner);

    // Make funding rate 0 for market.
    await connectedLongShort.changeMarketFundingRateMultiplier(OHMv1MarketIndex, 0);

    console.log("removed funding rate")
    console.log("new oracle address", oracleManager.address);
    // Update the oracle that is used
    await connectedLongShort.updateMarketOracle(OHMv1MarketIndex, oracleManager.address);

    // sanity checks, just make sure we upgraded the correct synthetic tokens 🙈
    const longAddress = await longShort.syntheticTokens(OHMv1MarketIndex, true);
    const shortAddress = await longShort.syntheticTokens(OHMv1MarketIndex, false);
    console.log("(sanity check) - should equal", longAddress, syntheticTokenLong.address)
    console.log("(sanity check) - should equal", shortAddress, syntheticTokenShort.address)

    // Update system state to finalise the deprecation 🚀💪
    await longShort.updateSystemState(OHMv1MarketIndex);
  } else {
    throw new Error("Only works on polygon at the moment")
  }
  console.log("after test txs");
};
module.exports.tags = ["deprecate-ohm-v1"];
