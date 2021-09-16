open LetOps;
open DeployHelpers;
open Globals;

let ethMarketCapOraclePriceFeedAddress =
  "0x67935f65D1577ced9f4929D3679A157E95C1c02c"->Ethers.Utils.getAddressUnsafe;
let btcMarketCapOraclePriceFeedAddress =
  "0x18E4058491C3F58bC2f747A9E64cA256Ed6B318d"->Ethers.Utils.getAddressUnsafe;

type allContracts = {
  staker: Staker.t,
  longShort: LongShort.t,
  paymentToken: ERC20Mock.t,
  treasury: Treasury_v0.t,
  syntheticToken: SyntheticToken.t,
};

let launchPolygonMarkets =
    (
      {longShort, staker, treasury, paymentToken},
      deploymentArgs: Hardhat.hardhatDeployArgument,
    ) => {
  let%AwaitThen namedAccounts = deploymentArgs.getNamedAccounts();
  let%AwaitThen loadedAccounts = Ethers.getSigners();

  let admin = loadedAccounts->Array.getUnsafe(1);

  Js.log("deploying markets");

  deployFlipp3ningPolygon(
    ~longShortInstance=longShort,
    ~treasuryInstance=treasury,
    ~stakerInstance=staker,
    ~deployments=deploymentArgs.deployments,
    ~namedAccounts,
    ~admin,
    ~paymentToken: ERC20Mock.t,
    ~ethMarketCapOraclePriceFeedAddress,
    ~btcMarketCapOraclePriceFeedAddress,
  );
};
