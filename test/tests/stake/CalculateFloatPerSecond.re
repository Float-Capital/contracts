open LetOps;
open Mocha;
open Globals;

let test =
    (
      ~contracts: ref(Helpers.stakerUnitTestContracts),
      ~accounts as _: ref(array(Ethers.Wallet.t)),
    ) => {
  let marketIndex = 1;
  let tenToThe14 = tenToThe18->div(bnFromInt(10000)); // 0.01 %
  let tenToThe16 = tenToThe18->div(bnFromInt(100)); // 1 %

  let (kVal, longPrice, shortPrice) =
    Helpers.Tuple.make3(Helpers.randomTokenAmount);
  let (randomValueLocked1, randomValueLocked2) =
    Helpers.Tuple.make2(() => Helpers.randomJsInteger() / 10 + 1);

  describe("calculateFloatPerSecond", () => {
    let calculateFloatPerSecondPerPaymentTokenLocked =
        (
          ~underBalancedSideValue,
          ~exponent,
          ~equilibriumOffsetMarketScaled,
          ~totalLocked,
          ~requiredBitShifting,
          ~equilibriumMultiplier,
        ) => {
      let overflowProtectionDivision = twoBn->pow(requiredBitShifting);

      let numerator =
        underBalancedSideValue
        ->add(equilibriumOffsetMarketScaled->mul(equilibriumMultiplier))
        ->div(overflowProtectionDivision->div(twoBn))
        ->pow(exponent);

      let denominator =
        totalLocked->div(overflowProtectionDivision)->pow(exponent);

      let overBalancedSideRate =
        numerator->mul(tenToThe18)->div(denominator)->div(twoBn);

      let underBalancedSideRate = tenToThe18->sub(overBalancedSideRate);

      Chai.expectTrue(underBalancedSideRate->bnGte(overBalancedSideRate));
      (overBalancedSideRate, underBalancedSideRate);
    };

    let balanceIncentiveCurve_exponent = bnFromInt(5);
    let safeExponentBitShifting = bnFromInt(50);

    before(() => {
      let {staker, longShortSmocked} = contracts.contents;

      let%AwaitThen _ =
        staker->StakerSmocked.InternalMock.setupFunctionForUnitTesting(
          ~functionName="_calculateFloatPerSecond",
        );

      longShortSmocked->LongShortSmocked.mockMarketSideValueInPaymentTokenToReturn((
        CONSTANTS.tenToThe18,
        CONSTANTS.tenToThe18,
      ));

      let%Await _ =
        staker->Staker.Exposed._changeBalanceIncentiveParametersExposed(
          ~marketIndex,
          ~balanceIncentiveCurve_exponent,
          ~balanceIncentiveCurve_equilibriumOffset=CONSTANTS.zeroBn,
          ~safeExponentBitShifting,
        );

      StakerSmocked.InternalMock.mock_getKValueToReturn(kVal);
    });

    let runTestsAndReturnUnscaledFpsValuesE18 =
        (~longValueUnscaled, ~shortValueUnscaled) => {
      let longValue = bnFromInt(longValueUnscaled)->mul(tenToThe18);
      let shortValue = bnFromInt(shortValueUnscaled)->mul(tenToThe18);

      let totalLocked = longValue->add(shortValue);

      let%AwaitThen balanceIncentiveCurve_equilibriumOffset =
        contracts.contents.staker
        ->Staker.balanceIncentiveCurve_equilibriumOffset(marketIndex);

      let equilibriumOffsetMarketScaled =
        balanceIncentiveCurve_equilibriumOffset
        ->mul(totalLocked)
        ->div(twoBn)
        ->div(tenToThe18);

      let%Await result =
        contracts.contents.staker
        ->Staker.Exposed._calculateFloatPerSecondExposed(
            ~marketIndex,
            ~longPrice,
            ~shortPrice,
            ~longValue,
            ~shortValue,
          );

      let longFloatPerSecond: Ethers.BigNumber.t = result.longFloatPerSecond;
      let shortFloatPerSecond: Ethers.BigNumber.t = result.shortFloatPerSecond;

      let longRateScaled = ref(None->Obj.magic);
      let shortRateScaled = ref(None->Obj.magic);

      if (longValue->bnGte(
            shortValue->sub(equilibriumOffsetMarketScaled->mul(twoBn)),
          )) {
        if (equilibriumOffsetMarketScaled->bnGte(shortValue)) {
          shortRateScaled := kVal->mul(shortPrice);
          longRateScaled := zeroBn;
        } else {
          let (longRate, shortRate) =
            calculateFloatPerSecondPerPaymentTokenLocked(
              ~underBalancedSideValue=shortValue,
              ~exponent=balanceIncentiveCurve_exponent,
              ~equilibriumOffsetMarketScaled,
              ~totalLocked,
              ~requiredBitShifting=safeExponentBitShifting,
              ~equilibriumMultiplier=bnFromInt(-1),
            );

          longRateScaled :=
            longRate->mul(kVal)->mul(longPrice)->div(tenToThe18);
          shortRateScaled :=
            shortRate->mul(kVal)->mul(shortPrice)->div(tenToThe18);
        };
      } else if (equilibriumOffsetMarketScaled
                 ->mul(bnFromInt(-1))
                 ->bnGte(longValue)) {
        shortRateScaled := zeroBn;
        longRateScaled := kVal->mul(longPrice);
      } else {
        let (shortRate, longRate) =
          calculateFloatPerSecondPerPaymentTokenLocked(
            ~underBalancedSideValue=longValue,
            ~exponent=balanceIncentiveCurve_exponent,
            ~equilibriumOffsetMarketScaled,
            ~totalLocked,
            ~requiredBitShifting=safeExponentBitShifting,
            ~equilibriumMultiplier=oneBn,
          );

        longRateScaled :=
          longRate->mul(kVal)->mul(longPrice)->div(tenToThe18);
        shortRateScaled :=
          shortRate->mul(kVal)->mul(shortPrice)->div(tenToThe18);
      };

      Chai.bnEqual(
        ~message=
          "[runTestsAndReturnUnscaledFpsValuesE18] unexpected longFloatPerSecond result",
        longFloatPerSecond,
        longRateScaled^,
      );
      Chai.bnEqual(
        ~message=
          "[runTestsAndReturnUnscaledFpsValuesE18] unexpected shortFloatPerSecond result",
        shortFloatPerSecond,
        shortRateScaled^,
      );

      let longFloatPerSecondUnscaledE18 =
        longFloatPerSecond->mul(tenToThe18)->div(kVal)->div(longPrice);
      let shortFloatPerSecondUnscaledE18 =
        shortFloatPerSecond->mul(tenToThe18)->div(kVal)->div(shortPrice);

      (longFloatPerSecondUnscaledE18, shortFloatPerSecondUnscaledE18);
    };

    describe(
      "returns correct longFloatPerSecond and shortFloatPerSecond for each market side",
      () => {
        describe("without offset", () => {
          before(() => {
            contracts.contents.staker
            ->Staker.Exposed.setEquilibriumOffset(
                ~marketIndex,
                ~balanceIncentiveCurve_equilibriumOffset=zeroBn,
              )
          });
          it("longValue > shortValue", () => {
            runTestsAndReturnUnscaledFpsValuesE18(
              ~longValueUnscaled=randomValueLocked1 + randomValueLocked2,
              ~shortValueUnscaled=randomValueLocked2,
            )
          });
          it("longValue < shortValue", () => {
            runTestsAndReturnUnscaledFpsValuesE18(
              ~longValueUnscaled=randomValueLocked1,
              ~shortValueUnscaled=randomValueLocked1 + randomValueLocked2,
            )
          });
          it("has a continuous curve through intersection point", () => {
            // note that the middle is at L=500 & S=500 when we have a 50% offset
            let%Await (longFps1UnscaledE18, shortFps1UnscaledE18) =
              runTestsAndReturnUnscaledFpsValuesE18(
                ~longValueUnscaled=499,
                ~shortValueUnscaled=501,
              );

            // long side should be >50% and short side <50%
            Chai.expectTrue(
              longFps1UnscaledE18->bnGt(tenToThe18->div(twoBn)),
            );
            Chai.expectTrue(
              shortFps1UnscaledE18->bnLt(tenToThe18->div(twoBn)),
            );

            let%Await (longFps2UnscaledE18, shortFps2UnscaledE18) =
              runTestsAndReturnUnscaledFpsValuesE18(
                ~longValueUnscaled=501,
                ~shortValueUnscaled=499,
              );

            // short side should be <50% and long side >50%
            Chai.expectTrue(
              longFps2UnscaledE18->bnLt(tenToThe18->div(twoBn)),
            );
            Chai.expectTrue(
              shortFps2UnscaledE18->bnGt(tenToThe18->div(twoBn)),
            );

            Chai.expectTrue(
              longFps1UnscaledE18
              ->sub(longFps2UnscaledE18)
              ->bnLt(tenToThe16) // deviation of 1%
            );
            Chai.expectTrue(
              shortFps2UnscaledE18
              ->sub(shortFps1UnscaledE18)
              ->bnLt(tenToThe16) // deviation of 1%
            );
          });
        });
        describe("with negative offset", () => {
          before_once'(() => {
            contracts.contents.staker
            ->Staker.Exposed.setEquilibriumOffset(
                ~marketIndex,
                ~balanceIncentiveCurve_equilibriumOffset=
                  bnFromInt(-1)->mul(tenToThe18)->div(twoBn),
              )
          });
          it("longValue > shortValue", () => {
            runTestsAndReturnUnscaledFpsValuesE18(
              ~longValueUnscaled=randomValueLocked1 + randomValueLocked2,
              ~shortValueUnscaled=randomValueLocked2,
            )
          });
          it("longValue < shortValue", () => {
            runTestsAndReturnUnscaledFpsValuesE18(
              ~longValueUnscaled=randomValueLocked1,
              ~shortValueUnscaled=randomValueLocked1 + randomValueLocked2,
            )
          });
          it("has a continuous curve through intersection point", () => {
            // note that the middle is at L=750 & S=250 when we have a -50% offset
            let%Await (longFps1UnscaledE18, shortFps1UnscaledE18) =
              runTestsAndReturnUnscaledFpsValuesE18(
                ~longValueUnscaled=749,
                ~shortValueUnscaled=251,
              );

            // long side should be >50% and short side <50%
            Chai.expectTrue(
              longFps1UnscaledE18->bnGt(tenToThe18->div(twoBn)),
            );
            Chai.expectTrue(
              shortFps1UnscaledE18->bnLt(tenToThe18->div(twoBn)),
            );

            let%Await (longFps2UnscaledE18, shortFps2UnscaledE18) =
              runTestsAndReturnUnscaledFpsValuesE18(
                ~longValueUnscaled=751,
                ~shortValueUnscaled=249,
              );

            // short side should be <50% and long side >50%
            Chai.expectTrue(
              longFps2UnscaledE18->bnLt(tenToThe18->div(twoBn)),
            );
            Chai.expectTrue(
              shortFps2UnscaledE18->bnGt(tenToThe18->div(twoBn)),
            );

            Chai.expectTrue(
              longFps1UnscaledE18
              ->sub(longFps2UnscaledE18)
              ->bnLt(tenToThe16) // deviation of 1%
            );
            Chai.expectTrue(
              shortFps2UnscaledE18
              ->sub(shortFps1UnscaledE18)
              ->bnLt(tenToThe16) // deviation of 1%
            );
          });
          it("has a continuous curve at the edge case boundary", () => {
            // note that the boundary is at L=25 & S=75 when we have a -50% offset
            let%Await (longFps1UnscaledE18, shortFps1UnscaledE18) =
              runTestsAndReturnUnscaledFpsValuesE18(
                ~longValueUnscaled=24,
                ~shortValueUnscaled=76,
              );

            Chai.bnEqual(
              ~message="long out-of-bounds result should be 1",
              longFps1UnscaledE18,
              tenToThe18,
            );
            Chai.bnEqual(
              ~message="short out-of-bounds result should be 0",
              shortFps1UnscaledE18,
              zeroBn,
            );

            let%Await (longFps2UnscaledE18, shortFps2UnscaledE18) =
              runTestsAndReturnUnscaledFpsValuesE18(
                ~longValueUnscaled=26,
                ~shortValueUnscaled=74,
              );

            Chai.expectTrue(
              longFps1UnscaledE18
              ->sub(longFps2UnscaledE18)
              ->bnLt(tenToThe14) // deviation of 0.01%
            );
            Chai.expectTrue(
              shortFps2UnscaledE18
              ->sub(shortFps1UnscaledE18)
              ->bnLt(tenToThe14) // deviation of 0.01%
            );
          });
        });
        describe("with positive offset", () => {
          before_once'(() => {
            contracts.contents.staker
            ->Staker.Exposed.setEquilibriumOffset(
                ~marketIndex,
                ~balanceIncentiveCurve_equilibriumOffset=
                  tenToThe18->div(twoBn),
              )
          });
          it("longValue > shortValue", () => {
            runTestsAndReturnUnscaledFpsValuesE18(
              ~longValueUnscaled=randomValueLocked1 + randomValueLocked2,
              ~shortValueUnscaled=randomValueLocked2,
            )
          });
          it("longValue < shortValue", () => {
            runTestsAndReturnUnscaledFpsValuesE18(
              ~longValueUnscaled=randomValueLocked1,
              ~shortValueUnscaled=randomValueLocked1 + randomValueLocked2,
            )
          });
          it("has a continuous curve through intersection point", () => {
            // note that the middle is at L=250 & S=750 when we have a 50% offset
            let%Await (longFps1UnscaledE18, shortFps1UnscaledE18) =
              runTestsAndReturnUnscaledFpsValuesE18(
                ~longValueUnscaled=249,
                ~shortValueUnscaled=751,
              );

            // long side should be >50% and short side <50%
            Chai.expectTrue(
              longFps1UnscaledE18->bnGt(tenToThe18->div(twoBn)),
            );
            Chai.expectTrue(
              shortFps1UnscaledE18->bnLt(tenToThe18->div(twoBn)),
            );

            let%Await (longFps2UnscaledE18, shortFps2UnscaledE18) =
              runTestsAndReturnUnscaledFpsValuesE18(
                ~longValueUnscaled=251,
                ~shortValueUnscaled=749,
              );

            // short side should be <50% and long side >50%
            Chai.expectTrue(
              longFps2UnscaledE18->bnLt(tenToThe18->div(twoBn)),
            );
            Chai.expectTrue(
              shortFps2UnscaledE18->bnGt(tenToThe18->div(twoBn)),
            );

            Chai.expectTrue(
              longFps1UnscaledE18
              ->sub(longFps2UnscaledE18)
              ->bnLt(tenToThe16) // deviation of 1%
            );
            Chai.expectTrue(
              shortFps2UnscaledE18
              ->sub(shortFps1UnscaledE18)
              ->bnLt(tenToThe16) // deviation of 1%
            );
          });
          it("has a continuous curve at the edge case boundary", () => {
            // note that the boundary is at L=75 & S=25 when we have a 50% offset
            let%Await (longFps1UnscaledE18, shortFps1UnscaledE18) =
              runTestsAndReturnUnscaledFpsValuesE18(
                ~longValueUnscaled=76,
                ~shortValueUnscaled=24,
              );

            Chai.bnEqual(
              ~message="long out-of-bounds result should be 0",
              longFps1UnscaledE18,
              zeroBn,
            );
            Chai.bnEqual(
              ~message="short out-of-bounds result should be 1",
              shortFps1UnscaledE18,
              tenToThe18,
            );

            let%Await (longFps2UnscaledE18, shortFps2UnscaledE18) =
              runTestsAndReturnUnscaledFpsValuesE18(
                ~longValueUnscaled=74,
                ~shortValueUnscaled=26,
              );

            Chai.expectTrue(
              longFps2UnscaledE18
              ->sub(longFps1UnscaledE18)
              ->bnLt(tenToThe14) // deviation of 0.01%
            );
            Chai.expectTrue(
              shortFps1UnscaledE18
              ->sub(shortFps2UnscaledE18)
              ->bnLt(tenToThe14) // deviation of 0.01%
            );
          });
        });
      },
    );
    it("calls getKValue correctly", () => {
      StakerSmocked.InternalMock.mock_getKValueToReturn(kVal);
      let%Await _result =
        contracts^.staker
        ->Staker.Exposed._calculateFloatPerSecondExposed(
            ~marketIndex,
            ~longPrice,
            ~shortPrice,
            ~longValue=randomValueLocked1->bnFromInt->mul(tenToThe18),
            ~shortValue=randomValueLocked2->bnFromInt->mul(tenToThe18),
          );

      StakerSmocked.InternalMock._getKValueCallCheck(
        {marketIndex: marketIndex}: StakerSmocked.InternalMock._getKValueCall,
      );
    });
  });
};
