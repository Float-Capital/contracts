open LetOps;
open TestnetDeployHelpers;
open Globals;
open ProtocolInteractionHelpers;

let btcUSDPriceFeedAddress =
  "0x65E8d79f3e8e36fE48eC31A2ae935e92F5bBF529"->Ethers.Utils.getAddressUnsafe;
// https://testnet.ftmscan.com/address/0x65E8d79f3e8e36fE48eC31A2ae935e92F5bBF529
let ethUSDPriceFeedAddress =
  "0xB8C458C957a6e6ca7Cc53eD95bEA548c52AFaA24"->Ethers.Utils.getAddressUnsafe;
// https://testnet.ftmscan.com/address/0xB8C458C957a6e6ca7Cc53eD95bEA548c52AFaA24
let fantomUsdPriceFeedAddress =
  "0xe04676B9A9A2973BCb0D1478b5E1E9098BBB7f3D"->Ethers.Utils.getAddressUnsafe;
// https://testnet.ftmscan.com/address/0xe04676B9A9A2973BCb0D1478b5E1E9098BBB7f3D

type allContracts = {
  staker: Staker.t,
  longShort: LongShort.t,
  paymentToken: ERC20Mock.t,
  treasury: Treasury_v0.t,
  syntheticToken: SyntheticToken.t,
};

let runFantomTestnetTransactions =
    (
      {longShort, staker, treasury, paymentToken},
      deploymentArgs: Hardhat.hardhatDeployArgument,
    ) => {
  let%AwaitThen namedAccounts = deploymentArgs.getNamedAccounts();
  let%AwaitThen loadedAccounts = Ethers.getSigners();

  let admin = loadedAccounts->Array.getUnsafe(1);
  let user1 = loadedAccounts->Array.getUnsafe(2);

  Js.log("deploying markets");

  let%AwaitThen _ =
    deployFantomTestnetMarketUpgradeable(
      ~syntheticName="ETH Market",
      ~syntheticSymbol="ETH",
      ~longShortInstance=longShort,
      ~treasuryInstance=treasury,
      ~stakerInstance=staker,
      ~deployments=deploymentArgs.deployments,
      ~namedAccounts,
      ~admin,
      ~paymentToken: ERC20Mock.t,
      ~oraclePriceFeedAddress=ethUSDPriceFeedAddress,
    );
  let%AwaitThen _ =
    deployFantomTestnetMarketUpgradeable(
      ~syntheticName="FANTOM Market",
      ~syntheticSymbol="FTM",
      ~longShortInstance=longShort,
      ~treasuryInstance=treasury,
      ~stakerInstance=staker,
      ~deployments=deploymentArgs.deployments,
      ~namedAccounts,
      ~admin,
      ~paymentToken: ERC20Mock.t,
      ~oraclePriceFeedAddress=fantomUsdPriceFeedAddress,
    );
  let%AwaitThen _ =
    deployFantomTestnetMarketUpgradeable(
      ~syntheticName="BTC Market",
      ~syntheticSymbol="BTC",
      ~longShortInstance=longShort,
      ~treasuryInstance=treasury,
      ~stakerInstance=staker,
      ~deployments=deploymentArgs.deployments,
      ~namedAccounts,
      ~admin,
      ~paymentToken: ERC20Mock.t,
      ~oraclePriceFeedAddress=btcUSDPriceFeedAddress,
    );
  let initialMarkets = [|1, 2, 3|];
  let longMintAmount = bnFromString("10000000000000000000");
  let shortMintAmount = longMintAmount->div(bnFromInt(2));
  let redeemShortAmount = shortMintAmount->div(bnFromInt(2));
  let longStakeAmount = longMintAmount->div(twoBn);
  Js.log("Executing Long Mints");
  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      mintNextPrice(
        ~amount=longMintAmount,
        ~marketIndex=_,
        ~paymentToken,
        ~longShort,
        ~user=user1,
        ~isLong=true,
      ),
    );
  Js.log("Executing Short Mints");
  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      mintNextPrice(
        ~amount=longMintAmount,
        ~marketIndex=_,
        ~paymentToken,
        ~longShort,
        ~user=user1,
        ~isLong=false,
      ),
    );
  let%AwaitThen _ = sleep(~timeMs=27000);
  Js.log("Executing Short Position Redeem");
  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      redeemNextPrice(
        ~amount=redeemShortAmount,
        ~marketIndex=_,
        ~longShort,
        ~user=user1,
        ~isLong=false,
      ),
    );
  let%AwaitThen _ = sleep(~timeMs=27000);
  Js.log("Staking long position");
  executeOnMarkets(
    initialMarkets,
    stakeSynthLong(
      ~amount=longStakeAmount,
      ~longShort,
      ~marketIndex=_,
      ~user=user1,
    ),
  );
};
