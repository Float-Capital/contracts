const {
  launchQiMarket,
} = require("../../../deployTests/AvalancheTransactions");
const { ethers } = require("hardhat");

const {
  STAKER,
  TEST_COLLATERAL_TOKEN,
  TREASURY,
  LONGSHORT,
  isAlphaLaunch,
  TREASURY_ALPHA,
} = require("../../../helper-hardhat-config");
const { avalancheDaiAddress } = require("../../config");
const { assert } = require("chai");

let networkToUse = network.name;

if (!!process.env.HARDHAT_FORK) {
  networkToUse = process.env.HARDHAT_FORK;
}

module.exports = async (hardhatDeployArguments) => {
  const { getNamedAccounts, deployments } = hardhatDeployArguments;
  const { admin } = await getNamedAccounts();

  console.log("All accounts", await getNamedAccounts());

  ////////////////////////
  //Retrieve Deployments//
  ////////////////////////
  console.log("1");
  let paymentTokenAddress;

  console.log("2");
  if (networkToUse === "avalanche") {
    paymentTokenAddress = avalancheDaiAddress;
  } else {
    console.error("This command is only available on avalanche");
  }

  // TEST_COLLATERAL_TOKEN = ERC20 token
  console.log("3", TEST_COLLATERAL_TOKEN);
  const paymentToken = await ethers.getContractAt(
    TEST_COLLATERAL_TOKEN,
    paymentTokenAddress
  );

  console.log("4");
  const LongShort = await deployments.get(LONGSHORT);
  console.log("5");
  const longShort = await ethers.getContractAt(LONGSHORT, LongShort.address);
  console.log("6", LongShort.address);

  let treasuryToUse = isAlphaLaunch ? TREASURY_ALPHA : TREASURY;
  console.log("7");
  const Treasury = await deployments.get(treasuryToUse);
  console.log("8");
  const treasury = await ethers.getContractAt(treasuryToUse, Treasury.address);
  console.log("9");

  const Staker = await deployments.get(STAKER);
  console.log("10");
  const staker = await ethers.getContractAt(STAKER, Staker.address);
  console.log("11");

  if (networkToUse == "avalanche") {
    console.log("avalanche transactions");
    await launchQiMarket(
      {
        staker,
        longShort: longShort.connect(admin),
        paymentToken,
        treasury,
      },
      hardhatDeployArguments
    );
  } else {
    console.error("This command is only available on avalanche");
  }

  console.log("Deployment complete");

  // Do some deployment validation.
  // mapping(uint32 => bool) public marketExists;
  let marketExists = await longShort.marketExists(3);
  console.log("marketExists", marketExists);
  assert.equal(marketExists, true);
  let assetPrice = await longShort.assetPrice(3);
  console.log("assetPrice", assetPrice);
  // assert.notEqual(assetPrice, 0);
  let marketUpdateIndex = await longShort.marketUpdateIndex(3);
  console.log("marketUpdateIndex", marketUpdateIndex.toString());
  // assert.notEqual(marketUpdateIndex, 1);
  let marketTreasurySplitGradient_e18 = await longShort.marketTreasurySplitGradient_e18(3);
  console.log("marketTreasurySplitGradient_e18", marketTreasurySplitGradient_e18.toString());
  let marketLeverage_e18 = await longShort.marketLeverage_e18(3);
  console.log("marketLeverage_e18", marketLeverage_e18.toString());
  let paymentTokens = await longShort.paymentTokens(3);
  console.log("paymentTokens", paymentTokens);
  let yieldManagers = await longShort.yieldManagers(3);
  console.log("yieldManagers", yieldManagers);
  let oracleManagers = await longShort.oracleManagers(3);
  console.log("oracleManagers", oracleManagers);
  let fundingRateMultiplier_e18 = await longShort.fundingRateMultiplier_e18(3);
  console.log("fundingRateMultiplier_e18", fundingRateMultiplier_e18.toString());
  let marketSideValueInPaymentToken = await longShort.marketSideValueInPaymentToken(3);
  console.log("marketSideValueInPaymentToken", marketSideValueInPaymentToken.toString());
};
module.exports.tags = ["QI"];
