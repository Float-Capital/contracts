open LetOps;
open Mocha;

open Globals;

let test =
    (
      ~contracts: ref(Helpers.stakerUnitTestContracts),
      ~accounts as _: ref(array(Ethers.Wallet.t)),
    ) => {
  let marketIndex = Helpers.randomJsInteger();

  let (
    longFloatPerSecond,
    shortFloatPerSecond,
    latestRewardIndexForMarket,
    accumFloatLong,
    accumFloatShort,
    timeDelta,
  ) =
    Helpers.Tuple.make6(Helpers.randomInteger);
  let (longValue, shortValue, longPrice, shortPrice) =
    Helpers.Tuple.make4(Helpers.randomInteger);

  describe("calculateNewCumulativeValue", () => {
    let promiseRef:
      ref(
        JsPromise.t(
          Staker.Exposed._calculateNewCumulativeIssuancePerStakedSynthExposedReturn,
        ),
      ) =
      ref(None->Obj.magic);
    before_once'(() => {
      let {staker} = contracts.contents;

      let%AwaitThen _ =
        staker->StakerSmocked.InternalMock.setupFunctionForUnitTesting(
          ~functionName="calculateNewCumulativeValue",
        );

      StakerSmocked.InternalMock.mock_calculateFloatPerSecondToReturn(
        longFloatPerSecond,
        shortFloatPerSecond,
      );

      StakerSmocked.InternalMock.mock_calculateTimeDeltaFromLastAccumulativeIssuancePerStakedSynthSnapshotToReturn(
        timeDelta,
      );

      let%Await _ =
        staker->Staker.Exposed.setCalculateNewCumulativeRateParams(
          ~marketIndex,
          ~latestRewardIndexForMarket,
          ~accumFloatLong,
          ~accumFloatShort,
        );

      promiseRef :=
        staker->Staker.Exposed._calculateNewCumulativeIssuancePerStakedSynthExposed(
          ~marketIndex,
          ~previousMarketUpdateIndex=latestRewardIndexForMarket,
          ~longPrice,
          ~shortPrice,
          ~longValue,
          ~shortValue,
        );
      let%Await _ = promiseRef^;
      ();
    });

    it(
      "returns the old cumulative float + (timeDelta * floatPerSecond) for each market side",
      () => {
        let mockFn = (~oldCumulative, ~timeDelta, ~fps) =>
          oldCumulative->Ethers.BigNumber.add(
            timeDelta->Ethers.BigNumber.mul(fps),
          );
        let%Await result = promiseRef^;
        let longCumulative: Ethers.BigNumber.t =
          result->Obj.magic->Array.getUnsafe(0);

        let shortCumulative: Ethers.BigNumber.t =
          result->Obj.magic->Array.getUnsafe(1);

        longCumulative->Chai.bnEqual(
          mockFn(
            ~oldCumulative=accumFloatLong,
            ~timeDelta,
            ~fps=longFloatPerSecond,
          ),
        );
        shortCumulative->Chai.bnEqual(
          mockFn(
            ~oldCumulative=accumFloatShort,
            ~timeDelta,
            ~fps=shortFloatPerSecond,
          ),
        );
      },
    );

    it("calls calculateFloatPerSecond with correct arguments", () => {
      StakerSmocked.InternalMock._calculateFloatPerSecondCallCheck({
        marketIndex,
        longPrice,
        shortPrice,
        longValue,
        shortValue,
      })
    });

    it("calls calculateTimeDelta with correct arguments", () => {
      StakerSmocked.InternalMock._calculateTimeDeltaFromLastAccumulativeIssuancePerStakedSynthSnapshotCallCheck({
        marketIndex,
        previousMarketUpdateIndex: latestRewardIndexForMarket,
      })
    });
  });
};
