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
// https://polygonscan.com/address/0x9c371aE34509590E10aB98205d2dF5936A1aD875
let axsUSDPriceFeedAddress =
  "0x9c371aE34509590E10aB98205d2dF5936A1aD875"->Ethers.Utils.getAddressUnsafe;
// https://polygonscan.com/address/0xB527769842f997d56dD1Ff73C34192141b69077e
let cviFeedAddress =
  "0xB527769842f997d56dD1Ff73C34192141b69077e"->Ethers.Utils.getAddressUnsafe;
// https://polygonscan.com/address/0xd9FFdb71EbE7496cC440152d43986Aae0AB76665
let linkFeedAddress =
  "0xd9FFdb71EbE7496cC440152d43986Aae0AB76665"->Ethers.Utils.getAddressUnsafe;
// https://polygonscan.com/address/0x4cE90F28C6357A7d3F47D680723d18AF3684cD00
let ohmV2FeedAddress =
  "0x4cE90F28C6357A7d3F47D680723d18AF3684cD00"->Ethers.Utils.getAddressUnsafe;

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
    ~chainlinkOricleFeedAddress=ohmUSDPriceFeedAddress,
    ~syntheticName,
    ~syntheticSymbol,
    ~leverageAmount,
    ~expectedMarketIndex=3,
    ~fundingRateMultiplier_e18=
      Ethers.BigNumber.fromUnsafe("3000000000000000000"),
  );
};

let launchAxsMarket =
    (
      {longShort, staker, treasury, paymentToken},
      deploymentArgs: Hardhat.hardhatDeployArgument,
    ) => {
  let%AwaitThen namedAccounts = deploymentArgs.getNamedAccounts();
  let%AwaitThen loadedAccounts = Ethers.getSigners();

  let admin = loadedAccounts->Array.getUnsafe(1);

  Js.log("deploying markets");

  let syntheticName = "AXS 2x";
  let syntheticSymbol = "2AXS";
  let leverageAmount = 2;

  deployMarketOnPolygon(
    ~longShortInstance=longShort,
    ~treasuryInstance=treasury,
    ~stakerInstance=staker,
    ~deployments=deploymentArgs.deployments,
    ~namedAccounts,
    ~admin,
    ~paymentToken: ERC20Mock.t,
    ~chainlinkOricleFeedAddress=axsUSDPriceFeedAddress,
    ~syntheticName,
    ~syntheticSymbol,
    ~leverageAmount,
    ~expectedMarketIndex=4,
    ~fundingRateMultiplier_e18=CONSTANTS.tenToThe18,
  );
};

let launchGeneralMarket =
    (
      ~syntheticName,
      ~syntheticSymbol,
      ~leverageAmount,
      ~oracleAddress,
      ~expectedMarketIndex,
      ~longShort,
      ~staker,
      ~treasury,
      ~paymentToken,
      ~fundingRateMultiplier,
      deploymentArgs: Hardhat.hardhatDeployArgument,
    ) => {
  let%AwaitThen namedAccounts = deploymentArgs.getNamedAccounts();
  let%AwaitThen loadedAccounts = Ethers.getSigners();

  let admin = loadedAccounts->Array.getUnsafe(1);

  deployMarketOnPolygon(
    ~longShortInstance=longShort,
    ~treasuryInstance=treasury,
    ~stakerInstance=staker,
    ~deployments=deploymentArgs.deployments,
    ~namedAccounts,
    ~admin,
    ~paymentToken: ERC20Mock.t,
    ~chainlinkOricleFeedAddress=oracleAddress,
    ~syntheticName,
    ~syntheticSymbol,
    ~leverageAmount,
    ~expectedMarketIndex,
    ~fundingRateMultiplier_e18=fundingRateMultiplier,
  );
};

let launchCviMarket =
    (
      {longShort, staker, treasury, paymentToken},
      deploymentArgs: Hardhat.hardhatDeployArgument,
    ) =>
  launchGeneralMarket(
    ~syntheticName="CVI 2x",
    ~syntheticSymbol="2CVI",
    ~leverageAmount=2,
    ~oracleAddress=cviFeedAddress,
    ~expectedMarketIndex=5,
    ~longShort,
    ~staker,
    ~treasury,
    ~paymentToken,
    ~fundingRateMultiplier=CONSTANTS.tenToThe18,
    deploymentArgs: Hardhat.hardhatDeployArgument,
  );

let launchLinkMarket =
    (
      {longShort, staker, treasury, paymentToken},
      deploymentArgs: Hardhat.hardhatDeployArgument,
    ) =>
  launchGeneralMarket(
    ~syntheticName="LINK 2x",
    ~syntheticSymbol="2LINK",
    ~leverageAmount=2,
    ~oracleAddress=linkFeedAddress,
    ~expectedMarketIndex=6,
    ~longShort,
    ~staker,
    ~treasury,
    ~paymentToken,
    ~fundingRateMultiplier=CONSTANTS.tenToThe18,
    deploymentArgs: Hardhat.hardhatDeployArgument,
  );

let launchOhmV2Market =
    (
      {longShort, staker, treasury, paymentToken},
      deploymentArgs: Hardhat.hardhatDeployArgument,
    ) =>
  launchGeneralMarket(
    ~syntheticName="OHMv2 2x",
    ~syntheticSymbol="2OHMv2",
    ~leverageAmount=2,
    ~oracleAddress=ohmV2FeedAddress,
    ~expectedMarketIndex=8,
    ~longShort,
    ~staker,
    ~treasury,
    ~paymentToken,
    ~fundingRateMultiplier=
      Ethers.BigNumber.fromInt(8)
      ->Ethers.BigNumber.mul(CONSTANTS.tenToThe18),
    deploymentArgs: Hardhat.hardhatDeployArgument,
  );
