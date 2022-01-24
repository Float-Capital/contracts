open Mocha;
open Globals;
open LetOps;
open SmockGeneral;

let testUnit =
    (
      ~contracts: ref(Helpers.longShortUnitTestContracts),
      ~accounts as _: ref(array(Ethers.Wallet.t)),
    ) => {
  describeUnit("Batched Settlement", () => {
    let marketIndex = Helpers.randomJsInteger();
    describe("_batchConfirmOutstandingPendingActions", () => {
      let syntheticTokenPrice_inPaymentTokens_long =
        Helpers.randomTokenAmount();
      let syntheticTokenPrice_inPaymentTokens_short =
        Helpers.randomTokenAmount();

      let setup =
          (
            ~batched_amountPaymentToken_depositLong,
            ~batched_amountPaymentToken_depositShort,
            ~batched_amountSyntheticToken_redeemLong,
            ~batched_amountSyntheticToken_redeemShort,
            ~batchedAmountSyntheticTokenToShiftFromLong,
            ~batchedAmountSyntheticTokenToShiftFromShort,
          ) => {
        let {longShort} = contracts.contents;
        let%AwaitThen _ =
          longShort->LongShortSmocked.InternalMock.setupFunctionForUnitTesting(
            ~functionName="_batchConfirmOutstandingPendingActions",
          );

        let%AwaitThen _ =
          longShort->LongShort.Exposed.setPerformOutstandingBatchedSettlementsGlobals(
            ~marketIndex,
            ~batched_amountPaymentToken_depositLong,
            ~batched_amountPaymentToken_depositShort,
            ~batched_amountSyntheticToken_redeemLong,
            ~batched_amountSyntheticToken_redeemShort,
            ~batchedAmountSyntheticTokenToShiftFromLong,
            ~batchedAmountSyntheticTokenToShiftFromShort,
          );

        longShort->LongShort.Exposed._batchConfirmOutstandingPendingActionsExposedCall(
          ~marketIndex,
          ~syntheticTokenPrice_inPaymentTokens_long,
          ~syntheticTokenPrice_inPaymentTokens_short,
        );
      };

      let runTest =
          (
            ~batched_amountPaymentToken_depositLong,
            ~batched_amountPaymentToken_depositShort,
            ~batched_amountSyntheticToken_redeemLong,
            ~batched_amountSyntheticToken_redeemShort,
            ~batchedAmountSyntheticTokenToShiftFromLong,
            ~batchedAmountSyntheticTokenToShiftFromShort,
          ) => {
        let batchedAmountSyntheticTokenToMintLong = ref(None->Obj.magic);
        let batchedAmountSyntheticTokenToMintShort = ref(None->Obj.magic);
        let batchedAmountOfPaymentTokensToBurnLong = ref(None->Obj.magic);
        let batchedAmountOfPaymentTokensToBurnShort = ref(None->Obj.magic);
        let batchedAmountOfPaymentTokensToShiftToLong = ref(None->Obj.magic);
        let batchedAmountOfPaymentTokensToShiftToShort = ref(None->Obj.magic);
        let batchedAmountSyntheticTokenToShiftToLong = ref(None->Obj.magic);
        let batchedAmountSyntheticTokenToShiftToShort = ref(None->Obj.magic);
        let calculatedValueChangeForLong = ref(None->Obj.magic);
        let calculatedValueChangeForShort = ref(None->Obj.magic);
        let calculatedValueChangeInSynthSupplyLong = ref(None->Obj.magic);
        let calculatedValueChangeInSynthSupplyShort = ref(None->Obj.magic);
        let returnOfPerformOutstandingBatchedSettlements =
          ref(None->Obj.magic);

        before_once'(() => {
          let%Await functionCallReturn =
            setup(
              ~batched_amountPaymentToken_depositLong,
              ~batched_amountPaymentToken_depositShort,
              ~batched_amountSyntheticToken_redeemLong,
              ~batched_amountSyntheticToken_redeemShort,
              ~batchedAmountSyntheticTokenToShiftFromLong,
              ~batchedAmountSyntheticTokenToShiftFromShort,
            );

          batchedAmountSyntheticTokenToMintLong :=
            Contract.LongShortHelpers.calcAmountSyntheticToken(
              ~amountPaymentToken=batched_amountPaymentToken_depositLong,
              ~price=syntheticTokenPrice_inPaymentTokens_long,
            );
          batchedAmountSyntheticTokenToMintShort :=
            Contract.LongShortHelpers.calcAmountSyntheticToken(
              ~amountPaymentToken=batched_amountPaymentToken_depositShort,
              ~price=syntheticTokenPrice_inPaymentTokens_short,
            );
          batchedAmountOfPaymentTokensToBurnLong :=
            Contract.LongShortHelpers.calcAmountPaymentToken(
              ~amountSyntheticToken=batched_amountSyntheticToken_redeemLong,
              ~price=syntheticTokenPrice_inPaymentTokens_long,
            );
          batchedAmountOfPaymentTokensToBurnShort :=
            Contract.LongShortHelpers.calcAmountPaymentToken(
              ~amountSyntheticToken=batched_amountSyntheticToken_redeemShort,
              ~price=syntheticTokenPrice_inPaymentTokens_short,
            );

          batchedAmountOfPaymentTokensToShiftToLong :=
            Contract.LongShortHelpers.calcAmountPaymentToken(
              ~amountSyntheticToken=batchedAmountSyntheticTokenToShiftFromShort,
              ~price=syntheticTokenPrice_inPaymentTokens_short,
            );
          batchedAmountOfPaymentTokensToShiftToShort :=
            Contract.LongShortHelpers.calcAmountPaymentToken(
              ~amountSyntheticToken=batchedAmountSyntheticTokenToShiftFromLong,
              ~price=syntheticTokenPrice_inPaymentTokens_long,
            );

          batchedAmountSyntheticTokenToShiftToShort :=
            Contract.LongShortHelpers.calcEquivalentAmountSyntheticTokensOnTargetSide(
              ~amountSyntheticTokenOriginSide=batchedAmountSyntheticTokenToShiftFromLong,
              ~priceOriginSide=syntheticTokenPrice_inPaymentTokens_long,
              ~priceTargetSide=syntheticTokenPrice_inPaymentTokens_short,
            );
          batchedAmountSyntheticTokenToShiftToLong :=
            Contract.LongShortHelpers.calcEquivalentAmountSyntheticTokensOnTargetSide(
              ~amountSyntheticTokenOriginSide=batchedAmountSyntheticTokenToShiftFromShort,
              ~priceOriginSide=syntheticTokenPrice_inPaymentTokens_short,
              ~priceTargetSide=syntheticTokenPrice_inPaymentTokens_long,
            );

          calculatedValueChangeForLong :=
            batched_amountPaymentToken_depositLong
            ->sub(batchedAmountOfPaymentTokensToBurnLong.contents)
            ->add(batchedAmountOfPaymentTokensToShiftToLong.contents)
            ->sub(batchedAmountOfPaymentTokensToShiftToShort.contents);
          calculatedValueChangeForShort :=
            batched_amountPaymentToken_depositShort
            ->sub(batchedAmountOfPaymentTokensToBurnShort.contents)
            ->add(batchedAmountOfPaymentTokensToShiftToShort.contents)
            ->sub(batchedAmountOfPaymentTokensToShiftToLong.contents);

          calculatedValueChangeInSynthSupplyLong :=
            batchedAmountSyntheticTokenToMintLong.contents
            ->sub(batched_amountSyntheticToken_redeemLong)
            ->add(batchedAmountSyntheticTokenToShiftToLong.contents)
            ->sub(batchedAmountSyntheticTokenToShiftFromLong);
          calculatedValueChangeInSynthSupplyShort :=
            batchedAmountSyntheticTokenToMintShort.contents
            ->sub(batched_amountSyntheticToken_redeemShort)
            ->add(batchedAmountSyntheticTokenToShiftToShort.contents)
            ->sub(batchedAmountSyntheticTokenToShiftFromShort);
          returnOfPerformOutstandingBatchedSettlements := functionCallReturn;
        });
        it(
          "call handleChangeInSyntheticTokensTotalSupply with the correct parameters",
          () => {
          // NOTE: due to the small optimization in the implementation (and ovoiding stack too deep errors) it is possible that the algorithm over issues float by a unit.
          //       This is probably not an issue since it overshoots rather than undershoots. However, this should be monitored or changed.
          let _ =
            LongShortSmocked.InternalMock._handleChangeInSyntheticTokensTotalSupplyCallCheck({
              marketIndex,
              isLong: true,
              changeInSyntheticTokensTotalSupply:
                calculatedValueChangeInSynthSupplyLong.contents,
            });
          LongShortSmocked.InternalMock._handleChangeInSyntheticTokensTotalSupplyCallCheck({
            marketIndex,
            isLong: false,
            changeInSyntheticTokensTotalSupply:
              calculatedValueChangeInSynthSupplyShort.contents,
          });
        });
        it(
          "call handleTotalValueChangeForMarketWithYieldManager with the correct parameters",
          () => {
            let totalPaymentTokenValueChangeForMarket =
              calculatedValueChangeForLong.contents
              ->add(calculatedValueChangeForShort.contents);
            LongShortSmocked.InternalMock._handleTotalPaymentTokenValueChangeForMarketWithYieldManagerCallCheck({
              marketIndex,
              totalPaymentTokenValueChangeForMarket,
            });
          },
        );
        it("should return the correct values", () => {
          Chai.recordEqualDeep(
            returnOfPerformOutstandingBatchedSettlements.contents,
            {
              long_changeInMarketValue_inPaymentToken:
                calculatedValueChangeForLong.contents,
              short_changeInMarketValue_inPaymentToken:
                calculatedValueChangeForShort.contents,
            },
          )
        });
      };

      describe("there are no actions in the batch", () => {
        runTest(
          ~batched_amountPaymentToken_depositLong=zeroBn,
          ~batched_amountPaymentToken_depositShort=zeroBn,
          ~batched_amountSyntheticToken_redeemLong=zeroBn,
          ~batched_amountSyntheticToken_redeemShort=zeroBn,
          ~batchedAmountSyntheticTokenToShiftFromLong=zeroBn,
          ~batchedAmountSyntheticTokenToShiftFromShort=zeroBn,
        )
      });
      describe("there is 1 deposit long", () => {
        runTest(
          ~batched_amountPaymentToken_depositLong=Helpers.randomTokenAmount(),
          ~batched_amountPaymentToken_depositShort=zeroBn,
          ~batched_amountSyntheticToken_redeemLong=zeroBn,
          ~batched_amountSyntheticToken_redeemShort=zeroBn,
          ~batchedAmountSyntheticTokenToShiftFromLong=zeroBn,
          ~batchedAmountSyntheticTokenToShiftFromShort=zeroBn,
        )
      });
      describe("there is 1 deposit short", () => {
        runTest(
          ~batched_amountPaymentToken_depositLong=zeroBn,
          ~batched_amountPaymentToken_depositShort=Helpers.randomTokenAmount(),
          ~batched_amountSyntheticToken_redeemLong=zeroBn,
          ~batched_amountSyntheticToken_redeemShort=zeroBn,
          ~batchedAmountSyntheticTokenToShiftFromLong=zeroBn,
          ~batchedAmountSyntheticTokenToShiftFromShort=zeroBn,
        )
      });
      describe("there is 1 withdraw long", () => {
        runTest(
          ~batched_amountPaymentToken_depositLong=zeroBn,
          ~batched_amountPaymentToken_depositShort=zeroBn,
          ~batched_amountSyntheticToken_redeemLong=Helpers.randomTokenAmount(),
          ~batched_amountSyntheticToken_redeemShort=zeroBn,
          ~batchedAmountSyntheticTokenToShiftFromLong=zeroBn,
          ~batchedAmountSyntheticTokenToShiftFromShort=zeroBn,
        )
      });
      describe("there is 1 withdraw short", () => {
        runTest(
          ~batched_amountPaymentToken_depositLong=zeroBn,
          ~batched_amountPaymentToken_depositShort=zeroBn,
          ~batched_amountSyntheticToken_redeemLong=zeroBn,
          ~batched_amountSyntheticToken_redeemShort=
            Helpers.randomTokenAmount(),
          ~batchedAmountSyntheticTokenToShiftFromLong=zeroBn,
          ~batchedAmountSyntheticTokenToShiftFromShort=zeroBn,
        )
      });
      describe("there is 1 shift from long to short", () => {
        runTest(
          ~batched_amountPaymentToken_depositLong=zeroBn,
          ~batched_amountPaymentToken_depositShort=zeroBn,
          ~batched_amountSyntheticToken_redeemLong=zeroBn,
          ~batched_amountSyntheticToken_redeemShort=zeroBn,
          ~batchedAmountSyntheticTokenToShiftFromLong=
            Helpers.randomTokenAmount(),
          ~batchedAmountSyntheticTokenToShiftFromShort=zeroBn,
        );
        it(
          "should set batched_amountSyntheticToken_toShiftAwayFrom_marketSide long to zero",
          () => {
            let%Await _ =
              contracts.contents.longShort
              ->LongShort.Exposed._batchConfirmOutstandingPendingActionsExposed(
                  ~marketIndex,
                  ~syntheticTokenPrice_inPaymentTokens_long,
                  ~syntheticTokenPrice_inPaymentTokens_short,
                );

            let%Await resultAfterCall =
              contracts.contents.longShort
              ->LongShort.batched_amountSyntheticToken_toShiftAwayFrom_marketSide(
                  marketIndex,
                  true,
                );
            Chai.bnEqual(resultAfterCall, zeroBn);
          },
        );
      });
      describe("there is 1 shift from short to long", () => {
        runTest(
          ~batched_amountPaymentToken_depositLong=zeroBn,
          ~batched_amountPaymentToken_depositShort=zeroBn,
          ~batched_amountSyntheticToken_redeemLong=zeroBn,
          ~batched_amountSyntheticToken_redeemShort=zeroBn,
          ~batchedAmountSyntheticTokenToShiftFromLong=zeroBn,
          ~batchedAmountSyntheticTokenToShiftFromShort=
            Helpers.randomTokenAmount(),
        );
        it(
          "should set batched_amountSyntheticToken_toShiftAwayFrom_marketSide short to zero",
          () => {
            let%Await _ =
              contracts.contents.longShort
              ->LongShort.Exposed._batchConfirmOutstandingPendingActionsExposed(
                  ~marketIndex,
                  ~syntheticTokenPrice_inPaymentTokens_long,
                  ~syntheticTokenPrice_inPaymentTokens_short,
                );

            let%Await resultAfterCall =
              contracts.contents.longShort
              ->LongShort.batched_amountSyntheticToken_toShiftAwayFrom_marketSide(
                  marketIndex,
                  false,
                );
            Chai.bnEqual(resultAfterCall, zeroBn);
          },
        );
      });
      describe(
        "there are random deposits and withdrawals (we could be more specific with this test possibly?)",
        () => {
        runTest(
          ~batched_amountPaymentToken_depositLong=Helpers.randomTokenAmount(),
          ~batched_amountPaymentToken_depositShort=Helpers.randomTokenAmount(),
          ~batched_amountSyntheticToken_redeemLong=Helpers.randomTokenAmount(),
          ~batched_amountSyntheticToken_redeemShort=
            Helpers.randomTokenAmount(),
          ~batchedAmountSyntheticTokenToShiftFromLong=
            Helpers.randomTokenAmount(),
          ~batchedAmountSyntheticTokenToShiftFromShort=
            Helpers.randomTokenAmount(),
        )
      });
    });
    describe("_handleChangeInSyntheticTokensTotalSupply", () => {
      before_once'(() => {
        let {
          longShort,
          syntheticToken1Smocked: longSyntheticToken,
          syntheticToken2Smocked: shortSyntheticToken,
        } =
          contracts.contents;

        longShort->LongShort.Exposed.setHandleChangeInSyntheticTokensTotalSupplyGlobals(
          ~marketIndex,
          ~longSyntheticToken=longSyntheticToken.address,
          ~shortSyntheticToken=shortSyntheticToken.address,
        );
      });
      let testHandleChangeInSyntheticTokensTotalSupply = (~isLong) => {
        describe("changeInSyntheticTokensTotalSupply > 0", () => {
          let changeInSyntheticTokensTotalSupply = Helpers.randomTokenAmount();
          before_once'(() => {
            let {longShort} = contracts.contents;

            longShort->LongShort.Exposed._handleChangeInSyntheticTokensTotalSupplyExposed(
              ~marketIndex,
              ~isLong,
              ~changeInSyntheticTokensTotalSupply,
            );
          });
          it(
            "should call the mint function on the correct synthetic token with correct arguments.",
            () => {
              let {longShort, syntheticToken1Smocked, syntheticToken2Smocked} =
                contracts.contents;
              let syntheticToken =
                isLong ? syntheticToken1Smocked : syntheticToken2Smocked;
              syntheticToken->SyntheticTokenSmocked.mintCallCheck({
                _to: longShort.address,
                amount: changeInSyntheticTokensTotalSupply,
              });
            },
          );
          it("should NOT call the burn function.", () => {
            let {syntheticToken1Smocked, syntheticToken2Smocked} =
              contracts.contents;
            let syntheticToken =
              isLong ? syntheticToken1Smocked : syntheticToken2Smocked;

            expect(syntheticToken->SyntheticTokenSmocked.burnFunction)
            ->toHaveCallCount(0);
          });
        });
        describe("changeInSyntheticTokensTotalSupply < 0", () => {
          let changeInSyntheticTokensTotalSupply =
            zeroBn->sub(Helpers.randomTokenAmount());
          before_once'(() => {
            let {longShort, syntheticToken1Smocked, syntheticToken2Smocked} =
              contracts.contents;
            let syntheticToken =
              isLong ? syntheticToken1Smocked : syntheticToken2Smocked;

            let _ =
              syntheticToken
              ->SyntheticTokenSmocked.mintFunction
              ->SmockGeneral.reset;

            longShort->LongShort.Exposed._handleChangeInSyntheticTokensTotalSupplyExposed(
              ~marketIndex,
              ~isLong,
              ~changeInSyntheticTokensTotalSupply,
            );
          });
          it(
            "should NOT call the mint function on the correct synthetic token.",
            () => {
            let {syntheticToken1Smocked, syntheticToken2Smocked} =
              contracts.contents;
            let syntheticToken =
              isLong ? syntheticToken1Smocked : syntheticToken2Smocked;

            expect(syntheticToken->SyntheticTokenSmocked.mintFunction)
            ->toHaveCallCount(0);
          });
          it(
            "should call the burn function on the correct synthetic token with correct arguments.",
            () => {
              let {syntheticToken1Smocked, syntheticToken2Smocked} =
                contracts.contents;
              let syntheticToken =
                isLong ? syntheticToken1Smocked : syntheticToken2Smocked;

              syntheticToken->SyntheticTokenSmocked.burnCallCheck({
                amount: zeroBn->sub(changeInSyntheticTokensTotalSupply),
              });
            },
          );
        });
        describe("changeInSyntheticTokensTotalSupply == 0", () => {
          it("should call NEITHER the mint NOR burn function.", () => {
            let changeInSyntheticTokensTotalSupply = zeroBn;
            let {longShort, syntheticToken1Smocked, syntheticToken2Smocked} =
              contracts.contents;
            let syntheticToken =
              isLong ? syntheticToken1Smocked : syntheticToken2Smocked;
            let _ =
              syntheticToken
              ->SyntheticTokenSmocked.mintFunction
              ->SmockGeneral.reset;
            let _ =
              syntheticToken
              ->SyntheticTokenSmocked.burnFunction
              ->SmockGeneral.reset;

            let%Await _ =
              longShort->LongShort.Exposed._handleChangeInSyntheticTokensTotalSupplyExposed(
                ~marketIndex,
                ~isLong,
                ~changeInSyntheticTokensTotalSupply,
              );

            expect(syntheticToken->SyntheticTokenSmocked.mintFunction)
            ->toHaveCallCount(0);
            expect(syntheticToken->SyntheticTokenSmocked.burnFunction)
            ->toHaveCallCount(0);
          })
        });
      };
      describe("LongSide", () => {
        testHandleChangeInSyntheticTokensTotalSupply(~isLong=true);
        testHandleChangeInSyntheticTokensTotalSupply(~isLong=false);
      });
    });
    describe(
      "_handleTotalPaymentTokenValueChangeForMarketWithYieldManager", () => {
      before_once'(() => {
        let {longShort, yieldManagerSmocked} = contracts.contents;

        longShort->LongShortStateSetters.setYieldManager(
          ~marketIndex,
          ~yieldManager=yieldManagerSmocked.address,
        );
      });
      describe("totalPaymentTokenValueChangeForMarket > 0", () => {
        let totalPaymentTokenValueChangeForMarket =
          Helpers.randomTokenAmount();
        before_once'(() => {
          let {longShort} = contracts.contents;

          longShort->LongShort.Exposed._handleTotalPaymentTokenValueChangeForMarketWithYieldManagerExposed(
            ~marketIndex,
            ~totalPaymentTokenValueChangeForMarket,
          );
        });
        it(
          "should call the depositPaymentToken function on the correct synthetic token with correct arguments.",
          () => {
          contracts.contents.yieldManagerSmocked
          ->YieldManagerAaveBasicSmocked.depositPaymentTokenCallCheck({
              amount: totalPaymentTokenValueChangeForMarket,
            })
        });
        it("should NOT call the removePaymentTokenFromMarket function.", () =>
          expect(
            contracts.contents.yieldManagerSmocked
            ->YieldManagerAaveBasicSmocked.removePaymentTokenFromMarketFunction,
          )
          ->toHaveCallCount(0)
        );
      });
      describe("totalPaymentTokenValueChangeForMarket < 0", () => {
        let totalPaymentTokenValueChangeForMarket =
          zeroBn->sub(Helpers.randomTokenAmount());
        before_once'(() => {
          let {longShort, yieldManagerSmocked} = contracts.contents;

          let _ =
            yieldManagerSmocked
            ->YieldManagerAaveBasicSmocked.depositPaymentTokenFunction
            ->SmockGeneral.reset;

          longShort->LongShort.Exposed._handleTotalPaymentTokenValueChangeForMarketWithYieldManagerExposed(
            ~marketIndex,
            ~totalPaymentTokenValueChangeForMarket,
          );
        });
        it(
          "should NOT call the depositPaymentToken function on the correct synthetic token.",
          () =>
          expect(
            contracts.contents.yieldManagerSmocked
            ->YieldManagerAaveBasicSmocked.depositPaymentTokenFunction,
          )
          ->toHaveCallCount(0)
        );
        it(
          "should call the removePaymentTokenFromMarket function on the correct synthetic token with correct arguments.",
          () => {
          contracts.contents.yieldManagerSmocked
          ->YieldManagerAaveBasicSmocked.removePaymentTokenFromMarketCallCheck({
              amount: zeroBn->sub(totalPaymentTokenValueChangeForMarket),
            })
        });
      });
      describe("totalPaymentTokenValueChangeForMarket == 0", () => {
        it(
          "should call NEITHER the depositPaymentToken NOR removePaymentTokenFromMarket function.",
          () => {
            let totalPaymentTokenValueChangeForMarket = zeroBn;

            let {longShort, yieldManagerSmocked} = contracts.contents;

            let _ =
              yieldManagerSmocked
              ->YieldManagerAaveBasicSmocked.depositPaymentTokenFunction
              ->SmockGeneral.reset;
            let _ =
              yieldManagerSmocked
              ->YieldManagerAaveBasicSmocked.removePaymentTokenFromMarketFunction
              ->SmockGeneral.reset;

            let%Await _ =
              longShort->LongShort.Exposed._handleTotalPaymentTokenValueChangeForMarketWithYieldManagerExposed(
                ~marketIndex,
                ~totalPaymentTokenValueChangeForMarket,
              );

            expect(
              yieldManagerSmocked->YieldManagerAaveBasicSmocked.depositPaymentTokenFunction,
            )
            ->toHaveCallCount(0);
            expect(
              yieldManagerSmocked->YieldManagerAaveBasicSmocked.removePaymentTokenFromMarketFunction,
            )
            ->toHaveCallCount(0);
          },
        )
      });
    });
  });
};
