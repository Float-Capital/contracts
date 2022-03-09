open LetOps;
open DeployHelpers;

// CHAINLINK FEEDS:
//     https://docs.chain.link/docs/avalanche-price-feeds/

// SOURCE: https://docs.chain.link/docs/avalanche-price-feeds/
let avaxOraclePriceFeedAddress =
  "0x0A77230d17318075983913bC2145DB16C7366156"->Ethers.Utils.getAddressUnsafe;
// https://snowtrace.io/address/0x02D35d3a8aC3e1626d3eE09A78Dd87286F5E8e3a
let joeOraclePriceFeedAddress =
  "0x02D35d3a8aC3e1626d3eE09A78Dd87286F5E8e3a"->Ethers.Utils.getAddressUnsafe;
// https://snowtrace.io/address/0x36E039e6391A5E7A7267650979fdf613f659be5D
let qiOraclePriceFeedAddress =
  "0x36E039e6391A5E7A7267650979fdf613f659be5D"->Ethers.Utils.getAddressUnsafe;
// https://snowtrace.io/address/0x4F3ddF9378a4865cf4f28BE51E10AECb83B7daeE
let spellOraclePriceFeedAddress =
  "0x4F3ddF9378a4865cf4f28BE51E10AECb83B7daeE"->Ethers.Utils.getAddressUnsafe;

// qiDAI
// https://docs.benqi.fi/contracts
let benqiDaiCToken = "0x835866d37AFB8CB8F8334dCCdaf66cf01832Ff5D";
// jDAI
// https://docs.traderjoexyz.com/main/security-and-contracts/contracts
let joeDaiCToken = "0xc988c170d0E38197DC634A45bF00169C7Aa7CA19";

type allContracts = {
  staker: Staker.t,
  longShort: LongShort.t,
  paymentToken: ERC20Mock.t,
  treasury: Treasury_v0.t,
  syntheticToken: SyntheticToken.t,
  expectedMarketIndex: int,
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
    ~yieldManagerVariant=CompoundDAI(benqiDaiCToken),
  );
};

let launchSpellMarket =
    (
      {longShort, staker, treasury, paymentToken, expectedMarketIndex},
      deploymentArgs: Hardhat.hardhatDeployArgument,
    ) => {
  let%AwaitThen namedAccounts = deploymentArgs.getNamedAccounts();
  let%AwaitThen loadedAccounts = Ethers.getSigners();

  let admin = loadedAccounts->Array.getUnsafe(1);

  // TODO: unpack the funding rate and multiplier
  Js.log("deploying markets");
  let syntheticName = "SPELL 2x";
  let syntheticSymbol = "2SPELL";

  deployAvalancheMarket(
    ~longShortInstance=longShort,
    ~treasuryInstance=treasury,
    ~stakerInstance=staker,
    ~deployments=deploymentArgs.deployments,
    ~namedAccounts,
    ~admin,
    ~paymentToken: ERC20Mock.t,
    ~oraclePriceFeedAddress=spellOraclePriceFeedAddress,
    ~syntheticName,
    ~syntheticSymbol,
    ~fundingRateMultiplier=CONSTANTS.twoTimesTenToThe18,
    ~marketLeverage=2,
    ~expectedMarketIndex,
    ~yieldManagerVariant=CompoundDAI(joeDaiCToken),
  );
};
