open LetOps;
open Mocha;

open Globals;

let test =
    (
      ~contracts: ref(Helpers.stakerUnitTestContracts),
      ~accounts as _: ref(array(Ethers.Wallet.t)),
    ) => {
  let marketIndex = Helpers.randomJsInteger();

  let (longFloatPerSecond, shortFloatPerSecond, latestRewardIndexForMarket) =
    Helpers.Tuple.make3(Helpers.randomInteger);
  let timestampRef = ref(CONSTANTS.zeroBn);

  // NOTE: these numbers have to be bigger than 2^80 otherwise they become insignificant after the bitshift.
  let accumFloatLong =
    Helpers.randomBigIntInRange(1, 1000)->mul(CONSTANTS.eightyBitShift);
  let accumFloatShort =
    Helpers.randomBigIntInRange(1, 1000)->mul(CONSTANTS.eightyBitShift);

  let (longValue, shortValue, longPrice, shortPrice) =
    Helpers.Tuple.make4(Helpers.randomInteger);

  describe("calculateNewCumulativeValue", () => {
    let promiseRef:
      ref(
        Promise.t(
          Staker.Exposed._calculateNewCumulativeIssuancePerStakedSynthExposedReturn,
        ),
      ) =
      ref(None->Obj.magic);
    before_once'(() => {
      let {staker} = contracts.contents;

      let%AwaitThen timestamp = Helpers.getRandomTimestampInPast();
      timestampRef := timestamp;

      let%AwaitThen _ =
        staker->StakerSmocked.InternalMock.setupFunctionForUnitTesting(
          ~functionName="calculateNewCumulativeValue",
        );

      StakerSmocked.InternalMock.mock_calculateFloatPerSecondToReturn(
        longFloatPerSecond,
        shortFloatPerSecond,
      );

      let%Await _ =
        staker->Staker.Exposed.setCalculateNewCumulativeRateParams(
          ~marketIndex,
          ~latestRewardIndexForMarket,
          ~accumFloatLong,
          ~accumFloatShort,
          ~timestamp=timestamp->Ethers.BigNumber.toNumber,
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
      promiseRef^;
    });

    it(
      "returns the old cumulative float + (timeDelta * floatPerSecond) for each market side",
      () => {
        let%AwaitThen {timestamp: timestampLatest} = Helpers.getBlock();

        let timeDelta =
          Ethers.BigNumber.fromInt(timestampLatest)
          ->Ethers.BigNumber.sub(timestampRef^);

        let mockFn = (~oldCumulative, ~timeDelta, ~fps) =>
          oldCumulative
          ->Ethers.BigNumber.mul(CONSTANTS.eightyBitShift)
          ->Ethers.BigNumber.add(timeDelta->Ethers.BigNumber.mul(fps));
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
  });
};
