open Globals;
open LetOps;
open Mocha;

let testUnit =
    (
      ~contracts: ref(Helpers.stakerUnitTestContracts),
      ~accounts as _: ref(array(Ethers.Wallet.t)),
    ) => {
  let marketIndex = Helpers.randomJsInteger();
  let (
    longPrice,
    shortPrice,
    longValue,
    shortValue,
    timeDeltaGreaterThanZero,
    longAccum,
    shortAccum,
  ) =
    Helpers.Tuple.make7(Helpers.randomInteger);

  describe("pushUpdatedMarketPricesToUpdateFloatIssuanceCalculations", () => {
    let timestampRef: ref(Ethers.BigNumber.t) = ref(CONSTANTS.zeroBn);
    let txReference = ref("NotSetYet"->Obj.magic);

    before_once'(() => {
      contracts.contents.staker
      ->StakerSmocked.InternalMock.setupFunctionForUnitTesting(
          ~functionName=
            "pushUpdatedMarketPricesToUpdateFloatIssuanceCalculations",
        )
    });

    let setup = (~marketUpdateIndex, ~timeDelta) => {
      let%Await {timestamp} = Helpers.getBlock();
      timestampRef := (timestamp + 1)->Ethers.BigNumber.fromInt /* one second per bloc*/;

      StakerSmocked.InternalMock.mock_calculateTimeDeltaFromLastAccumulativeIssuancePerStakedSynthSnapshotToReturn(
        timeDelta,
      );
      StakerSmocked.InternalMock.mock_calculateNewCumulativeIssuancePerStakedSynthToReturn(
        longAccum,
        shortAccum,
      );

      txReference :=
        contracts.contents.staker
        ->Staker.pushUpdatedMarketPricesToUpdateFloatIssuanceCalculations(
            ~marketIndex,
            ~longPrice,
            ~shortPrice,
            ~longValue,
            ~shortValue,
            ~marketUpdateIndex,
          );
    };

    describe("modifiers", () =>
      it("calls the onlyLongShort modifier", () => {
        let%Await _ =
          contracts.contents.staker
          ->Staker.pushUpdatedMarketPricesToUpdateFloatIssuanceCalculations(
              ~marketIndex,
              ~marketUpdateIndex=Helpers.randomInteger(),
              ~longPrice,
              ~shortPrice,
              ~longValue,
              ~shortValue,
            );

        StakerSmocked.InternalMock.onlyLongShortModifierLogicCallCheck();
      })
    );

    describe("case timeDelta > 0", () => {
      let marketUpdateIndex = Helpers.randomTokenAmount();

      before_once'(() =>
        setup(~timeDelta=timeDeltaGreaterThanZero, ~marketUpdateIndex)
      );
    });

    describe("case marketUpdateIndex > 0", () => {
      let latestRewardIndex = Helpers.randomInteger();
      let marketUpdateIndex = Helpers.randomInteger();
      let pushUpdatedMarketPricesToUpdateFloatIssuanceCalculationsTxPromise =
        ref("Not set yet"->Obj.magic);

      before_once'(() => {
        let%Await _ =
          contracts.contents.staker
          ->Staker.Exposed.setLatestRewardIndexGlobals(
              ~marketIndex,
              ~latestRewardIndex,
            );

        pushUpdatedMarketPricesToUpdateFloatIssuanceCalculationsTxPromise :=
          setup(~timeDelta=timeDeltaGreaterThanZero, ~marketUpdateIndex);

        pushUpdatedMarketPricesToUpdateFloatIssuanceCalculationsTxPromise.
          contents;
      });
    });

    describe("", () => {
      let marketUpdateIndex = Helpers.randomTokenAmount();

      before_once'(() =>
        setup(~timeDelta=timeDeltaGreaterThanZero, ~marketUpdateIndex)
      );

      it(
        "calls calculateNewCumulativeIssuancePerStakedSynth with correct arguments",
        () => {
        let%Await _ = txReference.contents;
        StakerSmocked.InternalMock._calculateNewCumulativeIssuancePerStakedSynthCallCheck({
          marketIndex,
          previousMarketUpdateIndex: marketUpdateIndex->sub(oneBn),
          longPrice,
          shortPrice,
          longValue,
          shortValue,
        });
      });
      it("sets the latestRewardIndex correctly", () => {
        let%Await latestRewardIndex =
          contracts.contents.staker->Staker.latestRewardIndex(marketIndex);
        latestRewardIndex->Chai.bnEqual(marketUpdateIndex);
      });
      it("mutates accumulativeFloatPerSyntheticTokenSnapshots", () => {
        let%Await rewardParams =
          contracts^.staker
          ->Staker.accumulativeFloatPerSyntheticTokenSnapshots(
              marketIndex,
              marketUpdateIndex,
            );
        rewardParams->Chai.recordEqualFlat({
          timestamp: timestampRef.contents,
          accumulativeFloatPerSyntheticToken_long: longAccum,
          accumulativeFloatPerSyntheticToken_short: shortAccum,
        });
      });
      it("emits AccumulativeIssuancePerStakedSynthSnapshotCreated event", () => {
        Chai.callEmitEvents(
          ~call=txReference.contents,
          ~contract=contracts.contents.staker->Obj.magic,
          ~eventName="AccumulativeIssuancePerStakedSynthSnapshotCreated",
        )
        ->Chai.withArgs4(
            marketIndex,
            marketUpdateIndex,
            longAccum,
            shortAccum,
          )
      });
    });
  });
};
