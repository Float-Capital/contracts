const { network } = require("hardhat");
const {
  STAKER,
  TEST_COLLATERAL_TOKEN,
  TREASURY,
  TREASURY_ALPHA,
  LONGSHORT,
  FLOAT_TOKEN,
  FLOAT_TOKEN_ALPHA,
  TOKEN_FACTORY,
  FLOAT_CAPITAL,
  GEMS,
  isAlphaLaunch,
} = require("../helper-hardhat-config");
const mumbaiDaiAddress = "0x001B3B4d0F3714Ca98ba10F6042DaEbF0B1B7b6F";
const polygonDaiAddress = "0x8f3Cf7ad23Cd3CaDbD9735AFf958023239c6A063";
const avalancheDaiAddress = "0xd586E7F844cEa2F87f50152665BCbc2C279D8d70";
const ftmTestnetDaiAddress = "0x5343b5bA672Ae99d627A1C87866b8E53F47Db2E6";

let networkToUse = network.name;

if (!!process.env.HARDHAT_FORK) {
  networkToUse = process.env.HARDHAT_FORK;
}

module.exports = async ({ getNamedAccounts, deployments }) => {
  const { deploy } = deployments;
  // const { deployer, admin } = await getNamedAccounts();
  const accounts = await ethers.getSigners();

  const deployer = accounts[0].address;
  const admin = accounts[1].address;

  let paymentTokenAddress;

  if (networkToUse === "polygon") {
    paymentTokenAddress = polygonDaiAddress;
  } else if (networkToUse === "avalanche") {
    paymentTokenAddress = avalancheDaiAddress;
  } else if (networkToUse === "mumbai") {
    paymentTokenAddress = mumbaiDaiAddress;
  } else if (networkToUse === "fantom-testnet") {
    paymentTokenAddress = ftmTestnetDaiAddress;
  } else {
    console.log(networkToUse, TEST_COLLATERAL_TOKEN);
    let paymentToken = await deploy(TEST_COLLATERAL_TOKEN, {
      from: deployer,
      log: true,
      args: ["dai token", "DAI"],
    });
    console.log("dai address", paymentToken.address);
    paymentTokenAddress = paymentToken.address;
    let paymentTokenContract = await ethers.getContractAt(
      TEST_COLLATERAL_TOKEN,
      paymentTokenAddress
    );

    accounts.map((account) => {
      paymentTokenContract["mint(address,uint256)"](
        account.address,
        "10000000000000000000000"
      );
    });
  }

  console.log("Deploying contracts with the account:", deployer);
  console.log("Admin Account:", admin);

  const floatTokenToUse = isAlphaLaunch ? FLOAT_TOKEN_ALPHA : FLOAT_TOKEN;
  let floatToken = await deploy(floatTokenToUse, {
    from: deployer,
    log: true,
    proxy: {
      proxyContract: "UUPSProxy",
      initializer: false,
    },
  });

  await deploy(FLOAT_CAPITAL, {
    from: deployer,
    proxy: {
      proxyContract: "UUPSProxy",
      execute: {
        methodName: "initialize",
        args: [admin],
      },
    },
    log: true,
  });

  const gems = await deploy(GEMS, {
    from: deployer,
    log: true,
    proxy: {
      proxyContract: "UUPSProxy",
      initializer: false,
    },
  });
  console.log("Gems", gems.address);

  const staker = await deploy(STAKER, {
    from: deployer,
    log: true,
    proxy: {
      proxyContract: "UUPSProxy",
      initializer: false,
    },
  });

  const longShort = await deploy(LONGSHORT, {
    from: deployer,
    log: true,
    proxy: {
      proxyContract: "UUPSProxy",
      initializer: false,
    },
  });
  console.log("StakerDeployed", staker.address);
  console.log("LongShortDeployed", longShort.address);

  await deploy(TOKEN_FACTORY, {
    from: admin,
    log: true,
    args: [longShort.address],
  });

  let treasuryToUse = isAlphaLaunch ? TREASURY_ALPHA : TREASURY;
  await deploy(treasuryToUse, {
    from: deployer,
    proxy: {
      proxyContract: "UUPSProxy",
      execute: {
        methodName: "initialize",
        args: [
          admin,
          paymentTokenAddress,
          floatToken.address,
          longShort.address,
        ],
      },
    },
    log: true,
  });
};
module.exports.tags = ["all", "contracts"];
