open LetOps;
open TestnetDeployHelpers;
open Globals;
open ProtocolInteractionHelpers;

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

  let admin = loadedAccounts->Array.getUnsafe(1);
  let user1 = loadedAccounts->Array.getUnsafe(2);

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

let deployNewPaymentTokenMarket =
    (
      {longShort, staker, treasury, paymentToken},
      deploymentArgs: Hardhat.hardhatDeployArgument,
    ) => {
  let%AwaitThen namedAccounts = deploymentArgs.getNamedAccounts();
  let%AwaitThen loadedAccounts = Ethers.getSigners();

  // let deployer = loadedAccounts->Array.getUnsafe(0);
  let admin = loadedAccounts->Array.getUnsafe(1);

  let syntheticSymbol = "SAND";
  let syntheticName = "the Sandbox";

  let oraclePriceFeedAddress = "0x9dd18534b8f456557d11B9DDB14dA89b2e52e308";
  let%AwaitThen oracleManager =
    deploymentArgs.deployments
    ->Hardhat.deploy(
        ~name="OracleManager" ++ syntheticSymbol,
        ~arguments={
          "from": namedAccounts.deployer,
          "log": true,
          "contract": "OracleManagerChainlinkTestnet",
          "args": (admin.address, oraclePriceFeedAddress, bnFromInt(27)),
        },
      );

  Js.log("a.3");

  let%AwaitThen yieldManagerDeployment =
    YieldManagerMock.make(
      ~longShort=longShort.address,
      ~treasury=treasury.address,
      ~token=paymentToken.address,
    );
  let%AwaitThen yieldManager =
    Hardhat.getContractAtName(
      "YieldManagerMock",
      yieldManagerDeployment.address,
    );

  let%AwaitThen _ =
    yieldManager->YieldManagerMock.setYieldRate(
      ~yieldRate=Ethers.BigNumber.fromUnsafe("43092609840000"),
    ); // 50000000000000000÷3154e+7 (5% ÷ number of seconds in a year)

  let%AwaitThen mintRole = paymentToken->ERC20Mock.mINTER_ROLE;

  let%AwaitThen _ =
    paymentToken->ERC20Mock.grantRole(
      ~role=mintRole,
      ~account=yieldManager.address,
    );

  Js.log("a.4");
  deployTestnetMarketUpgradeableCore(
    ~syntheticName,
    ~syntheticSymbol,
    ~longShortInstance=longShort,
    ~stakerInstance=staker,
    ~admin,
    ~paymentToken,
    ~oracleManagerAddress=oracleManager.address,
    ~yieldManagerAddress=yieldManager.address,
    ~deployments=deploymentArgs.deployments,
    ~namedAccounts,
  );
};
