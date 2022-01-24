open LetOps;
open Mocha;
open Globals;
open SmockGeneral;

let randomValueChange = tokenAmount => {
  tokenAmount
  ->mul(Js.Math.random_int(-100, 101)->Ethers.BigNumber.fromInt)
  ->div(Ethers.BigNumber.fromUnsafe("100"));
};

let testUnit =
    (
      ~contracts: ref(Helpers.longShortUnitTestContracts),
      ~accounts as _: ref(array(Ethers.Wallet.t)),
    ) => {
  describeUnit("updateSystemState", () => {
    describe("_updateSystemStateInternal", () => {
      let marketIndex = Helpers.randomJsInteger();

      let (
        oldAssetPrice,
        oldLongPrice,
        oldShortPrice,
        oldLongValue,
        oldShortValue,
        oldLongValueAfterYield,
        oldShortValueAfterYield,
      ) =
        Helpers.Tuple.make7(Helpers.randomTokenAmount);

      let (longSynthSupply, shortSynthSupply) =
        Helpers.Tuple.make2(Helpers.randomTokenAmount);

      let valueChangeLong = randomValueChange(oldLongValueAfterYield);
      let valueChangeShort = randomValueChange(oldShortValueAfterYield);

      let newAssetPrice = oldAssetPrice->add(oneBn);

      let latestUpdateIndexForMarket = Helpers.randomInteger();

      let staker: ref(StakerSmocked.t) = ref(None->Obj.magic);
      let oracle: ref(OracleManagerMockSmocked.t) = ref(None->Obj.magic);
      let longSynth: ref(SyntheticTokenSmocked.t) = ref(None->Obj.magic);
      let shortSynth: ref(SyntheticTokenSmocked.t) = ref(None->Obj.magic);

      let potentialNewLongPrice: ref(Ethers.BigNumber.t) = ref(zeroBn);
      let potentialNewShortPrice: ref(Ethers.BigNumber.t) = ref(zeroBn);
      let setup =
          (
            ~oldAssetPrice,
            ~newAssetPrice,
            ~oldLongPrice,
            ~oldShortPrice,
            ~stakerNextPrice_currentUpdateIndex,
          ) => {
        let%AwaitThen _ =
          contracts.contents.longShort->LongShortSmocked.InternalMock.setup;
        let%AwaitThen _ =
          contracts.contents.longShort
          ->LongShortSmocked.InternalMock.setupFunctionForUnitTesting(
              ~functionName="_updateSystemStateInternal",
            );

        LongShortSmocked.InternalMock.mock_claimAndDistributeYieldThenRebalanceMarketToReturn(
          oldLongValueAfterYield,
          oldShortValueAfterYield,
        );
        LongShortSmocked.InternalMock.mock_batchConfirmOutstandingPendingActionsToReturn(
          valueChangeLong,
          valueChangeShort,
        );

        let%AwaitThen stakerSmocked = StakerSmocked.make();

        let%AwaitThen oracleSmocked = OracleManagerMockSmocked.make();

        let _ =
          oracleSmocked->OracleManagerMockSmocked.mockUpdatePriceToReturn(
            newAssetPrice,
          );

        oracle := oracleSmocked;
        staker := stakerSmocked;

        let%AwaitThen longSynthSmocked = SyntheticTokenSmocked.make();

        longSynthSmocked->SyntheticTokenSmocked.mockTotalSupplyToReturn(
          longSynthSupply,
        );

        longSynth := longSynthSmocked;

        let%AwaitThen shortSynthSmocked = SyntheticTokenSmocked.make();

        shortSynthSmocked->SyntheticTokenSmocked.mockTotalSupplyToReturn(
          shortSynthSupply,
        );

        shortSynth := shortSynthSmocked;

        let longShort = contracts.contents.longShort;

        // function is pure so we don't mock it
        let%AwaitThen predictedLongPrice =
          longShort->LongShort.Exposed._getSyntheticTokenPriceExposed(
            ~amountPaymentTokenBackingSynth=oldLongValueAfterYield,
            ~amountSyntheticToken=longSynthSupply,
          );

        potentialNewLongPrice := predictedLongPrice;

        let%AwaitThen predictedShortPrice =
          longShort->LongShort.Exposed._getSyntheticTokenPriceExposed(
            ~amountPaymentTokenBackingSynth=oldShortValueAfterYield,
            ~amountSyntheticToken=shortSynthSupply,
          );

        potentialNewShortPrice := predictedShortPrice;

        let%AwaitThen _ =
          longShort->LongShort.Exposed.set_updateSystemStateInternalGlobals(
            ~marketIndex,
            ~latestUpdateIndexForMarket,
            ~syntheticTokenPrice_inPaymentTokens_long=oldLongPrice,
            ~syntheticTokenPrice_inPaymentTokens_short=oldShortPrice,
            ~assetPrice=oldAssetPrice,
            ~oracleManager=oracleSmocked.address,
            ~staker=stakerSmocked.address,
            ~longValue=oldLongValue,
            ~shortValue=oldShortValue,
            ~synthLong=longSynthSmocked.address,
            ~synthShort=shortSynthSmocked.address,
            ~stakerNextPrice_currentUpdateIndex,
          );

        longShort->LongShort.Exposed._updateSystemStateInternalExposed(
          ~marketIndex,
        );
      };
      let setupWithoutPriceChange =
        setup(
          ~oldAssetPrice,
          ~newAssetPrice=oldAssetPrice,
          ~oldLongPrice,
          ~oldShortPrice,
        );

      let assertNoUpdateStateOrNonOracleCallCheck = (~checkNoStakerCalls) => {
        if (checkNoStakerCalls) {
          expect(
            staker.contents
            ->StakerSmocked.pushUpdatedMarketPricesToUpdateFloatIssuanceCalculationsFunction,
          )
          ->toHaveCallCount(0);
        };

        let%AwaitThen updateIndex =
          contracts.contents.longShort
          ->LongShort.marketUpdateIndex(marketIndex);

        let%AwaitThen newPriceSnapshot =
          contracts.contents.longShort
          ->LongShort.syntheticToken_priceSnapshot(marketIndex, updateIndex);

        let newLongPrice = newPriceSnapshot.price_long;
        let newShortPrice = newPriceSnapshot.price_short;

        let%Await assetPrice =
          contracts.contents.longShort->LongShort.assetPrice(marketIndex);

        Chai.bnEqual(oldAssetPrice, assetPrice);
        Chai.bnEqual(updateIndex, latestUpdateIndexForMarket);
        Chai.bnEqual(newLongPrice, oldLongPrice);
        Chai.bnEqual(newShortPrice, oldShortPrice);

        expect(
          LongShortSmocked.InternalMock._claimAndDistributeYieldThenRebalanceMarketFunction(),
        )
        ->toHaveCallCount(0);
        expect(
          LongShortSmocked.InternalMock._getSyntheticTokenPriceFunction(),
        )
        ->toHaveCallCount(0);
        expect(
          LongShortSmocked.InternalMock._batchConfirmOutstandingPendingActionsFunction(),
        )
        ->toHaveCallCount(0);
        expect(longSynth.contents->SyntheticTokenSmocked.totalSupplyFunction)
        ->toHaveCallCount(0);
        expect(shortSynth.contents->SyntheticTokenSmocked.totalSupplyFunction)
        ->toHaveCallCount(0);
      };

      it(
        "shouldn't call pushUpdatedMarketPricesToUpdateFloatIssuanceCalculations if there is no price update",
        () => {
          let%Await _ =
            setupWithoutPriceChange(
              ~stakerNextPrice_currentUpdateIndex=zeroBn,
            );

          assertNoUpdateStateOrNonOracleCallCheck(~checkNoStakerCalls=true);
        },
      );
      it("calls for the latest price from the oracle", () => {
        let%Await _ =
          setupWithoutPriceChange(~stakerNextPrice_currentUpdateIndex=zeroBn);
        oracle.contents->OracleManagerMockSmocked.updatePriceCallCheck;
      });

      describe("There is a price change", () => {
        let setupWithPriceChange =
          setup(
            ~oldAssetPrice,
            ~newAssetPrice,
            ~oldLongPrice,
            ~oldShortPrice,
          );

        it(
          "it should call the pushUpdatedMarketPricesToUpdateFloatIssuanceCalculations on the staker function if there is a price change",
          () => {
            let%Await _ =
              setupWithPriceChange(
                ~stakerNextPrice_currentUpdateIndex=zeroBn,
              );
            staker.contents
            ->StakerSmocked.pushUpdatedMarketPricesToUpdateFloatIssuanceCalculationsCallCheck({
                marketIndex,
                marketUpdateIndex: latestUpdateIndexForMarket->add(oneBn),
                longPrice: potentialNewLongPrice.contents,
                shortPrice: potentialNewShortPrice.contents,
                longValue: oldLongValueAfterYield->add(valueChangeLong),
                shortValue: oldShortValueAfterYield->add(valueChangeShort),
              });
          },
        );

        it(
          "it should call `_claimAndDistributeYieldThenRebalanceMarket` with correct arguments",
          () => {
            let%Await _ =
              setupWithPriceChange(
                ~stakerNextPrice_currentUpdateIndex=zeroBn,
              );
            LongShortSmocked.InternalMock._claimAndDistributeYieldThenRebalanceMarketCallCheck({
              marketIndex,
              newAssetPrice,
            });
          },
        );
        it(
          "it should call `_performOutstandingSettlements` with correct arguments",
          () => {
          let%Await _ =
            setupWithPriceChange(~stakerNextPrice_currentUpdateIndex=zeroBn);
          LongShortSmocked.InternalMock._batchConfirmOutstandingPendingActionsCallCheck({
            marketIndex,
            syntheticTokenPrice_inPaymentTokens_long:
              potentialNewLongPrice.contents,
            syntheticTokenPrice_inPaymentTokens_short:
              potentialNewShortPrice.contents,
          });
        });

        it("should call `totalSupply` on the long and short synth tokens", () => {
          let%Await _ =
            setupWithPriceChange(~stakerNextPrice_currentUpdateIndex=zeroBn);
          longSynth.contents->SyntheticTokenSmocked.totalSupplyCallCheck;

          shortSynth.contents->SyntheticTokenSmocked.totalSupplyCallCheck;
        });

        it(
          "should mutate syntheticToken_priceSnapshots for long and short correctly",
          () => {
          let%Await _ =
            setupWithPriceChange(~stakerNextPrice_currentUpdateIndex=zeroBn);
          let newUpdateIndex = latestUpdateIndexForMarket->add(oneBn);
          let%Await newPriceSnapshot =
            contracts.contents.longShort
            ->LongShort.syntheticToken_priceSnapshot(
                marketIndex,
                newUpdateIndex,
              );

          let newLongPrice = newPriceSnapshot.price_long;
          let newShortPrice = newPriceSnapshot.price_short;

          newLongPrice->Chai.bnEqual(potentialNewLongPrice.contents);
          newShortPrice->Chai.bnEqual(potentialNewShortPrice.contents);
        });

        it(
          "should mutate marketSideValueInPaymentTokens for long and short correctly",
          () => {
          let%AwaitThen _ =
            setupWithPriceChange(~stakerNextPrice_currentUpdateIndex=zeroBn);
          let%Await newMarketValues =
            contracts.contents.longShort
            ->LongShort.marketSideValueInPaymentToken(marketIndex);
          let newLongValue = newMarketValues.value_long;
          let newShortValue = newMarketValues.value_short;

          newLongValue->Chai.bnEqual(
            oldLongValueAfterYield->add(valueChangeLong),
          );
          newShortValue->Chai.bnEqual(
            oldShortValueAfterYield->add(valueChangeShort),
          );
        });

        it("it should update the (underlying) asset price correctly", () => {
          let%AwaitThen _ =
            setupWithPriceChange(~stakerNextPrice_currentUpdateIndex=zeroBn);
          let%Await assetPrice =
            contracts.contents.longShort->LongShort.assetPrice(marketIndex);
          Chai.bnEqual(assetPrice, newAssetPrice);
        });

        it("it should increment the marketUpdateIndex by 1", () => {
          let%AwaitThen _ =
            setupWithPriceChange(~stakerNextPrice_currentUpdateIndex=zeroBn);
          let%Await updateIndex =
            contracts.contents.longShort
            ->LongShort.marketUpdateIndex(marketIndex);
          Chai.bnEqual(latestUpdateIndexForMarket->add(oneBn), updateIndex);
        });

        it(
          "it should emit the SystemStateUpdated event with the correct arguments",
          () => {
          Chai.callEmitEvents(
            ~call=
              setupWithPriceChange(
                ~stakerNextPrice_currentUpdateIndex=zeroBn,
              ),
            ~eventName="SystemStateUpdated",
            ~contract=contracts.contents.longShort->Obj.magic,
          )
          ->Chai.withArgs7(
              marketIndex,
              latestUpdateIndexForMarket->add(oneBn),
              newAssetPrice,
              oldLongValueAfterYield->add(valueChangeLong),
              oldShortValueAfterYield->add(valueChangeShort),
              potentialNewLongPrice.contents,
              potentialNewShortPrice.contents,
            )
        });
      });
    });

    let setupWithUpdateSystemStateInternalMocked = (~functionName) => {
      let%AwaitThen _ =
        contracts.contents.longShort->LongShortSmocked.InternalMock.setup;
      contracts.contents.longShort
      ->LongShortSmocked.InternalMock.setupFunctionForUnitTesting(
          ~functionName,
        );
    };
    describe("updateSystemStateMulti", () => {
      it(
        "should call `_updateSystemStateInternal` for each market in the array",
        () => {
        let marketIndexes =
          Array.makeBy(Js.Math.random_int(0, 51), _ =>
            Helpers.randomJsInteger()
          );
        let%AwaitThen _ =
          setupWithUpdateSystemStateInternalMocked(
            ~functionName="updateSystemStateMulti",
          );

        // we don't mock modifiers
        let%AwaitThen _ =
          contracts.contents.longShort
          ->LongShort.Exposed.setMarketExistsMulti(~marketIndexes);

        let%Await _ =
          contracts.contents.longShort
          ->LongShort.updateSystemStateMulti(~marketIndexes);

        marketIndexes->Array.map(index => {
          LongShortSmocked.InternalMock._updateSystemStateInternalCallCheck({
            marketIndex: index,
          })
        });
      })
    });
    describe("updateSystemState", () => {
      it(
        "should call to `_updateSystemStateInternal` with the correct market as an argument",
        () => {
          let marketIndex = Helpers.randomJsInteger();
          let%AwaitThen _ =
            setupWithUpdateSystemStateInternalMocked(
              ~functionName="updateSystemState",
            );

          let%AwaitThen _ =
            contracts.contents.longShort
            ->LongShort.Exposed.setMarketExistsMulti(
                ~marketIndexes=[|marketIndex|],
              );

          let%Await _ =
            contracts.contents.longShort
            ->LongShort.updateSystemState(~marketIndex);

          LongShortSmocked.InternalMock._updateSystemStateInternalCallCheck({
            marketIndex: marketIndex,
          });
        },
      )
    });
  });
};

let testIntegration =
    (
      ~contracts: ref(Helpers.coreContracts),
      ~accounts: ref(array(Ethers.Wallet.t)),
    ) => {
  describe("updateSystemState", () => {
    let testDistributeYield = (~longIsOverBalanced) =>
      it(
        "distribute yield to markets flow "
        ++ (
          longIsOverBalanced ? "(long over balanced)" : "(short over balanced)"
        ),
        () => {
          let {longShort, markets} = contracts.contents;
          let {yieldManager, oracleManager, marketIndex, paymentToken} =
            markets->Array.getUnsafe(0);
          let testUser = accounts.contents->Array.getUnsafe(2);

          // 32.1... DAI - any random amount would do...
          let amountOfYieldToAward = bnFromString("3216543216543216542");

          let%Await marketValues =
            longShort->LongShort.marketSideValueInPaymentToken(marketIndex);
          let amountToMintToGuaranteeImbalance =
            longIsOverBalanced
              ? marketValues.value_short : marketValues.value_long;

          // Make sure the correct side is over-balanced!
          let%AwaitThen _ =
            HelperActions.mintDirect(
              ~marketIndex,
              ~amount=amountToMintToGuaranteeImbalance,
              ~token=paymentToken,
              ~user=testUser,
              ~longShort,
              ~oracleManagerMock=oracleManager,
              ~isLong=longIsOverBalanced,
            );

          // get total balance pools etc before (and amount for treasury)
          let%Await tokenPoolValueBefore =
            longShort->LongShort.marketSideValueInPaymentToken(marketIndex);
          let longTokenPoolValueBefore = tokenPoolValueBefore.value_long;
          let shortTokenPoolValueBefore = tokenPoolValueBefore.value_short;

          let%Await totalDueForTreasuryBefore =
            yieldManager->YieldManagerMock.totalReservedForTreasury;
          let totalValueRelatedToMarketBefore =
            longTokenPoolValueBefore
            ->add(shortTokenPoolValueBefore)
            ->add(totalDueForTreasuryBefore);

          // add some yield
          let _ =
            yieldManager->YieldManagerMock.settleWithYieldAbsolute(
              ~totalYield=amountOfYieldToAward,
            );

          // update oracle price
          let%Await currentOraclePrice =
            oracleManager->OracleManagerMock.getLatestPrice;
          let%Await _ =
            oracleManager->OracleManagerMock.setPrice(
              ~newPrice=currentOraclePrice->add(bnFromInt(1)),
            );

          // run long short update state
          let%Await _ = longShort->LongShort.updateSystemState(~marketIndex);

          // get total balance pools after and amount for treasury
          let%Await tokenPoolValueAfter =
            longShort->LongShort.marketSideValueInPaymentToken(marketIndex);
          let longTokenPoolValueAfter = tokenPoolValueAfter.value_long;
          let shortTokenPoolValueAfter = tokenPoolValueAfter.value_short;

          let%Await totalDueForTreasuryAfter =
            yieldManager->YieldManagerMock.totalReservedForTreasury;
          let totalValueRelatedToMarketAfter =
            longTokenPoolValueAfter
            ->add(shortTokenPoolValueAfter)
            ->add(totalDueForTreasuryAfter);

          Chai.bnEqual(
            ~message=
              "yield is either being lost or over-allocated - should be exactly the same",
            totalValueRelatedToMarketBefore->add(amountOfYieldToAward),
            totalValueRelatedToMarketAfter,
          );
        },
      );

    testDistributeYield(~longIsOverBalanced=true);
    testDistributeYield(~longIsOverBalanced=false);
    it("cannot call updateSystemState on a market that doesn't exist", () => {
      let nonExistantMarketIndex = 321321654;
      Chai.expectRevert(
        ~transaction=
          contracts.contents.longShort
          ->LongShort.updateSystemState(~marketIndex=nonExistantMarketIndex),
        ~reason="market doesn't exist",
      );
    });
  });
};
