open Globals;
open LetOps;
open Mocha;
open SmockGeneral;

let test =
    (
      ~contracts: ref(Helpers.stakerUnitTestContracts),
      ~accounts: ref(array(Ethers.Wallet.t)),
    ) => {
  describe("addNewStakingFund", () => {
    let marketIndex = 1;
    let sampleLongAddress = Helpers.randomAddress();
    let sampleShortAddress = Helpers.randomAddress();
    let kInitialMultiplier =
      CONSTANTS.tenToThe18->add(Helpers.randomInteger());
    let kPeriod = Helpers.randomInteger();
    let unstakeFee_e18 = Helpers.randomInteger();

    let timestampRef = ref(0);

    let promiseRef: ref(Promise.t(ContractHelpers.transaction)) =
      ref(()->Promise.resolve->Obj.magic);

    before_once'(() => {
      let%Await _ =
        contracts^.staker
        ->StakerSmocked.InternalMock.setupFunctionForUnitTesting(
            ~functionName="addNewStakingFund",
          );

      let longShortAddress = (accounts^)->Array.getUnsafe(5);
      let%AwaitThen _ =
        contracts^.staker
        ->Staker.Exposed.setLongShort(~longShort=longShortAddress.address);

      let%AwaitThen {timestamp} = Helpers.getBlock();
      timestampRef := timestamp;
      let promise =
        contracts^.staker
        ->ContractHelpers.connect(~address=longShortAddress)
        ->Staker.Exposed.addNewStakingFund(
            ~marketIndex,
            ~longToken=sampleLongAddress,
            ~shortToken=sampleShortAddress,
            ~kInitialMultiplier,
            ~kPeriod,
            ~unstakeFee_e18,
            ~balanceIncentiveCurve_exponent=bnFromInt(5),
            ~balanceIncentiveCurve_equilibriumOffset=bnFromInt(0),
          );
      promiseRef := promise;
      promise;
    });

    it_skip("calls the onlyLongShortModifier", () => {
      expect(StakerSmocked.InternalMock.onlyLongShortModifierLogicFunction())
      ->toHaveCallCount(0)
    });

    it("calls _changeUnstakeFee with correct arguments", () => {
      StakerSmocked.InternalMock._changeUnstakeFeeCallCheck({
        marketIndex,
        newMarketUnstakeFee_e18: unstakeFee_e18,
      })
    });

    it("mutates accumulativeFloatPerSyntheticTokenSnapshots", () => {
      let%Await params =
        contracts^.staker
        ->Staker.accumulativeFloatPerSyntheticTokenSnapshots(
            1,
            CONSTANTS.zeroBn,
          );

      params->Chai.recordEqualFlat({
        timestamp: timestampRef^ + 1, // one second per block in hardhat
        accumulativeFloatPerSyntheticToken_long: CONSTANTS.zeroBn,
        accumulativeFloatPerSyntheticToken_short: CONSTANTS.zeroBn,
      });
    });

    it("mutates syntheticTokens", () => {
      let%Await tokenLong =
        contracts^.staker->Staker.syntheticTokens(1, true);
      let%Await tokenShort =
        contracts^.staker->Staker.syntheticTokens(1, false);

      Chai.addressEqual(~otherAddress=tokenLong, sampleLongAddress);
      Chai.addressEqual(~otherAddress=tokenShort, sampleShortAddress);
    });

    it("mutates marketIndexOfTokens", () => {
      let%AwaitThen longMarketIndex =
        contracts^.staker->Staker.marketIndexOfToken(sampleLongAddress);
      let%Await shortMarketIndex =
        contracts^.staker->Staker.marketIndexOfToken(sampleShortAddress);

      Chai.intEqual(marketIndex, longMarketIndex);
      Chai.intEqual(marketIndex, shortMarketIndex);
    });

    it("emits AccumulativeIssuancePerStakedSynthSnapshotCreated event", () => {
      Chai.callEmitEvents(
        ~call=promiseRef^,
        ~contract=contracts^.staker->Obj.magic,
        ~eventName="AccumulativeIssuancePerStakedSynthSnapshotCreated",
      )
      ->Chai.withArgs4(
          marketIndex,
          CONSTANTS.zeroBn,
          CONSTANTS.zeroBn,
          CONSTANTS.zeroBn,
        )
    });
  });
};
