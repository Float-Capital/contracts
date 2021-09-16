open LetOps;
open DeployHelpers;
open Globals;

type allContracts = {
  staker: Staker.t,
  longShort: LongShort.t,
  paymentToken: ERC20Mock.t,
  treasury: Treasury_v0.t,
  syntheticToken: SyntheticToken.t,
};

let runMumbaiTransactions =
    (
      {longShort, staker, treasury, paymentToken},
      deploymentArgs: Hardhat.hardhatDeployArgument,
    ) => {
  let%AwaitThen namedAccounts = deploymentArgs.getNamedAccounts();
  let%AwaitThen loadedAccounts = Ethers.getSigners();

  // let deployer = loadedAccounts->Array.getUnsafe(0);
  let admin = loadedAccounts->Array.getUnsafe(1);
  let user1 = loadedAccounts->Array.getUnsafe(2);
  // let user2 = loadedAccounts->Array.getUnsafe(3);
  // let user3 = loadedAccounts->Array.getUnsafe(4);

  // let%AwaitThen _ =
  //   DeployHelpers.topupBalanceIfLow(~from=deployer, ~to_=admin);
  // let%AwaitThen _ =
  //   DeployHelpers.topupBalanceIfLow(~from=deployer, ~to_=user1);
  // let%AwaitThen _ =
  //   DeployHelpers.topupBalanceIfLow(~from=deployer, ~to_=user2);
  // let%AwaitThen _ =
  //   DeployHelpers.topupBalanceIfLow(~from=deployer, ~to_=user3);

  Js.log("deploying markets");

  let%AwaitThen _ =
    deployMumbaiMarketUpgradeable(
      ~syntheticName="ETH Market",
      ~syntheticSymbol="ETH",
      ~longShortInstance=longShort,
      ~treasuryInstance=treasury,
      ~stakerInstance=staker,
      ~deployments=deploymentArgs.deployments,
      ~namedAccounts,
      ~admin,
      ~paymentToken: ERC20Mock.t,
      ~oraclePriceFeedAddress=ChainlinkOracleAddresses.Mumbai.ethOracleChainlink,
    );
  let%AwaitThen _ =
    deployMumbaiMarketUpgradeable(
      ~syntheticName="MATIC Market",
      ~syntheticSymbol="MATIC",
      ~longShortInstance=longShort,
      ~treasuryInstance=treasury,
      ~stakerInstance=staker,
      ~deployments=deploymentArgs.deployments,
      ~namedAccounts,
      ~admin,
      ~paymentToken: ERC20Mock.t,
      ~oraclePriceFeedAddress=ChainlinkOracleAddresses.Mumbai.maticOracleChainlink,
    );
  let%AwaitThen _ =
    deployMumbaiMarketUpgradeable(
      ~syntheticName="BTC Market",
      ~syntheticSymbol="BTC",
      ~longShortInstance=longShort,
      ~treasuryInstance=treasury,
      ~stakerInstance=staker,
      ~deployments=deploymentArgs.deployments,
      ~namedAccounts,
      ~admin,
      ~paymentToken: ERC20Mock.t,
      ~oraclePriceFeedAddress=ChainlinkOracleAddresses.Mumbai.btcOracleChainlink,
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
