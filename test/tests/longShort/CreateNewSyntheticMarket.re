open LetOps;
open Mocha;
open Globals;
open Helpers;

let testUnit =
    (
      ~contracts: ref(Helpers.longShortUnitTestContracts),
      ~accounts as _: ref(array(Ethers.Wallet.t)),
    ) => {
  describeUnit("createNewSyntheticMarketExternalSyntheticTokens", () => {
    let latestMarket = Helpers.randomJsInteger();

    let marketIndex = latestMarket + 1;

    let syntheticName = "Flippening";
    let syntheticSymbol = "FLP";

    let assetPrice = Helpers.randomTokenAmount();

    let (longToken, shortToken, paymentToken) =
      Helpers.Tuple.make3(Helpers.randomAddress);

    let createNewSyntheticMarketUpgradeableCallRef = ref(None->Obj.magic);

    before_once'(() => {
      contracts.contents.oracleManagerSmocked
      ->OracleManagerMockSmocked.mockUpdatePriceToReturn(assetPrice);
      let%AwaitThen _ =
        contracts.contents.longShort
        ->LongShortStateSetters.setLatestMarket(~latestMarket);
      createNewSyntheticMarketUpgradeableCallRef :=
        contracts.contents.longShort
        ->LongShort.createNewSyntheticMarketExternalSyntheticTokens(
            ~syntheticName,
            ~syntheticSymbol,
            ~longToken,
            ~shortToken,
            ~paymentToken,
            ~oracleManager=contracts.contents.oracleManagerSmocked.address,
            ~yieldManager=contracts.contents.yieldManagerSmocked.address,
          );
      createNewSyntheticMarketUpgradeableCallRef.contents;
    });

    it("increments latest market", () => {
      let%Await latestMarketContractVal =
        contracts.contents.longShort->LongShort.latestMarket;
      Chai.intEqual(marketIndex, latestMarketContractVal);
    });

    it("sets payment token correctly", () => {
      let%Await paymentTokenContractVal =
        contracts.contents.longShort->LongShort.paymentTokens(marketIndex);
      Chai.addressEqual(paymentTokenContractVal, ~otherAddress=paymentToken);
    });

    it("sets yield manager correctly", () => {
      let%Await yieldManagerContractVal =
        contracts.contents.longShort->LongShort.yieldManagers(marketIndex);
      Chai.addressEqual(
        yieldManagerContractVal,
        ~otherAddress=contracts.contents.yieldManagerSmocked.address,
      );
    });

    it("sets oracle manager correctly", () => {
      let%Await oracleManagerContractVal =
        contracts.contents.longShort->LongShort.oracleManagers(marketIndex);
      Chai.addressEqual(
        oracleManagerContractVal,
        ~otherAddress=contracts.contents.oracleManagerSmocked.address,
      );
    });

    it("sets asset price correctly", () => {
      let%Await assetPriceContractVal =
        contracts.contents.longShort->LongShort.assetPrice(marketIndex);
      Chai.bnEqual(assetPriceContractVal, assetPrice);
    });

    it("sets synthetic tokens correctly", () => {
      let%AwaitThen longTokenContractVal =
        contracts.contents.longShort
        ->LongShort.syntheticTokens(marketIndex, true);
      let%Await shortTokenContractVal =
        contracts.contents.longShort
        ->LongShort.syntheticTokens(marketIndex, false);
      Chai.addressEqual(longTokenContractVal, ~otherAddress=longToken);
      Chai.addressEqual(
        shortTokenContractVal,
        ~otherAddress=shortTokenContractVal,
      );
    });

    it("calls initializeForMarket on yield manager", () => {
      contracts.contents.yieldManagerSmocked
      ->YieldManagerAaveBasicSmocked.initializeForMarketCallCheck
    });

    it("calls updatePrice price on the oracle", () => {
      contracts.contents.oracleManagerSmocked
      ->OracleManagerMockSmocked.updatePriceCallCheck
    });

    it("emits SyntheticMarketCreated with correct args", () => {
      Chai.callEmitEvents(
        ~call=createNewSyntheticMarketUpgradeableCallRef.contents,
        ~contract=contracts.contents.longShort->Obj.magic,
        ~eventName="SyntheticMarketCreated",
      )
      ->Chai.withArgs9(
          marketIndex,
          longToken,
          shortToken,
          paymentToken,
          assetPrice,
          syntheticName,
          syntheticSymbol,
          contracts.contents.oracleManagerSmocked.address,
          contracts.contents.yieldManagerSmocked.address,
        )
    });
  });
};
