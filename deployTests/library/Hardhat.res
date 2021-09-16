type namedAccounts = {
  admin: Ethers.ethAddress,
  deployer: Ethers.ethAddress,
  user1: Ethers.ethAddress,
  user2: Ethers.ethAddress,
  user3: Ethers.ethAddress,
}

type createdDeployment = {
  address: Ethers.ethAddress,
  // abi,
  // transactionHash,
  // receipt,
  // args,
  // solcInputHash,
  // metadata,
  // bytecode,
  // deployedBytecode,
  // implementation,
  // devdoc,
  // userdoc,
  // storageLayout,
  // newlyDeployed
}
type deployments_t

@send
external deploy: (deployments_t, ~name: string, ~arguments: 'a) => JsPromise.t<createdDeployment> =
  "deploy"

type contractInstance
@send
external get: (deployments_t, ~name: string) => JsPromise.t<contractInstance> = "get"

type hardhatDeployArgument = {
  getNamedAccounts: unit => JsPromise.t<namedAccounts>,
  deployments: deployments_t,
}

@module("hardhat") @scope("ethers")
external getContractAt: (contractInstance, Ethers.ethAddress) => JsPromise.t<'a> = "getContractAt"
