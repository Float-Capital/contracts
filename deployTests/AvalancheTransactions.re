open LetOps;
open DeployHelpers;

// SOURCE: https://docs.chain.link/docs/avalanche-price-feeds/
let avaxOraclePriceFeedAddress =
  "0x0A77230d17318075983913bC2145DB16C7366156"->Ethers.Utils.getAddressUnsafe;
// https://snowtrace.io/address/0x02D35d3a8aC3e1626d3eE09A78Dd87286F5E8e3a
let joeOraclePriceFeedAddress =
  "0x02D35d3a8aC3e1626d3eE09A78Dd87286F5E8e3a"->Ethers.Utils.getAddressUnsafe;
// https://snowtrace.io/address/0x36E039e6391A5E7A7267650979fdf613f659be5D
let qiOraclePriceFeedAddress =
  "0x36E039e6391A5E7A7267650979fdf613f659be5D"->Ethers.Utils.getAddressUnsafe;

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
  let syntheticName = "Avalanche2";
  let syntheticSymbol = "Avax2";

  deployAvalancheMarket(
    ~longShortInstance=longShort,
    ~treasuryInstance=treasury,
    ~stakerInstance=staker,
    ~deployments=deploymentArgs.deployments,
    ~namedAccounts,
    ~admin,
    ~paymentToken: ERC20Mock.t,
    ~oraclePriceFeedAddress=avaxOraclePriceFeedAddress,
    ~syntheticName,
    ~syntheticSymbol,
    ~fundingRateMultiplier=CONSTANTS.tenToThe18,
    ~marketLeverage=2,
    ~expectedMarketIndex=1,
    ~yieldManagerVariant=AaveDAI,
  );
};

let launchJoeMarket =
    (
      {longShort, staker, treasury, paymentToken},
      deploymentArgs: Hardhat.hardhatDeployArgument,
    ) => {
  let%AwaitThen namedAccounts = deploymentArgs.getNamedAccounts();
  let%AwaitThen loadedAccounts = Ethers.getSigners();

  let admin = loadedAccounts->Array.getUnsafe(1);

  Js.log("deploying markets");
  let syntheticName = "JOE 2x";
  let syntheticSymbol = "2JOE";

  deployAvalancheMarket(
    ~longShortInstance=longShort,
    ~treasuryInstance=treasury,
    ~stakerInstance=staker,
    ~deployments=deploymentArgs.deployments,
    ~namedAccounts,
    ~admin,
    ~paymentToken: ERC20Mock.t,
    ~oraclePriceFeedAddress=joeOraclePriceFeedAddress,
    ~syntheticName,
    ~syntheticSymbol,
    ~fundingRateMultiplier=CONSTANTS.twoTimesTenToThe18,
    ~marketLeverage=2,
    ~expectedMarketIndex=2,
    ~yieldManagerVariant=AaveDAI,
  );
};

let launchQiMarket =
    (
      {longShort, staker, treasury, paymentToken},
      deploymentArgs: Hardhat.hardhatDeployArgument,
    ) => {
  let%AwaitThen namedAccounts = deploymentArgs.getNamedAccounts();
  let%AwaitThen loadedAccounts = Ethers.getSigners();

  let admin = loadedAccounts->Array.getUnsafe(1);

  // TODO: unpack the funding rate and multiplier
  Js.log("deploying markets");
  let syntheticName = "QI 2x";
  let syntheticSymbol = "2QI";

  deployAvalancheMarket(
    ~longShortInstance=longShort,
    ~treasuryInstance=treasury,
    ~stakerInstance=staker,
    ~deployments=deploymentArgs.deployments,
    ~namedAccounts,
    ~admin,
    ~paymentToken: ERC20Mock.t,
    ~oraclePriceFeedAddress=qiOraclePriceFeedAddress,
    ~syntheticName,
    ~syntheticSymbol,
    ~fundingRateMultiplier=CONSTANTS.twoTimesTenToThe18,
    ~marketLeverage=2,
    ~expectedMarketIndex=3,
    ~yieldManagerVariant=BenqiDAI,
  );
};
