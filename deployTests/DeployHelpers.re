open Ethers;
open LetOps;
open Globals;

let minSenderBalance = bnFromString("50000000000000000");
let minRecieverBalance = bnFromString("20000000000000000");

let topupBalanceIfLow = (~from: Wallet.t, ~to_: Wallet.t) => {
  let%AwaitThen senderBalance = from->Wallet.getBalance;

  if (senderBalance->bnLt(minSenderBalance)) {
    Js.Exn.raiseError(
      "WARNING - Sender doesn't have enough eth - need at least 0.05 ETH! (top up to over 1 ETH to be safe)",
    );
  };
  let%Await recieverBalance = to_->Wallet.getBalance;
  if (recieverBalance->bnLt(minRecieverBalance)) {
    let _ =
      from->Wallet.sendTransaction({
        to_: to_.address,
        value: minRecieverBalance,
      });
    ();
  };
};

let updateSystemState = (~longShort, ~admin, ~marketIndex) => {
  longShort
  ->ContractHelpers.connect(~address=admin)
  ->LongShort.updateSystemState(~marketIndex);
};

let mintAndApprove = (~paymentToken, ~amount, ~user, ~approvedAddress) => {
  let%AwaitThen _ = paymentToken->ERC20Mock.mint(~_to=user.address, ~amount);

  paymentToken
  ->ContractHelpers.connect(~address=user)
  ->ERC20Mock.approve(~spender=approvedAddress, ~amount);
};

let stakeSynthLong = (~amount, ~longShort, ~marketIndex, ~user) => {
  let%AwaitThen longAddress =
    longShort->LongShort.syntheticTokens(marketIndex, true);
  let%AwaitThen synth = SyntheticToken.at(longAddress);
  let%Await usersSyntheticTokenBalance =
    synth->SyntheticToken.balanceOf(~account=user.address);
  if (usersSyntheticTokenBalance->bnGt(bnFromString("0"))) {
    let _ =
      synth
      ->ContractHelpers.connect(~address=user)
      ->SyntheticToken.stake(~amount);
    ();
  };
};

let executeOnMarkets =
    (marketIndexes: array(int), functionToExecute: int => Js.Promise.t('a)) => {
  marketIndexes->Array.reduce(
    JsPromise.resolve(),
    (previousPromise, marketIndex) => {
      let%AwaitThen _ = previousPromise;
      functionToExecute(marketIndex);
    },
  );
};

let setOracleManagerPrice = (~longShort, ~marketIndex, ~admin) => {
  let%AwaitThen oracleManagerAddr =
    longShort->LongShort.oracleManagers(marketIndex);
  let%AwaitThen oracleManager = OracleManagerMock.at(oracleManagerAddr);

  let%AwaitThen currentPrice = oracleManager->OracleManagerMock.getLatestPrice;
  let nextPrice = currentPrice->mul(bnFromInt(101))->div(bnFromInt(100));

  oracleManager
  ->ContractHelpers.connect(~address=admin)
  ->OracleManagerMock.setPrice(~newPrice=nextPrice);
};

let redeemShortNextPriceWithSystemUpdate =
    (~amount, ~marketIndex, ~longShort, ~user, ~admin) => {
  let%AwaitThen _ =
    longShort
    ->ContractHelpers.connect(~address=user)
    ->LongShort.redeemShortNextPrice(~marketIndex, ~tokens_redeem=amount);
  let%AwaitThen _ = setOracleManagerPrice(~longShort, ~marketIndex, ~admin);
  updateSystemState(~longShort, ~admin, ~marketIndex);
};

let redeemNextPrice = (~amount, ~marketIndex, ~longShort, ~user, ~isLong) => {
  let redeemFunction =
    isLong ? LongShort.redeemLongNextPrice : LongShort.redeemShortNextPrice;
  longShort
  ->ContractHelpers.connect(~address=user)
  ->redeemFunction(~marketIndex, ~tokens_redeem=amount);
};

let shiftFromShortNextPriceWithSystemUpdate =
    (~amount, ~marketIndex, ~longShort, ~user, ~admin) => {
  let%AwaitThen _ =
    longShort
    ->ContractHelpers.connect(~address=user)
    ->LongShort.shiftPositionFromShortNextPrice(
        ~marketIndex,
        ~amountSyntheticTokensToShift=amount,
      );
  let%AwaitThen _ = setOracleManagerPrice(~longShort, ~marketIndex, ~admin);
  longShort
  ->ContractHelpers.connect(~address=admin)
  ->LongShort.updateSystemState(~marketIndex);
};
let shiftFromLongNextPriceWithSystemUpdate =
    (~amount, ~marketIndex, ~longShort, ~user, ~admin) => {
  let%AwaitThen _ =
    longShort
    ->ContractHelpers.connect(~address=user)
    ->LongShort.shiftPositionFromLongNextPrice(
        ~marketIndex,
        ~amountSyntheticTokensToShift=amount,
      );
  let%AwaitThen _ = setOracleManagerPrice(~longShort, ~marketIndex, ~admin);
  longShort->LongShort.updateSystemState(~marketIndex);
};

let mintLongNextPriceWithSystemUpdate =
    (
      ~amount,
      ~marketIndex,
      ~paymentToken,
      ~longShort: LongShort.t,
      ~user,
      ~admin,
    ) => {
  let%AwaitThen _ =
    mintAndApprove(
      ~paymentToken,
      ~amount,
      ~user,
      ~approvedAddress=longShort.address,
    );
  let%AwaitThen _ =
    longShort
    ->ContractHelpers.connect(~address=user)
    ->LongShort.mintLongNextPrice(~marketIndex, ~amount);
  let%AwaitThen _ = setOracleManagerPrice(~longShort, ~marketIndex, ~admin);
  updateSystemState(~longShort, ~admin, ~marketIndex);
};

let mintNextPrice =
    (
      ~amount,
      ~marketIndex,
      ~paymentToken,
      ~longShort: LongShort.t,
      ~user,
      ~isLong,
    ) => {
  let%AwaitThen _ =
    paymentToken
    ->ContractHelpers.connect(~address=user)
    ->ERC20Mock.approve(~spender=longShort.address, ~amount);

  let mintFunction =
    isLong ? LongShort.mintLongNextPrice : LongShort.mintShortNextPrice;

  longShort
  ->ContractHelpers.connect(~address=user)
  ->mintFunction(~marketIndex, ~amount);
};

let mintShortNextPriceWithSystemUpdate =
    (
      ~amount,
      ~marketIndex,
      ~paymentToken,
      ~longShort: LongShort.t,
      ~user,
      ~admin,
    ) => {
  let%AwaitThen _ =
    mintAndApprove(
      ~paymentToken,
      ~amount,
      ~user,
      ~approvedAddress=longShort.address,
    );
  let%AwaitThen _ =
    longShort
    ->ContractHelpers.connect(~address=user)
    ->LongShort.mintShortNextPrice(~marketIndex, ~amount);

  let%AwaitThen _ = setOracleManagerPrice(~longShort, ~marketIndex, ~admin);

  updateSystemState(~longShort, ~admin, ~marketIndex);
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
  let%AwaitThen oracleManager = OracleManagerMock.make(~admin=admin.address);

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

  let%AwaitThen _ =
    longShortInstance
    ->ContractHelpers.connect(~address=admin)
    ->LongShort.createNewSyntheticMarket(
        ~syntheticName,
        ~syntheticSymbol,
        ~paymentToken=paymentToken.address,
        ~oracleManager=oracleManager.address,
        ~yieldManager=yieldManager.address,
      );

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

let deployMumbaiMarket =
    (
      ~syntheticName,
      ~syntheticSymbol,
      ~longShortInstance: LongShort.t,
      ~treasuryInstance: Treasury_v0.t,
      ~admin,
      ~paymentToken: ERC20Mock.t,
      ~oraclePriceFeedAddress: Ethers.ethAddress,
    ) => {
  let%AwaitThen oracleManager =
    OracleManagerChainlink.make(
      ~admin=admin.address,
      ~chainlinkOracle=oraclePriceFeedAddress,
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

  let%AwaitThen _ =
    longShortInstance
    ->ContractHelpers.connect(~address=admin)
    ->LongShort.createNewSyntheticMarket(
        ~syntheticName,
        ~syntheticSymbol,
        ~paymentToken=paymentToken.address,
        ~oracleManager=oracleManager.address,
        ~yieldManager=yieldManager.address,
      );

  let%AwaitThen latestMarket = longShortInstance->LongShort.latestMarket;
  let kInitialMultiplier = bnFromString("5000000000000000000"); // 5x
  let kPeriod = bnFromInt(864000); // 10 days

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

  let kInitialMultiplier = bnFromString("2000000000000000000"); // 5x
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
      ~marketTreasurySplitGradient_e18=bnFromInt(1),
      ~marketLeverage=bnFromInt(3)->mul(CONSTANTS.tenToThe18),
    );
};
