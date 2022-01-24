const {
  launch3thMarket,
} = require("../../../deployTests/PolygonTransactions");
const { ethers } = require("hardhat");

const {
  STAKER,
  TEST_COLLATERAL_TOKEN,
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

  ////////////////////////
  //Retrieve Deployments//
  ////////////////////////
  console.log("1");
  let paymentTokenAddress;
  if (networkToUse == "polygon") {
    paymentTokenAddress = "0x8f3Cf7ad23Cd3CaDbD9735AFf958023239c6A063";
  } else if (networkToUse == "mumbai") {
    paymentTokenAddress = "0x001B3B4d0F3714Ca98ba10F6042DaEbF0B1B7b6F";
  } else if (networkToUse == "hardhat" || networkToUse == "ganache") {
    paymentTokenAddress = (await deployments.get(TEST_COLLATERAL_TOKEN)).address;
  }
  const paymentToken = await ethers.getContractAt(
    TEST_COLLATERAL_TOKEN,
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
    await launch3thMarket(
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
module.exports.tags = ["3th"];
