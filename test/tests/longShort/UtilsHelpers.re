open Mocha;
open Globals;
open LetOps;

let testUnit =
    (
      ~contracts: ref(Helpers.longShortUnitTestContracts),
      ~accounts as _: ref(array(Ethers.Wallet.t)),
    ) => {
  describe("Long Short Utilities and helpers", () => {
    describe("_getYieldSplit", () => {
      let marketIndex = 1;

      let test =
          (
            ~marketSideValueInPaymentTokenLong,
            ~marketSideValueInPaymentTokenShort,
          ) => {
        let totalValueLockedInMarket =
          marketSideValueInPaymentTokenLong->add(
            marketSideValueInPaymentTokenShort,
          );

        let isLongSideUnderbalanced =
          marketSideValueInPaymentTokenShort->bnGte(
            marketSideValueInPaymentTokenLong,
          );

        let imbalance =
          isLongSideUnderbalanced
            ? marketSideValueInPaymentTokenShort->sub(
                marketSideValueInPaymentTokenLong,
              )
            : marketSideValueInPaymentTokenLong->sub(
                marketSideValueInPaymentTokenShort,
              );

        let%AwaitThen marketTreasurySplitGradient_e18 =
          contracts.contents.longShort
          ->LongShort.marketTreasurySplitGradient_e18(marketIndex);

        let marketPercentCalculated_e18 =
          imbalance
          ->mul(marketTreasurySplitGradient_e18)
          ->div(totalValueLockedInMarket);
        let marketPercent_e18 =
          bnMin(marketPercentCalculated_e18, CONSTANTS.tenToThe18);

        let treasuryPercent_e18 =
          CONSTANTS.tenToThe18->sub(marketPercent_e18);

        let expectedResult = treasuryPercent_e18;

        let%Await actualResult =
          contracts.contents.longShort
          ->LongShort.Exposed._getYieldSplitExposed(
              ~marketIndex,
              ~longValue=marketSideValueInPaymentTokenLong,
              ~shortValue=marketSideValueInPaymentTokenShort,
              ~totalValueLockedInMarket,
            );
        Chai.bnEqual(
          ~message=
            "expectedResult and result after `_getYieldSplit` not the same",
          expectedResult,
          actualResult.treasuryYieldPercent_e18,
        );
      };

      it("works as expected if longValue > shortValue", () => {
        let marketSideValueInPaymentTokenShort = Helpers.randomTokenAmount();
        let marketSideValueInPaymentTokenLong =
          marketSideValueInPaymentTokenShort->add(
            Helpers.randomTokenAmount(),
          );

        test(
          ~marketSideValueInPaymentTokenLong,
          ~marketSideValueInPaymentTokenShort,
        );
      });
      it("works as expected if shortValue > longValue", () => {
        let marketSideValueInPaymentTokenLong = Helpers.randomTokenAmount();
        let marketSideValueInPaymentTokenShort =
          marketSideValueInPaymentTokenLong->add(Helpers.randomTokenAmount());

        test(
          ~marketSideValueInPaymentTokenLong,
          ~marketSideValueInPaymentTokenShort,
        );
      });
      it("works as expected if shortValue == longValue", () => {
        let marketSideValueInPaymentTokenLong = Helpers.randomTokenAmount();
        let marketSideValueInPaymentTokenShort = marketSideValueInPaymentTokenLong;

        test(
          ~marketSideValueInPaymentTokenLong,
          ~marketSideValueInPaymentTokenShort,
        );
      });
    })
  });
};
