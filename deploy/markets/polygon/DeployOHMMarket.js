const {
  launchOhmMarket,
} = require("../../../deployTests/PolygonTransactions");
const { ethers } = require("hardhat");

const {
  STAKER,
  COLLATERAL_TOKEN,
  TREASURY,
  LONGSHORT,
  isAlphaLaunch,
  TREASURY_ALPHA,
} = require("../../../helper-hardhat-config");

let networkToUse = network.name;

if (!!process.env.HARDHAT_FORK) {
  networkToUse = process.env.HARDHAT_FORK;
}

module.exports = async (hardhatDeployArguments) => {
  console.log("setup contracts");
  const { getNamedAccounts, deployments } = hardhatDeployArguments;
  const { admin } = await getNamedAccounts();

  console.log("All accounts", await getNamedAccounts())

  ////////////////////////
  //Retrieve Deployments//
  ////////////////////////
  console.log("1");
  let paymentTokenAddress;

  if (networkToUse == "polygon") {
    paymentTokenAddress = "0x8f3Cf7ad23Cd3CaDbD9735AFf958023239c6A063";
  } else {
    console.error("This command is only available on polygon")
  }

  const paymentToken = await ethers.getContractAt(
    COLLATERAL_TOKEN,
    paymentTokenAddress
  );

  const LongShort = await deployments.get(LONGSHORT);
  const longShort = await ethers.getContractAt(LONGSHORT, LongShort.address);

  let treasuryToUse = isAlphaLaunch ? TREASURY_ALPHA : TREASURY;
  const Treasury = await deployments.get(treasuryToUse);
  const treasury = await ethers.getContractAt(treasuryToUse, Treasury.address);

  const Staker = await deployments.get(STAKER);
  const staker = await ethers.getContractAt(STAKER, Staker.address);


  if (networkToUse == "polygon") {
    console.log("polygon test transactions");
    await launchOhmMarket(
      {
        staker,
        longShort: longShort.connect(admin),
        paymentToken,
        treasury,
      },
      hardhatDeployArguments
    );
  } else {
    console.error("This command is only available on polygon")
  }

  console.log("after test txs");
};
module.exports.tags = ["OHM"];
