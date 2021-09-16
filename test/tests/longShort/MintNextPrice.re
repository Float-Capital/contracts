open Globals;
open LetOps;
open Mocha;

let testIntegration =
    (
      ~contracts: ref(Helpers.coreContracts),
      ~accounts: ref(array(Ethers.Wallet.t)),
    ) =>
  describe("mintLongNextPrice", () => {
    it("should work as expected happy path", () => {
      // let admin = accounts.contents->Array.getUnsafe(0);
      let testUser = accounts.contents->Array.getUnsafe(8);
      let amountToNextPriceMint = Helpers.randomTokenAmount();

      let {longShort, markets} =
        // let {tokenFactory, treasury, floatToken, staker, longShort, markets} =
        contracts.contents;
      let {
        paymentToken,
        oracleManager,
        // yieldManager,
        longSynth,
        // shortSynth,
        marketIndex,
      } =
        markets->Array.getUnsafe(0);

      let%AwaitThen _longValueBefore =
        longShort->LongShort.marketSideValueInPaymentToken(
          marketIndex,
          true /*long*/,
        );

      let%AwaitThen _ =
        paymentToken->ERC20Mock.mint(
          ~_to=testUser.address,
          ~amount=amountToNextPriceMint,
        );

      let%AwaitThen _ =
        paymentToken
        ->ContractHelpers.connect(~address=testUser)
        ->ERC20Mock.approve(
            ~spender=longShort.address,
            ~amount=amountToNextPriceMint,
          );

      let%AwaitThen _ =
        longShort
        ->ContractHelpers.connect(~address=testUser)
        ->LongShort.mintLongNextPrice(
            ~marketIndex,
            ~amount=amountToNextPriceMint,
          );

      let%AwaitThen previousPrice =
        oracleManager->OracleManagerMock.getLatestPrice;

      let nextPrice =
        previousPrice
        ->mul(bnFromInt(12)) // 20% increase
        ->div(bnFromInt(10));

      // let%AwaitThen userNextPriceActions =
      //   longShort->Contract.LongShort.userNextPriceActions(
      //     ~marketIndex,
      //     ~user=testUser.address,
      //   );

      // let%AwaitThen usersBalanceBeforeOracleUpdate =
      //   longSynth->Contract.SyntheticToken.balanceOf(
      //     ~account=testUser.address,
      //   );

      let%AwaitThen _ =
        oracleManager->OracleManagerMock.setPrice(~newPrice=nextPrice);

      let%AwaitThen _ = longShort->LongShort.updateSystemState(~marketIndex);

      let%AwaitThen usersBalanceBeforeSettlement =
        longSynth->SyntheticToken.balanceOf(~account=testUser.address);

      // This triggers the _executeOutstandingNextPriceSettlements function
      let%AwaitThen _ =
        longShort
        ->ContractHelpers.connect(~address=testUser)
        ->LongShort.mintLongNextPrice(~marketIndex, ~amount=bnFromInt(0));
      let%AwaitThen usersUpdatedBalance =
        longSynth->SyntheticToken.balanceOf(~account=testUser.address);

      Chai.bnEqual(
        ~message=
          "Balance after price system update but before user settlement should be the same as after settlement",
        usersBalanceBeforeSettlement,
        usersUpdatedBalance,
      );

      let%Await longTokenPrice =
        longShort->Contract.LongShortHelpers.getSyntheticTokenPrice(
          ~marketIndex,
          ~isLong=true,
        );

      let expectedNumberOfTokensToRecieve =
        amountToNextPriceMint
        ->mul(CONSTANTS.tenToThe18)
        ->div(longTokenPrice);

      Chai.bnEqual(
        ~message="balance is incorrect",
        expectedNumberOfTokensToRecieve,
        usersUpdatedBalance,
      );
    })
  });

let testUnit =
    (
      ~contracts: ref(Helpers.longShortUnitTestContracts),
      ~accounts: ref(array(Ethers.Wallet.t)),
    ) => {
  describeUnit("mintNextPrice external functions", () => {
    let marketIndex = 1;
    let amount = Helpers.randomTokenAmount();

    before_once'(() =>
      contracts.contents.longShort->LongShortSmocked.InternalMock.setup
    );

    describe("mintLongNextPrice", () => {
      it("calls _mintNextPrice with isLong==true", () => {
        let%Await _ =
          contracts.contents.longShort
          ->LongShortSmocked.InternalMock.setupFunctionForUnitTesting(
              ~functionName="mintLongNextPrice",
            );

        let%Await _ =
          contracts.contents.longShort
          ->LongShort.mintLongNextPrice(~marketIndex, ~amount);

        LongShortSmocked.InternalMock._mintNextPriceCallCheck({
          marketIndex,
          amount,
          isLong: true,
        });
      })
    });

    describe("mintShortNextPrice", () => {
      it("calls _mintNextPrice with isLong==false", () => {
        let%Await _ =
          contracts.contents.longShort
          ->LongShortSmocked.InternalMock.setupFunctionForUnitTesting(
              ~functionName="mintShortNextPrice",
            );

        let%Await _ =
          contracts.contents.longShort
          ->LongShort.mintShortNextPrice(~marketIndex, ~amount);

        // let mintNextPriceCallCheck =
        //   LongShortSmocked.InternalMock._mintNextPriceCallCheck();

        LongShortSmocked.InternalMock._mintNextPriceCallCheck({
          marketIndex,
          amount,
          isLong: false,
        });
      })
    });
  });

  describe("mintNextPrice internal function", () => {
    let marketIndex = 1;
    let marketUpdateIndex = Helpers.randomInteger();
    let amount = Helpers.randomTokenAmount();
    let mintNextPriceExposedTxRef = ref("Not defined yet"->Obj.magic);

    let testMarketSide = (~isLong) => {
      before_once'(() => {
        Js.log("Running" ++ (isLong ? "Long" : "Short"));
        let testWallet = accounts.contents->Array.getUnsafe(1);

        let%AwaitThen _ =
          contracts.contents.longShort->LongShortSmocked.InternalMock.setup;

        let%AwaitThen _ =
          contracts.contents.longShort
          ->LongShortSmocked.InternalMock.setupFunctionForUnitTesting(
              ~functionName="_mintNextPrice",
            );

        let%AwaitThen _ =
          contracts.contents.longShort
          ->LongShortStateSetters.setMarketUpdateIndex(
              ~marketIndex,
              ~marketUpdateIndex,
            );

        let longShort =
          contracts.contents.longShort
          ->ContractHelpers.connect(~address=testWallet);

        mintNextPriceExposedTxRef :=
          longShort->LongShort.Exposed._mintNextPriceExposed(
            ~marketIndex,
            ~amount,
            ~isLong,
          );
        mintNextPriceExposedTxRef.contents;
      });

      it("calls the executeOutstandingNextPriceSettlements modifier", () => {
        let testWallet = accounts.contents->Array.getUnsafe(1);

        LongShortSmocked.InternalMock._executeOutstandingNextPriceSettlementsCallCheck({
          user: testWallet.address,
          marketIndex,
        });
      });

      it("emits the NextPriceDeposit event", () => {
        let testWallet = accounts.contents->Array.getUnsafe(1);

        Chai.callEmitEvents(
          ~call=mintNextPriceExposedTxRef.contents,
          ~eventName="NextPriceDeposit",
          ~contract=contracts.contents.longShort->Obj.magic,
        )
        ->Chai.withArgs5(
            marketIndex,
            isLong,
            amount,
            testWallet.address,
            marketUpdateIndex->add(oneBn),
          );
      });

      it("calls depositFunds with correct parameters", () => {
        LongShortSmocked.InternalMock._transferPaymentTokensFromUserToYieldManagerCallCheck({
          marketIndex,
          amount,
        })
      });

      it("updates the correct state variables with correct values", () => {
        let testWallet = accounts.contents->Array.getUnsafe(1);

        let%AwaitThen updatedBatchedAmountOfTokens_deposit =
          contracts.contents.longShort
          ->LongShort.batched_amountPaymentToken_deposit(marketIndex, isLong);

        let%AwaitThen updatedUserNextPriceDepositAmount =
          contracts.contents.longShort
          ->LongShort.userNextPrice_paymentToken_depositAmount(
              marketIndex,
              isLong,
              testWallet.address,
            );

        let%Await updateduserNextPrice_currentUpdateIndex =
          contracts.contents.longShort
          ->LongShort.userNextPrice_currentUpdateIndex(
              marketIndex,
              testWallet.address,
            );

        Chai.bnEqual(
          ~message="batched_amountPaymentToken_deposit not updated correctly",
          updatedBatchedAmountOfTokens_deposit,
          amount,
        );

        Chai.bnEqual(
          ~message="userNextPriceDepositAmount not updated correctly",
          updatedUserNextPriceDepositAmount,
          amount,
        );

        Chai.bnEqual(
          ~message="userNextPrice_currentUpdateIndex not updated correctly",
          updateduserNextPrice_currentUpdateIndex,
          marketUpdateIndex->add(oneBn),
        );
      });
    };

    describe("long", () => {
      testMarketSide(~isLong=true)
    });
    describe("short", () => {
      testMarketSide(~isLong=false)
    });
  });
};
