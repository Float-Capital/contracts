const { GEMS } = require("../helper-hardhat-config");

module.exports = async ({ getNamedAccounts, deployments }) => {
  const { deploy } = deployments;
  // const { deployer, admin } = await getNamedAccounts();
  const accounts = await ethers.getSigners();

  const deployer = accounts[0].address;

  console.log("Deploying contracts with the account:", deployer);
  console.log(deployer);
  const gems = await deploy(GEMS, {
    from: deployer,
    log: true,
    proxy: {
      proxyContract: "UUPSProxy",
      initializer: false,
    },
  });
  console.log("Gems", gems.address);
};
module.exports.tags = ["gems"];
