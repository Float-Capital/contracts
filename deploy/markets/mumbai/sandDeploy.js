const {
  runMumbaiTransactions,
  deployNewPaymentTokenMarket,
} = require("../../../deployTests/RunMumbaiTransactions");
const { ethers } = require("hardhat");

const {
  STAKER,
  LONGSHORT,
  TREASURY_ALPHA,
} = require("../../../helper-hardhat-config");

let networkToUse = network.name;

if (!!process.env.HARDHAT_FORK) {
  networkToUse = process.env.HARDHAT_FORK;
}

module.exports = async (hardhatDeployArguments) => {
  console.log("setup contracts");
  const { getNamedAccounts, deployments } = hardhatDeployArguments;
  const accounts = await ethers.getSigners();

  const deployer = accounts[0].address;
  const admin = accounts[1].address;

  ////////////////////////
  //Retrieve Deployments//
  ////////////////////////
  console.log("1");
  let paymentTokenAddress;
  if (networkToUse != "mumbai") {
    throw new Error("not mumbai!");
  }

  const LongShort = await deployments.get(LONGSHORT);
  const longShort = await ethers.getContractAt(LONGSHORT, LongShort.address);

  const Treasury = await deployments.get(TREASURY_ALPHA);
  const treasury = await ethers.getContractAt(TREASURY_ALPHA, Treasury.address);

  const Staker = await deployments.get(STAKER);
  const staker = await ethers.getContractAt(STAKER, Staker.address);
  console.log("3", longShort.address, staker.address);

  const fakePaymentTokenContractName = "ERC20MockWithPublicMint";
  let paymentToken = await deployments.deploy(fakePaymentTokenContractName, {
    from: deployer,
    log: true,
    args: ["fake ETH token", "fETH"],
  });
  // console.log("fake eth address", paymentToken);
  paymentTokenAddress = paymentToken.address;
  let fakePaymentTokenContract = await ethers.getContractAt(
    fakePaymentTokenContractName,
    paymentTokenAddress
  );
  // A hack to get the ERC20MockWithPublicMint token to work on ganache (with test transactions - codegen doesn't account for duplicate named functions)
  //    related: https://github.com/Float-Capital/monorepo/issues/1767
  fakePaymentTokenContract["mint"] = paymentToken["mint(address,uint256)"];

  for (const account of accounts) {
    await fakePaymentTokenContract["mint(address,uint256)"](
      account.address,
      "10000000000000000000000"
    );
  };
  await deployNewPaymentTokenMarket(
    {
      staker,
      longShort: longShort.connect(admin),
      paymentToken: fakePaymentTokenContract,
      treasury,
    },
    hardhatDeployArguments
  );
};
module.exports.tags = ["sand"];
