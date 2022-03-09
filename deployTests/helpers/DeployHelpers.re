open Ethers;
open LetOps;
open Globals;

let addGemsNfts = (~gemCollectorNFT, ~tokenUri, ~minGems) => {
  let%AwaitThen loadedAccounts = Ethers.getSigners();

  let admin = loadedAccounts->Array.getUnsafe(1);

  Js.log("deploying gems nft");

  gemCollectorNFT
  ->ContractHelpers.connect(~address=admin)
  ->GemCollectorNFT.addToken(~tokenUri, ~minGems);
};

let mintGemsNft = (~gemCollectorNFT, ~levelId, ~receiver) => {
  Js.log("minting a gems nft");
  let%AwaitThen loadedAccounts = Ethers.getSigners();

  let user1 = loadedAccounts->Array.getUnsafe(2);

  gemCollectorNFT
  ->ContractHelpers.connect(~address=user1)
  ->GemCollectorNFT.mintNFT(~levelId, ~receiver);
};

let deployFlipp3ningPolygon =
    (
      ~longShortInstance: LongShort.t,
      ~stakerInstance: Staker.t,
      ~treasuryInstance: Treasury_v0.t,
      ~admin,
      ~paymentToken: ERC20Mock.t,
      ~ethMarketCapOraclePriceFeedAddress: Ethers.ethAddress,
      ~btcMarketCapOraclePriceFeedAddress: Ethers.ethAddress,
      ~deployments: Hardhat.deployments_t,
      ~namedAccounts: Hardhat.namedAccounts,
    ) => {
  let syntheticName = "Flipp3ning";
  let syntheticSymbol = "F3";

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
              "fs" ++ syntheticSymbol,
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
              "fl" ++ syntheticSymbol,
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

  let%AwaitThen oracleManager =
    deployments->Hardhat.deploy(
      ~name="OracleManager" ++ syntheticSymbol,
      ~arguments={
        "from": namedAccounts.deployer,
        "log": true,
        "contract": "OracleManagerFlipp3ning",
        "args": (
          ethMarketCapOraclePriceFeedAddress,
          btcMarketCapOraclePriceFeedAddress,
        ),
      },
    );

  Js.log("a.1");

  // https://polygonscan.com/address/0xd05e3E715d945B59290df0ae8eF85c1BdB684744#code
  let aavePoolAddressProviderPolygon = "0xd05e3E715d945B59290df0ae8eF85c1BdB684744";
  // https://polygonscan.com/address/0x27F8D03b3a2196956ED754baDc28D73be8830A6e#code
  let aDaiPolygon = "0x27F8D03b3a2196956ED754baDc28D73be8830A6e";
  // https://polygonscan.com/address/0x357D51124f59836DeD84c8a1730D72B749d8BC23#code
  let aaveIncentivesControllerPolygon = "0x357D51124f59836DeD84c8a1730D72B749d8BC23";

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
              aDaiPolygon,
              aavePoolAddressProviderPolygon,
              aaveIncentivesControllerPolygon,
              0,
              admin.address,
            ),
          },
        },
      },
    );
  Js.log("a.4");
  Js.log((
    yieldManager.address,
    syntheticTokenLong.address,
    syntheticTokenShort.address,
  ));

  let%AwaitThen _ =
    longShortInstance
    ->ContractHelpers.connect(~address=admin)
    ->LongShort.createNewSyntheticMarketExternalSyntheticTokens(
        ~syntheticName,
        ~syntheticSymbol,
        ~longToken=syntheticTokenLong.address,
        ~shortToken=syntheticTokenShort.address,
        ~paymentToken=paymentToken.address,
        ~oracleManager=oracleManager.address,
        ~yieldManager=yieldManager.address,
      );

  Js.log("a.5");

  let kInitialMultiplier = bnFromString("2000000000000000000"); // 2x
  let kPeriod = bnFromInt(5184000); // 60 days

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
      ~marketTreasurySplitGradient_e18=CONSTANTS.tenToThe18,
      ~marketLeverage=bnFromInt(3)->mul(CONSTANTS.tenToThe18),
    );
};
let deploy3TH_Polygon =
    (
      ~longShortInstance: LongShort.t,
      ~stakerInstance: Staker.t,
      ~treasuryInstance: Treasury_v0.t,
      ~admin,
      ~paymentToken: ERC20Mock.t,
      ~ethUSDPriceFeedAddress: Ethers.ethAddress,
      ~deployments: Hardhat.deployments_t,
      ~namedAccounts: Hardhat.namedAccounts,
    ) => {
  let syntheticName = "Ether 3x";
  let syntheticSymbol = "3TH";

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
              "fs" ++ syntheticSymbol,
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
            "methodName": "initialize",
            "args": (
              "Float Long " ++ syntheticName,
              "fl" ++ syntheticSymbol,
              longShortInstance.address,
              stakerInstance.address,
              newMarketIndex,
              true,
            ),
          },
        },
      },
    );

  let%AwaitThen oracleManager =
    deployments->Hardhat.deploy(
      ~name="OracleManager" ++ syntheticSymbol,
      ~arguments={
        "from": namedAccounts.deployer,
        "log": true,
        "contract": "OracleManagerChainlink",
        "args": (namedAccounts.admin, ethUSDPriceFeedAddress),
      },
    );
  Js.log("a.1");
  // https://polygonscan.com/address/0xd05e3E715d945B59290df0ae8eF85c1BdB684744#code
  let aavePoolAddressProviderPolygon = "0xd05e3E715d945B59290df0ae8eF85c1BdB684744";
  // https://polygonscan.com/address/0x27F8D03b3a2196956ED754baDc28D73be8830A6e#code
  let aDaiPolygon = "0x27F8D03b3a2196956ED754baDc28D73be8830A6e";
  // https://polygonscan.com/address/0x357D51124f59836DeD84c8a1730D72B749d8BC23#code
  let aaveIncentivesControllerPolygon = "0x357D51124f59836DeD84c8a1730D72B749d8BC23";
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
              aDaiPolygon,
              aavePoolAddressProviderPolygon,
              aaveIncentivesControllerPolygon,
              0,
              admin.address,
            ),
          },
        },
      },
    );
  Js.log("a.4");
  Js.log((
    yieldManager.address,
    syntheticTokenLong.address,
    syntheticTokenShort.address,
  ));
  let%AwaitThen _ =
    longShortInstance
    ->ContractHelpers.connect(~address=admin)
    ->LongShort.createNewSyntheticMarketExternalSyntheticTokens(
        ~syntheticName,
        ~syntheticSymbol,
        ~longToken=syntheticTokenLong.address,
        ~shortToken=syntheticTokenShort.address,
        ~paymentToken=paymentToken.address,
        ~oracleManager=oracleManager.address,
        ~yieldManager=yieldManager.address,
      );
  Js.log("a.5");
  let kInitialMultiplier = bnFromString("2000000000000000000"); // 2x
  let kPeriod = bnFromInt(5184000); // 60 days
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
      ~marketTreasurySplitGradient_e18=CONSTANTS.tenToThe18,
      ~marketLeverage=bnFromInt(3)->mul(CONSTANTS.tenToThe18),
    );
};

let deployMarketOnPolygon =
    (
      ~longShortInstance: LongShort.t,
      ~stakerInstance: Staker.t,
      ~treasuryInstance: Treasury_v0.t,
      ~admin,
      ~paymentToken: ERC20Mock.t,
      ~chainlinkOricleFeedAddress: Ethers.ethAddress,
      ~deployments: Hardhat.deployments_t,
      ~namedAccounts: Hardhat.namedAccounts,
      ~syntheticName,
      ~syntheticSymbol,
      ~leverageAmount,
      ~expectedMarketIndex,
      ~fundingRateMultiplier_e18,
    ) => {
  let%AwaitThen latestMarket = longShortInstance->LongShort.latestMarket;
  let newMarketIndex = latestMarket + 1;

  if (newMarketIndex != expectedMarketIndex) {
    Js.Exn.raiseError(
      "Wrong market Index: "
      ++ newMarketIndex->Int.toString
      ++ "(actuall index) != "
      ++ expectedMarketIndex->Int.toString
      ++ "(expected index)",
    );
  };

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
              "fs" ++ syntheticSymbol,
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
            "methodName": "initialize",
            "args": (
              "Float Long " ++ syntheticName,
              "fl" ++ syntheticSymbol,
              longShortInstance.address,
              stakerInstance.address,
              newMarketIndex,
              true,
            ),
          },
        },
      },
    );

  let%AwaitThen oracleManager =
    deployments->Hardhat.deploy(
      ~name="OracleManager" ++ syntheticSymbol,
      ~arguments={
        "from": namedAccounts.deployer,
        "log": true,
        "contract": "OracleManagerChainlink",
        "args": (namedAccounts.admin, chainlinkOricleFeedAddress),
      },
    );
  Js.log("a.1");
  // https://polygonscan.com/address/0xd05e3E715d945B59290df0ae8eF85c1BdB684744#code
  let aavePoolAddressProviderPolygon = "0xd05e3E715d945B59290df0ae8eF85c1BdB684744";
  // https://polygonscan.com/address/0x27F8D03b3a2196956ED754baDc28D73be8830A6e#code
  let aDaiPolygon = "0x27F8D03b3a2196956ED754baDc28D73be8830A6e";
  // https://polygonscan.com/address/0x357D51124f59836DeD84c8a1730D72B749d8BC23#code
  let aaveIncentivesControllerPolygon = "0x357D51124f59836DeD84c8a1730D72B749d8BC23";
  Js.log("a.3");
  let%AwaitThen yieldManager =
    deployments->Hardhat.deploy(
      ~name="YieldManager" ++ syntheticSymbol,
      ~arguments={
        "from": namedAccounts.deployer,
        "contract": "YieldManagerAaveBasic",
        "log": true,
        "proxy": {
          "proxyContract": "UUPSProxy",
          "execute": {
            "methodName": "initialize",
            "args": (
              longShortInstance.address,
              treasuryInstance.address,
              paymentToken.address,
              aDaiPolygon,
              aavePoolAddressProviderPolygon,
              aaveIncentivesControllerPolygon,
              0,
              admin.address,
            ),
          },
        },
      },
    );
  Js.log("a.4");
  Js.log((
    yieldManager.address,
    syntheticTokenLong.address,
    syntheticTokenShort.address,
  ));
  let%AwaitThen _ =
    longShortInstance
    ->ContractHelpers.connect(~address=admin)
    ->LongShort.createNewSyntheticMarketExternalSyntheticTokens(
        ~syntheticName,
        ~syntheticSymbol,
        ~longToken=syntheticTokenLong.address,
        ~shortToken=syntheticTokenShort.address,
        ~paymentToken=paymentToken.address,
        ~oracleManager=oracleManager.address,
        ~yieldManager=yieldManager.address,
      );
  Js.log("a.5");
  let kInitialMultiplier = bnFromString("1000000000000000000"); // 1x
  let kPeriod = bnFromInt(0); // 0 days
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
  Js.log2("a.7 - seeding from", admin);
  let%AwaitThen _ =
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
        ~marketTreasurySplitGradient_e18=CONSTANTS.tenToThe18,
        ~marketLeverage=bnFromInt(leverageAmount)->mul(CONSTANTS.tenToThe18),
      );
  Js.log("market launched, setting funding rate");

  longShortInstance
  ->ContractHelpers.connect(~address=admin)
  ->LongShort.changeMarketFundingRateMultiplier(
      ~marketIndex=newMarketIndex,
      ~fundingRateMultiplier_e18,
    );
};

let deployAaveDAIYieldManager =
    (
      ~deployments,
      ~syntheticSymbol,
      ~deployer,
      ~longShortInstanceAddress,
      ~treasuryInstanceAddress,
      ~paymentTokenAddress,
      ~admin,
    ) => {
  // https://cchain.explorer.avax.network/address/0xb6A86025F0FE1862B372cb0ca18CE3EDe02A318f
  let aavePoolAddressProviderPolygon = "0xb6A86025F0FE1862B372cb0ca18CE3EDe02A318f";
  // https://cchain.explorer.avax.network/address/0x47AFa96Cdc9fAb46904A55a6ad4bf6660B53c38a
  let aDaiPolygon = "0x47AFa96Cdc9fAb46904A55a6ad4bf6660B53c38a";
  // https://cchain.explorer.avax.network/address/0x01D83Fe6A10D2f2B7AF17034343746188272cAc9
  let aaveIncentivesControllerPolygon = "0x01D83Fe6A10D2f2B7AF17034343746188272cAc9";

  deployments->Hardhat.deploy(
    ~name="YieldManager" ++ syntheticSymbol,
    ~arguments={
      "from": deployer,
      "contract": "DefaultYieldManagerAave",
      "log": true,
      "proxy": {
        "proxyContract": "UUPSProxy",
        "execute": {
          "methodName": "initialize",
          "args": (
            longShortInstanceAddress,
            treasuryInstanceAddress,
            paymentTokenAddress,
            aDaiPolygon,
            aavePoolAddressProviderPolygon,
            aaveIncentivesControllerPolygon,
            0,
            admin.address,
          ),
        },
      },
    },
  );
};

let deployCompoundDAIYieldManager =
    (
      ~deployments,
      ~syntheticSymbol,
      ~deployer,
      ~longShortInstanceAddress,
      ~treasuryInstanceAddress,
      ~paymentTokenAddress,
      ~admin,
      ~cToken,
    ) => {
  deployments->Hardhat.deploy(
    ~name="YieldManager" ++ syntheticSymbol,
    ~arguments={
      "from": deployer,
      "contract": "DefaultYieldManagerCompound",
      "log": true,
      "proxy": {
        "proxyContract": "UUPSProxy",
        "execute": {
          "methodName": "initialize",
          "args": (
            longShortInstanceAddress,
            treasuryInstanceAddress,
            paymentTokenAddress,
            cToken,
            admin.address,
          ),
        },
      },
    },
  );
};

type cTokenAddress = string;
type yieldManagers =
  | AaveDAI
  | CompoundDAI(cTokenAddress);

let deployAvalancheMarket =
    (
      ~longShortInstance: LongShort.t,
      ~stakerInstance: Staker.t,
      ~treasuryInstance: Treasury_v0.t,
      ~admin,
      ~paymentToken: ERC20Mock.t,
      ~oraclePriceFeedAddress: Ethers.ethAddress,
      ~deployments: Hardhat.deployments_t,
      ~namedAccounts: Hardhat.namedAccounts,
      ~syntheticName,
      ~syntheticSymbol,
      ~fundingRateMultiplier,
      ~marketLeverage,
      ~expectedMarketIndex,
      ~yieldManagerVariant: yieldManagers,
    ) => {
  let%AwaitThen latestMarket = longShortInstance->LongShort.latestMarket;
  let newMarketIndex = latestMarket + 1;

  if (newMarketIndex != expectedMarketIndex) {
    Js.Exn.raiseError(
      "Wrong market Index: "
      ++ newMarketIndex->Int.toString
      ++ "(actuall index) != "
      ++ expectedMarketIndex->Int.toString
      ++ "(expected index)",
    );
  };

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
              "fs" ++ syntheticSymbol,
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
            "methodName": "initialize",
            "args": (
              "Float Long " ++ syntheticName,
              "fl" ++ syntheticSymbol,
              longShortInstance.address,
              stakerInstance.address,
              newMarketIndex,
              true,
            ),
          },
        },
      },
    );
  Js.log2("Oracle manager!", oraclePriceFeedAddress);

  let%AwaitThen oracleManager =
    deployments->Hardhat.deploy(
      ~name="OracleManager" ++ syntheticSymbol,
      ~arguments={
        "from": namedAccounts.deployer,
        "log": true,
        "contract": "OracleManagerChainlink",
        "args": (namedAccounts.admin, oraclePriceFeedAddress),
      },
    );
  Js.log("a.1");
  Js.log("a.2");
  Js.log("a.3");
  let%AwaitThen yieldManager =
    switch (yieldManagerVariant) {
    | CompoundDAI(cToken) =>
      deployCompoundDAIYieldManager(
        ~deployments,
        ~syntheticSymbol,
        ~deployer=namedAccounts.deployer,
        ~longShortInstanceAddress=longShortInstance.address,
        ~treasuryInstanceAddress=treasuryInstance.address,
        ~paymentTokenAddress=paymentToken.address,
        ~admin,
        ~cToken,
      )
    | AaveDAI =>
      deployAaveDAIYieldManager(
        ~deployments,
        ~syntheticSymbol,
        ~deployer=namedAccounts.deployer,
        ~longShortInstanceAddress=longShortInstance.address,
        ~treasuryInstanceAddress=treasuryInstance.address,
        ~paymentTokenAddress=paymentToken.address,
        ~admin,
      )
    };
  Js.log("a.4");
  Js.log((
    yieldManager.address,
    syntheticTokenLong.address,
    syntheticTokenShort.address,
  ));
  let%AwaitThen _ =
    longShortInstance
    ->ContractHelpers.connect(~address=admin)
    ->LongShort.createNewSyntheticMarketExternalSyntheticTokens(
        ~syntheticName,
        ~syntheticSymbol,
        ~longToken=syntheticTokenLong.address,
        ~shortToken=syntheticTokenShort.address,
        ~paymentToken=paymentToken.address,
        ~oracleManager=oracleManager.address,
        ~yieldManager=yieldManager.address,
      );
  Js.log("a.5");
  let kInitialMultiplier = bnFromString("2000000000000000000"); // 2x
  let kPeriod = bnFromInt(5184000); // 60 days
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
  let%AwaitThen _ =
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
        ~marketTreasurySplitGradient_e18=CONSTANTS.tenToThe18,
        ~marketLeverage=bnFromInt(marketLeverage)->mul(CONSTANTS.tenToThe18),
      );
  Js.log("market launched, setting the funding rate");

  longShortInstance
  ->ContractHelpers.connect(~address=admin)
  ->LongShort.changeMarketFundingRateMultiplier(
      ~marketIndex=newMarketIndex,
      ~fundingRateMultiplier_e18=fundingRateMultiplier,
    );
};
