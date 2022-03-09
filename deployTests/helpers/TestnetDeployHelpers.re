open Ethers;
open LetOps;
open Globals;
open ProtocolInteractionHelpers;

let deployTestMarketCore =
    (
      ~syntheticName,
      ~syntheticSymbol,
      ~longShortInstance: LongShort.t,
      ~admin,
      ~paymentToken: ERC20Mock.t,
      ~oracleManagerAddress,
      ~yieldManagerAddress,
    ) => {
  let%AwaitThen _ =
    longShortInstance
    ->ContractHelpers.connect(~address=admin)
    ->LongShort.createNewSyntheticMarket(
        ~syntheticName,
        ~syntheticSymbol,
        ~paymentToken=paymentToken.address,
        ~oracleManager=oracleManagerAddress,
        ~yieldManager=yieldManagerAddress,
      );

  let%AwaitThen _ = Helpers.increaseTime(2);

  let%AwaitThen latestMarket = longShortInstance->LongShort.latestMarket;
  let kInitialMultiplier = bnFromString("1000000000000000000"); // 5x
  let kPeriod = bnFromInt(0); // 10 days

  let%AwaitThen _ =
    mintAndApprove(
      ~paymentToken,
      ~amount=bnFromString("2000000000000000000"),
      ~user=admin,
      ~approvedAddress=longShortInstance.address,
    );

  let unstakeFee_e18 = bnFromString("5000000000000000"); // 50 basis point unstake fee
  let initialMarketSeedForEachMarketSide =
    bnFromString("1000000000000000000");

  longShortInstance
  ->ContractHelpers.connect(~address=admin)
  ->LongShort.initializeMarket(
      ~marketIndex=latestMarket,
      ~kInitialMultiplier,
      ~kPeriod,
      ~unstakeFee_e18, // 50 basis point unstake fee
      ~initialMarketSeedForEachMarketSide,
      ~balanceIncentiveCurve_exponent=bnFromInt(5),
      ~balanceIncentiveCurve_equilibriumOffset=bnFromInt(0),
      ~marketTreasurySplitGradient_e18=bnFromInt(1),
      ~marketLeverage=CONSTANTS.tenToThe18,
    );
};

let deployTestMarket =
    (
      ~syntheticName,
      ~syntheticSymbol,
      ~longShortInstance: LongShort.t,
      ~treasuryInstance: TreasuryAlpha.t,
      ~admin,
      ~paymentToken: ERC20Mock.t,
    ) => {
  let%AwaitThen oracleManager =
    OracleManagerMock.make(
      ~admin=admin.address,
      ~maxUpdateIntervalSeconds=bnFromInt(2),
    );

  let%AwaitThen yieldManager =
    YieldManagerMock.make(
      ~longShort=longShortInstance.address,
      ~treasury=treasuryInstance.address,
      ~token=paymentToken.address,
    );

  let%AwaitThen mintRole = paymentToken->ERC20Mock.mINTER_ROLE;

  let%AwaitThen _ =
    paymentToken->ERC20Mock.grantRole(
      ~role=mintRole,
      ~account=yieldManager.address,
    );
  deployTestMarketCore(
    ~syntheticName,
    ~syntheticSymbol,
    ~longShortInstance,
    ~admin,
    ~paymentToken,
    ~oracleManagerAddress=oracleManager.address,
    ~yieldManagerAddress=yieldManager.address,
  );
};

let deployTestnetMarketUpgradeableCore =
    (
      ~syntheticName,
      ~syntheticSymbol,
      ~longShortInstance: LongShort.t,
      ~stakerInstance: Staker.t,
      ~admin,
      ~paymentToken: ERC20Mock.t,
      ~oracleManagerAddress,
      ~yieldManagerAddress,
      ~deployments: Hardhat.deployments_t,
      ~namedAccounts: Hardhat.namedAccounts,
    ) => {
  let%AwaitThen latestMarket = longShortInstance->LongShort.latestMarket;
  let newMarketIndex = latestMarket + 1;

  let%AwaitThen syntheticTokenShort =
    deployments->Hardhat.deploy(
      ~name="SS" ++ syntheticSymbol,
      ~arguments={
        "contract": "SyntheticTokenUpgradeable",
        "from": namedAccounts.deployer,
        "log": true,
        "proxy": {
          "proxyContract": "UUPSProxy",
          "execute": {
            "methodName": "initialize",
            "args": (
              "Float Short " ++ syntheticName,
              "f↗️" ++ syntheticSymbol,
              longShortInstance.address,
              stakerInstance.address,
              newMarketIndex,
              false,
            ),
          },
        },
      },
    );
  let%AwaitThen syntheticTokenLong =
    deployments->Hardhat.deploy(
      ~name="SL" ++ syntheticSymbol,
      ~arguments={
        "from": namedAccounts.deployer,
        "log": true,
        "contract": "SyntheticTokenUpgradeable",
        "proxy": {
          "proxyContract": "UUPSProxy",
          "execute": {
            "args": (
              "Float Long " ++ syntheticName,
              "f↘️" ++ syntheticSymbol,
              longShortInstance.address,
              stakerInstance.address,
              newMarketIndex,
              true,
            ),
            "methodName": "initialize",
          },
        },
      },
    );

  let%AwaitThen _ =
    longShortInstance
    ->ContractHelpers.connect(~address=admin)
    ->LongShort.createNewSyntheticMarketExternalSyntheticTokens(
        ~syntheticName,
        ~syntheticSymbol,
        ~longToken=syntheticTokenLong.address,
        ~shortToken=syntheticTokenShort.address,
        ~paymentToken=paymentToken.address,
        ~oracleManager=oracleManagerAddress,
        ~yieldManager=yieldManagerAddress,
      );

  Js.log("a.5");

  let kInitialMultiplier = bnFromString("5000000000000000000"); // 5x
  let kPeriod = bnFromInt(864000); // 10 days

  Js.log("a.6");
  let unstakeFee_e18 = bnFromString("5000000000000000"); // 50 basis point unstake fee
  let initialMarketSeedForEachMarketSide =
    bnFromString("1000000000000000000");

  let%AwaitThen _ =
    paymentToken
    ->ContractHelpers.connect(~address=admin)
    ->ERC20Mock.approve(
        ~spender=longShortInstance.address,
        ~amount=initialMarketSeedForEachMarketSide->mul(bnFromInt(3)),
      );

  Js.log("a.7");
  longShortInstance
  ->ContractHelpers.connect(~address=admin)
  ->LongShort.initializeMarket(
      ~marketIndex=newMarketIndex,
      ~kInitialMultiplier,
      ~kPeriod,
      ~unstakeFee_e18, // 50 basis point unstake fee
      ~initialMarketSeedForEachMarketSide,
      ~balanceIncentiveCurve_exponent=bnFromInt(5),
      ~balanceIncentiveCurve_equilibriumOffset=bnFromInt(0),
      ~marketTreasurySplitGradient_e18=bnFromInt(1),
      ~marketLeverage=CONSTANTS.tenToThe18,
    );
};

let deployMumbaiMarketUpgradeable =
    (
      ~syntheticName,
      ~syntheticSymbol,
      ~longShortInstance: LongShort.t,
      ~stakerInstance: Staker.t,
      ~treasuryInstance: Treasury_v0.t,
      ~admin,
      ~paymentToken: ERC20Mock.t,
      ~oraclePriceFeedAddress: Ethers.ethAddress,
      ~deployments: Hardhat.deployments_t,
      ~namedAccounts: Hardhat.namedAccounts,
    ) => {
  let%AwaitThen oracleManager =
    deployments->Hardhat.deploy(
      ~name="OracleManager" ++ syntheticSymbol,
      ~arguments={
        "from": namedAccounts.deployer,
        "log": true,
        "contract": "OracleManagerChainlinkTestnet",
        "args": (admin.address, oraclePriceFeedAddress, bnFromInt(27)),
      },
    );

  Js.log("a.1");

  let aavePoolAddressProviderMumbai = "0x178113104fEcbcD7fF8669a0150721e231F0FD4B";
  let mumbaiADai = "0x639cB7b21ee2161DF9c882483C9D55c90c20Ca3e";
  let mumbaiAaveIncentivesController = "0xd41aE58e803Edf4304334acCE4DC4Ec34a63C644";

  Js.log("a.3");
  let%AwaitThen yieldManager =
    deployments->Hardhat.deploy(
      ~name="YieldManager" ++ syntheticSymbol,
      ~arguments={
        "from": namedAccounts.deployer,
        "contract": "YieldManagerAave",
        "log": true,
        "proxy": {
          "proxyContract": "UUPSProxy",
          "execute": {
            "methodName": "initialize",
            "args": (
              longShortInstance.address,
              treasuryInstance.address,
              paymentToken.address,
              mumbaiADai,
              aavePoolAddressProviderMumbai,
              mumbaiAaveIncentivesController,
              0,
              admin.address,
            ),
          },
        },
      },
    );
  Js.log("a.4");
  deployTestnetMarketUpgradeableCore(
    ~syntheticName,
    ~syntheticSymbol,
    ~longShortInstance,
    ~stakerInstance,
    ~admin,
    ~paymentToken,
    ~oracleManagerAddress=oracleManager.address,
    ~yieldManagerAddress=yieldManager.address,
    ~deployments,
    ~namedAccounts,
  );
};

let deployFantomTestnetMarketUpgradeable =
    (
      ~syntheticName,
      ~syntheticSymbol,
      ~longShortInstance: LongShort.t,
      ~stakerInstance: Staker.t,
      ~treasuryInstance: Treasury_v0.t,
      ~admin,
      ~paymentToken: ERC20Mock.t,
      ~oraclePriceFeedAddress: Ethers.ethAddress,
      ~deployments: Hardhat.deployments_t,
      ~namedAccounts: Hardhat.namedAccounts,
    ) => {
  let%AwaitThen oracleManager =
    deployments->Hardhat.deploy(
      ~name="OracleManager" ++ syntheticSymbol,
      ~arguments={
        "from": namedAccounts.deployer,
        "log": true,
        "contract": "OracleManagerChainlinkTestnet",
        "args": (admin.address, oraclePriceFeedAddress, bnFromInt(27)),
      },
    );

  Js.log("a.1");

  let aavePoolAddressProviderFantomTestnet = "0xD90db1ca5A6e9873BCD9B0279AE038272b656728";
  let fantomTestnetADai = "0xbAA97949f28899Fc7E89ff67A033e9f46fbA0846";
  let fantomTestnetAaveIncentivesController = "0x509B2506FbA1BD41765F6A82C7B0Dd4229191768";

  Js.log("a.3");
  let%AwaitThen yieldManager =
    deployments->Hardhat.deploy(
      ~name="YieldManager" ++ syntheticSymbol,
      ~arguments={
        "from": namedAccounts.deployer,
        "contract": "DefaultYieldManagerAaveV3",
        "log": true,
        "proxy": {
          "proxyContract": "UUPSProxy",
          "execute": {
            "methodName": "initialize",
            "args": (
              longShortInstance.address,
              treasuryInstance.address,
              paymentToken.address,
              fantomTestnetADai,
              aavePoolAddressProviderFantomTestnet,
              fantomTestnetAaveIncentivesController,
              0,
              admin.address,
            ),
          },
        },
      },
    );
  Js.log("a.4");
  deployTestnetMarketUpgradeableCore(
    ~syntheticName,
    ~syntheticSymbol,
    ~longShortInstance,
    ~stakerInstance,
    ~admin,
    ~paymentToken,
    ~oracleManagerAddress=oracleManager.address,
    ~yieldManagerAddress=yieldManager.address,
    ~deployments,
    ~namedAccounts,
  );
};
