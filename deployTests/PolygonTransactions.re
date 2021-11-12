open LetOps;
open DeployHelpers;

// SOURCE: https://docs.chain.link/docs/matic-addresses/
let ethMarketCapOraclePriceFeedAddress =
  "0x67935f65D1577ced9f4929D3679A157E95C1c02c"->Ethers.Utils.getAddressUnsafe;
let btcMarketCapOraclePriceFeedAddress =
  "0x18E4058491C3F58bC2f747A9E64cA256Ed6B318d"->Ethers.Utils.getAddressUnsafe;
// https://polygonscan.com/address/0xF9680D99D6C9589e2a93a78A04A279e509205945
let ethUSDPriceFeedAddress =
  "0xF9680D99D6C9589e2a93a78A04A279e509205945"->Ethers.Utils.getAddressUnsafe;
// https://polygonscan.com/address/0xa8B05B6337040c0529919BDB51f6B40A684eb08C
let ohmUSDPriceFeedAddress =
  "0xa8B05B6337040c0529919BDB51f6B40A684eb08C"->Ethers.Utils.getAddressUnsafe;

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
let launch3thMarket =
    (
      {longShort, staker, treasury, paymentToken},
      deploymentArgs: Hardhat.hardhatDeployArgument,
    ) => {
  let%AwaitThen namedAccounts = deploymentArgs.getNamedAccounts();
  let%AwaitThen loadedAccounts = Ethers.getSigners();

  let admin = loadedAccounts->Array.getUnsafe(1);

  Js.log("deploying markets");

  deploy3TH_Polygon(
    ~longShortInstance=longShort,
    ~treasuryInstance=treasury,
    ~stakerInstance=staker,
    ~deployments=deploymentArgs.deployments,
    ~namedAccounts,
    ~admin,
    ~paymentToken: ERC20Mock.t,
    ~ethUSDPriceFeedAddress,
  );
};
let launchOhmMarket =
    (
      {longShort, staker, treasury, paymentToken},
      deploymentArgs: Hardhat.hardhatDeployArgument,
    ) => {
  let%AwaitThen namedAccounts = deploymentArgs.getNamedAccounts();
  let%AwaitThen loadedAccounts = Ethers.getSigners();

  let admin = loadedAccounts->Array.getUnsafe(1);

  Js.log("deploying markets");

  let syntheticName = "OHM 2x";
  let syntheticSymbol = "2OHM";
  let leverageAmount = 2;

  deployMarketOnPolygon(
    ~longShortInstance=longShort,
    ~treasuryInstance=treasury,
    ~stakerInstance=staker,
    ~deployments=deploymentArgs.deployments,
    ~namedAccounts,
    ~admin,
    ~paymentToken: ERC20Mock.t,
    ~ohmUSDPriceFeedAddress,
    ~syntheticName,
    ~syntheticSymbol,
    ~leverageAmount,
  );
};
