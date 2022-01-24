open Mocha;
open Globals;
open LetOps;

let oneYearInDays = "31557600"->bnFromString; // 365.25 days in seconds

let testUnit =
    (
      ~contracts: ref(Helpers.longShortUnitTestContracts),
      ~accounts as _: ref(array(Ethers.Wallet.t)),
    ) => {
  describeUnit("_claimAndDistributeYieldThenRebalanceMarket", () => {
    let marketIndex = Helpers.randomJsInteger();

    let longAmount = Helpers.randomTokenAmount();
    let shortAmount = Helpers.randomTokenAmount();

    let fundingRateMultiplier_e18 =
      Helpers.randomBigIntInRange(5, 5000)
      ->Ethers.BigNumber.mul(
          Ethers.BigNumber.fromUnsafe("10000000000000000"),
        );

    let overBalanced =
      longAmount->Ethers.BigNumber.gt(shortAmount) ? longAmount : shortAmount;
    let underBalanced =
      longAmount->Ethers.BigNumber.gt(shortAmount) ? shortAmount : longAmount;

    let timestampRef = ref(zeroBn);

    let setup = () => {
      let longShort = contracts.contents.longShort;
      let staker = contracts.contents.stakerSmocked;
      let%AwaitThen timestamp = Helpers.getRandomTimestampInPast();
      timestampRef := timestamp;

      let%AwaitThen _ =
        longShort->LongShortStateSetters.setStaker(
          ~stakerAddress=staker.address,
        );

      staker->StakerSmocked.mockSafe_getUpdateTimestampToReturn(timestamp);

      longShort->LongShort.changeMarketFundingRateMultiplier(
        ~marketIndex,
        ~fundingRateMultiplier_e18,
      );
    };

    it("calculates a funding rate", _ => {
      let%AwaitThen _ = setup();

      let%AwaitThen {timestamp} = Helpers.getBlock();
      let%Await fundingRate =
        contracts.contents.longShort
        ->LongShort.Exposed._calculateFundingAmountExposedCall(
            ~marketIndex,
            ~fundingRateMultiplier_e18,
            ~overbalancedValue=overBalanced,
            ~underbalancedValue=underBalanced,
          );

      let expectedFundingRate =
        overBalanced
        ->sub(underBalanced)
        ->mul(fundingRateMultiplier_e18)
        ->mul(timestamp->bnFromInt->sub(timestampRef.contents))
        ->div(oneYearInDays->mul(tenToThe18));

      Chai.bnEqual(expectedFundingRate, fundingRate);
    });
  });
};
