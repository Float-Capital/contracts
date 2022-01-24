open Globals;
open LetOps;
open Mocha;
open SmockGeneral;

let test =
    (
      ~contracts: ref(Helpers.stakerUnitTestContracts),
      ~accounts as _: ref(array(Ethers.Wallet.t)),
    ) => {
  describe("mintAccumulatedFloat", () => {
    let marketIndex = Helpers.randomJsInteger();

    let (floatToMintLong, floatToMintShort, latestRewardIndexForMarket) =
      Helpers.Tuple.make3(Helpers.randomTokenAmount);

    let user = Helpers.randomAddress();

    let promiseRef: ref(Promise.t(ContractHelpers.transaction)) =
      ref(None->Obj.magic);

    let setup = (~floatToMintLong, ~floatToMintShort) => {
      let {staker} = contracts.contents;

      let%AwaitThen _ =
        staker->StakerSmocked.InternalMock.setupFunctionForUnitTesting(
          ~functionName="mintAccumulatedFloat",
        );

      StakerSmocked.InternalMock.mock_calculateAccumulatedFloatAndExecuteOutstandingActionsToReturn(
        floatToMintLong->add(floatToMintShort),
      );

      let%AwaitThen _ =
        staker->Staker.Exposed.setMintAccumulatedFloatAndClaimFloatParams(
          ~marketIndex,
          ~latestRewardIndexForMarket,
        );

      promiseRef :=
        contracts^.staker
        ->Staker.Exposed._mintAccumulatedFloatAndExecuteOutstandingActionsExposed(
            ~marketIndex,
            ~user,
          );

      let%Await _ = promiseRef^;
      promiseRef^;
    };

    describe("case floatToMint > 0", () => {
      before_once'(() => setup(~floatToMintLong, ~floatToMintShort));

      it("calls calculateAccumulatedFloat with correct arguments", () =>
        StakerSmocked.InternalMock._calculateAccumulatedFloatAndExecuteOutstandingActionsCallCheck({
          marketIndex,
          user,
        })
      );

      it("calls mintFloat with correct arguments", () =>
        StakerSmocked.InternalMock._mintFloatCallCheck({
          user,
          floatToMint:
            floatToMintLong->Ethers.BigNumber.add(floatToMintShort),
        })
      );

      it("emits FloatMinted event", () => {
        Chai.callEmitEvents(
          ~call=promiseRef^,
          ~contract=contracts^.staker->Obj.magic,
          ~eventName="FloatMinted",
        )
        ->Chai.withArgs3(
            user,
            marketIndex,
            floatToMintLong->add(floatToMintShort),
          )
      });
    });

    describe("case floatToMint == 0", () => {
      before_once'(() => {
        let {staker} = contracts.contents;
        let%Await _ =
          staker->StakerStakeSetters.setUserIndexOfLastClaimedReward(
            ~marketIndex,
            ~user,
            ~rewardIndex=zeroBn,
          );

        setup(
          ~floatToMintLong=CONSTANTS.zeroBn,
          ~floatToMintShort=CONSTANTS.zeroBn,
        );
      });

      it("calls calculateAccumulatedFloat with correct arguments", () =>
        StakerSmocked.InternalMock._calculateAccumulatedFloatAndExecuteOutstandingActionsCallCheck({
          marketIndex,
          user,
        })
      );

      it("doesn't call mintFloat", () =>
        expect(StakerSmocked.InternalMock._mintFloatFunction())
        ->toHaveCallCount(0)
      );

      it("doesn't mutate userIndexOfLastClaimed", () => {
        let%Await lastClaimed =
          contracts^.staker
          ->Staker.userIndexOfLastClaimedReward(marketIndex, user);
        lastClaimed->Chai.bnEqual(zeroBn);
      });

      it("doesn't emit FloatMinted event", () => {
        Chai.callEmitEvents(
          ~call=promiseRef^,
          ~contract=contracts^.staker->Obj.magic,
          ~eventName="FloatMinted",
        )
        ->Chai.expectToNotEmit
      });
    });
  });
};
