open Globals;
open LetOps;
open Mocha;
open SmockGeneral;

let test =
    (
      ~contracts: ref(Helpers.stakerUnitTestContracts),
      ~accounts: ref(array(Ethers.Wallet.t)),
    ) => {
  describe("stakeFromUser", () => {
    let marketIndexForToken = Helpers.randomJsInteger();

    let latestRewardIndex =
      Js.Math.random_int(2, Js.Int.max)->Ethers.BigNumber.fromInt;
    let randomRewardIndexBelow = num =>
      Js.Math.random_int(1, num->Ethers.BigNumber.toNumber)
      ->Ethers.BigNumber.fromInt;

    let from = Helpers.randomAddress();
    let mockTokenWalletRef: ref(Ethers.Wallet.t) = ref(None->Obj.magic);

    let (userAmountStaked, userAmountToStake) =
      Helpers.Tuple.make2(Helpers.randomTokenAmount); // will be at least two

    let promiseRef: ref(Promise.t(ContractHelpers.transaction)) =
      ref(None->Obj.magic);

    let setup = (~userLastRewardIndex, ~latestRewardIndex) => {
      let {staker, longShortSmocked} = contracts.contents;

      let%Await _ =
        staker->StakerSmocked.InternalMock.setupFunctionForUnitTesting(
          ~functionName="stakeFromUser",
        );

      mockTokenWalletRef := accounts.contents->Array.getExn(6);

      let%Await _ =
        contracts.contents.staker
        ->Staker.Exposed.setStakeFromUserParams(
            ~longshort=longShortSmocked.address,
            ~token=mockTokenWalletRef.contents.address,
            ~marketIndexForToken,
            ~user=from,
            ~latestRewardIndex,
            ~userAmountStaked,
            ~userLastRewardIndex,
          );

      let promise =
        contracts.contents.staker
        ->ContractHelpers.connect(~address=mockTokenWalletRef.contents)
        ->Staker.stakeFromUser(~from, ~amount=userAmountToStake);
      promiseRef := promise;
      promise;
    };

    it_skip("calls onlyValidSynthetic with correct args", () => {
      // StakerSmocked.InternalMock.onlyValidSyntheticCallCheck()
      // ->Array.getExn(0)
      // ->Chai.recordEqualFlat({synth: mockTokenWalletRef.contents.address})
      ()
    });

    it("calls updateSystemState on longshort with correct args", () => {
      let {longShortSmocked} = contracts.contents;

      let%Await _ =
        setup(
          ~userLastRewardIndex=randomRewardIndexBelow(latestRewardIndex),
          ~latestRewardIndex,
        );

      longShortSmocked->LongShortSmocked.updateSystemStateCallCheck({
        marketIndex: marketIndexForToken,
      });
    });

    describe("case user has outstanding float to be minted", () => {
      before_once'(() =>
        setup(
          ~userLastRewardIndex=randomRewardIndexBelow(latestRewardIndex),
          ~latestRewardIndex,
        )
      );

      it(
        "calls updateUsersStakedPosition_mintAccumulatedFloat with correct args",
        () => {
        StakerSmocked.InternalMock._updateUsersStakedPosition_mintAccumulatedFloatAndExecuteOutstandingNextPriceActionsCallCheck({
          marketIndex: marketIndexForToken,
          user: from,
        })
      });
      it("mutates userAmountStaked", () => {
        let%Await amountStaked =
          contracts.contents.staker
          ->Staker.userAmountStaked(
              mockTokenWalletRef.contents.address,
              from,
            );
        amountStaked->Chai.bnEqual(
          userAmountStaked->Ethers.BigNumber.add(userAmountToStake),
        );
      });

      it("mutates userIndexOfLastClaimedReward", () => {
        let%Await lastClaimedReward =
          contracts.contents.staker
          ->Staker.userIndexOfLastClaimedReward(marketIndexForToken, from);

        lastClaimedReward->Chai.bnEqual(latestRewardIndex);
      });

      it("emits StakeAdded", () =>
        Chai.callEmitEvents(
          ~call=promiseRef.contents,
          ~contract=contracts.contents.staker->Obj.magic,
          ~eventName="StakeAdded",
        )
        ->Chai.withArgs4(
            from,
            mockTokenWalletRef.contents.address,
            userAmountToStake,
            latestRewardIndex,
          )
      );
    });

    // next two cases still do everything except call mintFloat but unwieldy to test
    describe("case user has last claimed index of 0", () => {
      before_once'(() =>
        setup(
          ~userLastRewardIndex=CONSTANTS.zeroBn,
          ~latestRewardIndex=Helpers.randomInteger(),
        )
      );

      it("doesn't call mintAccumulatedFloat", () => {
        expect(
          StakerSmocked.InternalMock._mintAccumulatedFloatAndExecuteOutstandingActionsFunction(),
        )
        ->toHaveCallCount(0)
      });
    });

    describe(
      "case users last claimed index == latestRewardIndex for market", () => {
      let index = Helpers.randomInteger();
      before_once'(() =>
        setup(~userLastRewardIndex=index, ~latestRewardIndex=index)
      );

      it("doesn't call mintAccumulatedFloat", () => {
        expect(
          StakerSmocked.InternalMock._mintAccumulatedFloatAndExecuteOutstandingActionsFunction(),
        )
        ->toHaveCallCount(0)
      });
    });
  });
};
