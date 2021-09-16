open LetOps;
open Mocha;

let testUnit =
    (
      ~contracts: ref(Helpers.longShortUnitTestContracts),
      ~accounts as _: ref(array(Ethers.Wallet.t)),
    ) => {
  describe("executeOutstandingNextPriceSettlementsUserMulti", () => {
    it(
      "calls _executeOutstandingNextPriceSettlements with correct arguments for given array of market indexes",
      () => {
        let user = Helpers.randomAddress();
        let marketIndexes = [|1, 2, 3, 4|];

        let%Await _ =
          contracts.contents.longShort
          ->LongShortSmocked.InternalMock.setupFunctionForUnitTesting(
              ~functionName="executeOutstandingNextPriceSettlementsUserMulti",
            );

        let%Await _ =
          contracts.contents.longShort
          ->LongShort.Exposed.executeOutstandingNextPriceSettlementsUserMulti(
              ~user,
              ~marketIndexes,
            );

        let _ =
          LongShortSmocked.InternalMock._executeOutstandingNextPriceSettlementsCallCheck({
            user,
            marketIndex: 1,
          });
        let _ =
          LongShortSmocked.InternalMock._executeOutstandingNextPriceSettlementsCallCheck({
            user,
            marketIndex: 2,
          });
        let _ =
          LongShortSmocked.InternalMock._executeOutstandingNextPriceSettlementsCallCheck({
            user,
            marketIndex: 3,
          });
        LongShortSmocked.InternalMock._executeOutstandingNextPriceSettlementsCallCheck({
          user,
          marketIndex: 4,
        });
      },
    )
  });
};
