open Globals;
open LetOps;
open Mocha;

let testUnit =
    (
      ~contracts: ref(Helpers.stakerUnitTestContracts),
      ~accounts: ref(array(Ethers.Wallet.t)),
    ) => {
  describe("Staker Admin Functions", () => {
    let marketIndex = Helpers.randomJsInteger();

    describe("changeFloatPercentage", () => {
      let newFloatPerc = bnFromString("42000000000000000");

      let txPromiseRef: ref(JsPromise.t(ContractHelpers.transaction)) =
        ref(()->JsPromise.resolve->Obj.magic);

      before_once'(() => {
        let%Await _ =
          contracts.contents.staker
          ->StakerSmocked.InternalMock.setupFunctionForUnitTesting(
              ~functionName="changeFloatPercentage",
            );

        txPromiseRef :=
          contracts.contents.staker
          ->Staker.Exposed.changeFloatPercentage(
              ~newFloatPercentage=newFloatPerc,
            );

        txPromiseRef.contents;
      });

      it("should call the onlyAdmin modifier", () => {
        StakerSmocked.InternalMock.onlyAdminModifierLogicCallCheck()
      });

      it("should call _changeFloatPercentage with correct argument", () => {
        StakerSmocked.InternalMock._changeFloatPercentageCallCheck({
          newFloatPercentage: newFloatPerc,
        })
      });

      it("emits FloatPercentageUpdated with correct argument", () => {
        Chai.callEmitEvents(
          ~call=txPromiseRef.contents,
          ~contract=contracts.contents.staker->Obj.magic,
          ~eventName="FloatPercentageUpdated",
        )
        ->Chai.withArgs2(newFloatPerc)
      });

      it("should revert if !(0 < newFloatPercentage <= 100 percent)", () => {
        let testValueWithinBounds = bnFromString("420000000000000000");
        let testValueOutOfBoundsLowSide = bnFromInt(0);
        let testValueOutOfBoundsHighSide =
          bnFromString("1010000000000000000");

        let%Await _ =
          contracts.contents.staker
          ->Staker.Exposed._changeFloatPercentageExposed(
              ~newFloatPercentage=testValueWithinBounds,
            );

        let%Await _ =
          Chai.expectRevert(
            ~transaction=
              contracts.contents.staker
              ->Staker.Exposed._changeFloatPercentageExposed(
                  ~newFloatPercentage=testValueOutOfBoundsLowSide,
                ),
            ~reason="",
          );

        Chai.expectRevert(
          ~transaction=
            contracts.contents.staker
            ->Staker.Exposed._changeFloatPercentageExposed(
                ~newFloatPercentage=testValueOutOfBoundsHighSide,
              ),
          ~reason="",
        );
      });

      it("should update floatPercentage correctly", () => {
        let randomNewFloatPerc =
          Helpers.randomInteger()->mul(bnFromString("10000000"));

        let%Await _ =
          contracts.contents.staker
          ->Staker.Exposed._changeFloatPercentageExposed(
              ~newFloatPercentage=randomNewFloatPerc,
            );

        let%Await floatPercAfterCall =
          contracts.contents.staker->Staker.Exposed.floatPercentage;

        Chai.bnEqual(randomNewFloatPerc, floatPercAfterCall);
      });
    });

    describe("changeUnstakeFee", () => {
      let unstakeFeeBasisPoints = Helpers.randomInteger();

      let txPromiseRef: ref(JsPromise.t(ContractHelpers.transaction)) =
        ref(()->JsPromise.resolve->Obj.magic);

      before_once'(() => {
        let%Await _ =
          contracts.contents.staker
          ->StakerSmocked.InternalMock.setupFunctionForUnitTesting(
              ~functionName="changeUnstakeFee",
            );

        txPromiseRef :=
          contracts.contents.staker
          ->Staker.Exposed.changeUnstakeFee(
              ~marketIndex,
              ~newMarketUnstakeFee_e18=unstakeFeeBasisPoints,
            );
        txPromiseRef.contents;
      });

      it("should call _changeUnstakeFee with correct arguments", () => {
        StakerSmocked.InternalMock._changeUnstakeFeeCallCheck({
          marketIndex,
          newMarketUnstakeFee_e18: unstakeFeeBasisPoints,
        })
      });

      it("should emit StakeWithdrawalFeeUpdated with correct arguments", () => {
        Chai.callEmitEvents(
          ~call=txPromiseRef.contents,
          ~contract=contracts.contents.staker->Obj.magic,
          ~eventName="StakeWithdrawalFeeUpdated",
        )
        ->Chai.withArgs2(marketIndex, unstakeFeeBasisPoints)
      });

      it("should not allow new unstake fee greater than 5 percent", () => {
        let adminWallet = accounts.contents->Array.getUnsafe(0);
        let sixPercent = bnFromString("60000000000000000");

        let%Await _ =
          Chai.expectRevert(
            ~transaction=
              contracts.contents.staker
              ->ContractHelpers.connect(~address=adminWallet)
              ->Staker.Exposed._changeUnstakeFeeExposed(
                  ~marketIndex,
                  ~newMarketUnstakeFee_e18=sixPercent,
                ),
            ~reason="",
          );
        ();
      });

      it("should update unstake fee correctly", () => {
        let adminWallet = accounts.contents->Array.getUnsafe(0);
        let newFeePercentageRandom =
          Helpers.randomInteger()->mul(bnFromString("10000000"));

        let%Await _ =
          contracts.contents.staker
          ->ContractHelpers.connect(~address=adminWallet)
          ->Staker.Exposed._changeUnstakeFeeExposed(
              ~marketIndex=1,
              ~newMarketUnstakeFee_e18=newFeePercentageRandom,
            );

        let%Await feeAfterCall =
          contracts.contents.staker->Staker.Exposed.marketUnstakeFee_e18(1);

        Chai.bnEqual(feeAfterCall, newFeePercentageRandom);
      });
    });

    describe("changeBalanceIncentiveParameters", () => {
      let marketIndex = 23;
      let startingTestExponent = bnFromInt(1);
      let updatedExponent = bnFromInt(2);
      let safeExponentBitShifting = bnFromInt(50);

      let txPromiseRef: ref(JsPromise.t(ContractHelpers.transaction)) =
        ref(()->JsPromise.resolve->Obj.magic);

      before_once'(() => {
        let%Await _ =
          contracts.contents.staker
          ->StakerSmocked.InternalMock.setupFunctionForUnitTesting(
              ~functionName="changeBalanceIncentiveParameters",
            );

        let stakerAddress = accounts.contents->Array.getUnsafe(5);

        txPromiseRef :=
          contracts.contents.staker
          ->ContractHelpers.connect(~address=stakerAddress)
          ->Staker.changeBalanceIncentiveParameters(
              ~marketIndex,
              ~balanceIncentiveCurve_exponent=startingTestExponent,
              ~balanceIncentiveCurve_equilibriumOffset=zeroBn,
              ~safeExponentBitShifting,
            );
        txPromiseRef.contents;
      });

      it("should call the onlyAdmin Modifier", () => {
        let%Await _ =
          contracts.contents.staker
          ->Staker.changeBalanceIncentiveParameters(
              ~marketIndex,
              ~balanceIncentiveCurve_exponent=updatedExponent,
              ~balanceIncentiveCurve_equilibriumOffset=zeroBn,
              ~safeExponentBitShifting,
            );
        StakerSmocked.InternalMock.onlyAdminModifierLogicCallCheck();
      });

      it(
        "should call _changeBalanceIncentiveExponent with correct arguments",
        () =>
        StakerSmocked.InternalMock._changeBalanceIncentiveParametersCallCheck({
          marketIndex,
          balanceIncentiveCurve_exponent: updatedExponent,
          balanceIncentiveCurve_equilibriumOffset: zeroBn,
          safeExponentBitShifting,
        })
      );

      it(
        "should emit BalanceIncentiveParamsUpdated with correct arguments", () => {
        Chai.callEmitEvents(
          ~call=txPromiseRef^,
          ~contract=contracts.contents.staker->Obj.magic,
          ~eventName="BalanceIncentiveParamsUpdated",
        )
        ->Chai.withArgs4(
            marketIndex,
            startingTestExponent,
            zeroBn,
            safeExponentBitShifting,
          )
      });

      it("should only allow (0 < new exponent)", () => {
        let adminWallet = accounts.contents->Array.getUnsafe(0);
        let newExponentOutOfBoundsLowSide = bnFromInt(0);

        Chai.expectRevert(
          ~transaction=
            contracts.contents.staker
            ->ContractHelpers.connect(~address=adminWallet)
            ->Staker.Exposed._changeBalanceIncentiveParametersExposed(
                ~marketIndex,
                ~balanceIncentiveCurve_exponent=newExponentOutOfBoundsLowSide,
                ~balanceIncentiveCurve_equilibriumOffset=zeroBn,
                ~safeExponentBitShifting,
              ),
          ~reason="",
        );
      });

      it("should ensure (-9e17 < new equilibrium offset < 9e17)", () => {
        let adminWallet = accounts.contents->Array.getUnsafe(0);
        let balanceIncentiveCurve_exponent = bnFromInt(3);
        let newOffsetOutOfBoundsHighSide =
          bnFromString("900000000000000000")
          ->add(Helpers.randomTokenAmount());
        let newOffsetOutOfBoundsLowSide =
          bnFromString("-900000000000000000")
          ->sub(Helpers.randomTokenAmount());

        let%Await _ =
          Chai.expectRevert(
            ~transaction=
              contracts.contents.staker
              ->ContractHelpers.connect(~address=adminWallet)
              ->Staker.Exposed._changeBalanceIncentiveParametersExposed(
                  ~marketIndex,
                  ~balanceIncentiveCurve_exponent,
                  ~balanceIncentiveCurve_equilibriumOffset=newOffsetOutOfBoundsHighSide,
                  ~safeExponentBitShifting,
                ),
            ~reason="",
          );

        let%Await _ =
          Chai.expectRevert(
            ~transaction=
              contracts.contents.staker
              ->ContractHelpers.connect(~address=adminWallet)
              ->Staker.Exposed._changeBalanceIncentiveParametersExposed(
                  ~marketIndex,
                  ~balanceIncentiveCurve_exponent,
                  ~balanceIncentiveCurve_equilibriumOffset=newOffsetOutOfBoundsLowSide,
                  ~safeExponentBitShifting,
                ),
            ~reason="",
          );
        ();
      });

      it("should update incentive exponent correctly", () => {
        let adminWallet = accounts.contents->Array.getUnsafe(0);
        let newExponent = bnFromInt(4);

        let {staker, longShortSmocked} = contracts.contents;

        let _ =
          longShortSmocked->LongShortSmocked.mockMarketSideValueInPaymentTokenToReturn(
            CONSTANTS.tenToThe18,
          );

        let%Await _ =
          staker->Staker.setVariable(
            ~name="longShort",
            ~value=longShortSmocked.address,
          );

        let%Await _ =
          staker
          ->ContractHelpers.connect(~address=adminWallet)
          ->Staker.Exposed._changeBalanceIncentiveParametersExposed(
              ~marketIndex,
              ~balanceIncentiveCurve_exponent=newExponent,
              ~balanceIncentiveCurve_equilibriumOffset=zeroBn,
              ~safeExponentBitShifting,
            );

        let%Await exponentAfterCall =
          staker->Staker.Exposed.balanceIncentiveCurve_exponent(marketIndex);

        Chai.bnEqual(exponentAfterCall, newExponent);
      });
    });
  });
};
