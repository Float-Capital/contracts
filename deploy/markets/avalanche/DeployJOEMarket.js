const { launchJoeMarket } = require("../../../deployTests/AvalancheTransactions");
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

let networkToUse = network.name;

if (!!process.env.HARDHAT_FORK) {
  networkToUse = process.env.HARDHAT_FORK;
}

module.exports = async (hardhatDeployArguments) => {
  console.log("setup contracts");
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
    console.log("avalanche test transactions");
    await launchJoeMarket(
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
};
module.exports.tags = ["JOE"];
