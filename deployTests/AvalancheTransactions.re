open LetOps;
open DeployHelpers;

// SOURCE: https://docs.chain.link/docs/avalanche-price-feeds/
let avaxOraclePriceFeedAddress =
  "0x0A77230d17318075983913bC2145DB16C7366156"->Ethers.Utils.getAddressUnsafe;

type allContracts = {
  staker: Staker.t,
  longShort: LongShort.t,
  paymentToken: ERC20Mock.t,
  treasury: Treasury_v0.t,
  syntheticToken: SyntheticToken.t,
};

let launchAvaxMarket =
    (
      {longShort, staker, treasury, paymentToken},
      deploymentArgs: Hardhat.hardhatDeployArgument,
    ) => {
  let%AwaitThen namedAccounts = deploymentArgs.getNamedAccounts();
  let%AwaitThen loadedAccounts = Ethers.getSigners();

  let admin = loadedAccounts->Array.getUnsafe(1);

  Js.log("deploying markets");

  deployAvax_Avalanche(
    ~longShortInstance=longShort,
    ~treasuryInstance=treasury,
    ~stakerInstance=staker,
    ~deployments=deploymentArgs.deployments,
    ~namedAccounts,
    ~admin,
    ~paymentToken: ERC20Mock.t,
    ~avaxOraclePriceFeedAddress,
  );
};
