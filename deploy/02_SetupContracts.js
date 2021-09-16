const { runTestTransactions } = require("../deployTests/RunTestTransactions");
const {
  runMumbaiTransactions,
} = require("../deployTests/RunMumbaiTransactions");
const {
  launchPolygonMarkets,
} = require("../deployTests/PolygonTransactions");
const { ethers } = require("hardhat");

const {
  STAKER,
  COLLATERAL_TOKEN,
  TREASURY,
  LONGSHORT,
  FLOAT_TOKEN,
  TOKEN_FACTORY,
  FLOAT_CAPITAL,
  isAlphaLaunch,
  FLOAT_TOKEN_ALPHA,
  TREASURY_ALPHA,
  GEMS,
} = require("../helper-hardhat-config");

let networkToUse = network.name;

if (!!process.env.HARDHAT_FORK) {
  networkToUse = process.env.HARDHAT_FORK;
}

module.exports = async (hardhatDeployArguments) => {
  console.log("setup contracts");
  const { getNamedAccounts, deployments } = hardhatDeployArguments;
  const { deployer, admin, discountSigner } = await getNamedAccounts();

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
    paymentTokenAddress = (await deployments.get(COLLATERAL_TOKEN)).address;
  }
  const paymentToken = await ethers.getContractAt(
    COLLATERAL_TOKEN,
    paymentTokenAddress
  );

  const Gems = await deployments.get(GEMS);
  const gems = await ethers.getContractAt(GEMS, Gems.address);

  const LongShort = await deployments.get(LONGSHORT);
  const longShort = await ethers.getContractAt(LONGSHORT, LongShort.address);

  let treasuryToUse = isAlphaLaunch ? TREASURY_ALPHA : TREASURY;
  const Treasury = await deployments.get(treasuryToUse);
  const treasury = await ethers.getContractAt(treasuryToUse, Treasury.address);

  const TokenFactory = await deployments.get(TOKEN_FACTORY);
  const tokenFactory = await ethers.getContractAt(
    TOKEN_FACTORY,
    TokenFactory.address
  );

  const Staker = await deployments.get(STAKER);
  const staker = await ethers.getContractAt(STAKER, Staker.address);
  console.log("3", longShort.address, staker.address);

  const floatTokenToUse = isAlphaLaunch ? FLOAT_TOKEN_ALPHA : FLOAT_TOKEN;
  const FloatToken = await deployments.get(floatTokenToUse);
  const floatToken = await ethers.getContractAt(
    floatTokenToUse,
    FloatToken.address
  );
  console.log("4");

  const FloatCapital = await deployments.get(FLOAT_CAPITAL);
  const floatCapital = await ethers.getContractAt(
    FLOAT_CAPITAL,
    FloatCapital.address
  );
  ///////////////////////////
  //Initialize the contracts/
  ///////////////////////////
  await longShort.initialize(
    admin,
    tokenFactory.address,
    staker.address,
    gems.address
  );
  if (isAlphaLaunch) {
    await floatToken.initialize(
      "Alpha Float",
      "alphaFLT",
      staker.address,
      treasury.address
    );
  } else {
    await floatToken.initialize("Float", "FLT", staker.address);
  }
  await staker.initialize(
    admin,
    longShort.address,
    floatToken.address,
    treasury.address,
    floatCapital.address,
    discountSigner,
    "333333333333333333", // 25% for flt (33.333/133.333 ~= 0.25)
    gems.address
  );

  await gems.initialize(admin, longShort.address, staker.address);

  if (networkToUse == "polygon") {
    console.log("polygon test transactions");
    await launchPolygonMarkets(
      {
        staker,
        longShort: longShort.connect(admin),
        paymentToken,
        treasury,
      },
      hardhatDeployArguments
    );
  } else if (networkToUse == "mumbai") {
    console.log("mumbai test transactions");
    await runMumbaiTransactions(
      {
        staker,
        longShort: longShort.connect(admin),
        paymentToken,
        treasury,
      },
      hardhatDeployArguments
    );
  } else if (networkToUse == "hardhat" || networkToUse == "ganache") {
    console.log("local test transactions");
    await runTestTransactions(
      {
        staker,
        longShort: longShort.connect(admin),
        paymentToken,
        treasury,
      },
      hardhatDeployArguments
    );
  }
  console.log("after test txs");
};
module.exports.tags = ["all", "setup"];
