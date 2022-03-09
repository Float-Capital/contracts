const { network, ethers } = require("hardhat");
const { LONGSHORT } = require("../../helper-hardhat-config");

let networkToUse = network.name;

if (!!process.env.HARDHAT_FORK) {
  networkToUse = process.env.HARDHAT_FORK;
}

const oneHunderedPercentFundingRate = "1000000000000000000";
const twoHunderedPercentFundingRate = "2000000000000000000";
const threeHunderedPercentFundingRate = "3000000000000000000";
const eightHunderedPercentFundingRate = "8000000000000000000";

const pollygonConfig = [
  {
    marketIndex: 1,
    fundingRateMultiplier_e18: twoHunderedPercentFundingRate,
  },
  {
    marketIndex: 2,
    fundingRateMultiplier_e18: twoHunderedPercentFundingRate,
  },
  {
    marketIndex: 4,
    fundingRateMultiplier_e18: oneHunderedPercentFundingRate,
  },
  {
    marketIndex: 5,
    fundingRateMultiplier_e18: oneHunderedPercentFundingRate,
  },
  {
    marketIndex: 6,
    fundingRateMultiplier_e18: oneHunderedPercentFundingRate,
  },
  {
    marketIndex: 8,
    fundingRateMultiplier_e18: eightHunderedPercentFundingRate,
  },
];

const avalancheConfig = [
  {
    marketIndex: 1,
    fundingRateMultiplier_e18: oneHunderedPercentFundingRate,
  },
  {
    marketIndex: 2,
    fundingRateMultiplier_e18: twoHunderedPercentFundingRate,
  },
  {
    marketIndex: 3,
    fundingRateMultiplier_e18: twoHunderedPercentFundingRate,
  },
  {
    marketIndex: 4,
    fundingRateMultiplier_e18: twoHunderedPercentFundingRate,
  },
];

let updateFundingRate = async (
  longShortContract,
  marketIndex,
  fundingRateMultiplier_e18
) => {
  const fundingRateMultiplier =
    await longShortContract.fundingRateMultiplier_e18(marketIndex);

  if (
    fundingRateMultiplier.eq(ethers.BigNumber.from(fundingRateMultiplier_e18))
  ) {
    console.log(
      "fundingRateMultiplier set correctly for market",
      marketIndex,
      `(funding rate is ${fundingRateMultiplier_e18})`
    );
  } else {
    console.log(
      `fundingRateMultiplier incorrect for market ${marketIndex} - should be ${fundingRateMultiplier_e18} but it is currently ${fundingRateMultiplier.toString()}`
    );
    await longShortContract.changeMarketFundingRateMultiplier(
      marketIndex,
      fundingRateMultiplier_e18
    );

    const updatedFundingRate =
      await longShortContract.fundingRateMultiplier_e18(marketIndex);

    if (updatedFundingRate.toString() != fundingRateMultiplier_e18) {
      throw new Error(
        "there was a problem with the upgrade, funding rate not set correctly. current value is " +
          updatedFundingRate.toString()
      );
    }
  }
};

module.exports = async ({ deployments }) => {
  const accounts = await ethers.getSigners();
  const admin = accounts[1];

  const longShortAddress = (await deployments.get(LONGSHORT)).address;
  const longShortContract = (
    await ethers.getContractAt(LONGSHORT, longShortAddress)
  ).connect(admin);

  if (networkToUse === "polygon") {
    for ({ marketIndex, fundingRateMultiplier_e18 } of pollygonConfig) {
      await updateFundingRate(
        longShortContract,
        marketIndex,
        fundingRateMultiplier_e18
      );
    }
  } else if (networkToUse === "avalanche") {
    for ({ marketIndex, fundingRateMultiplier_e18 } of avalancheConfig) {
      await updateFundingRate(
        longShortContract,
        marketIndex,
        fundingRateMultiplier_e18
      );
    }
  } else {
    throw new Error(
      `keeper config not defined for this network (you are trying to use ${networkToUse})`
    );
  }
};

module.exports.tags = ["funding-rate"];
