open Globals;
open LetOps;
open Mocha;

let testUnit =
    (
      ~contracts: ref(Helpers.stakerUnitTestContracts),
      ~accounts: ref(array(Ethers.Wallet.t)),
    ) => {
  describeUnit("Withdraw functions", () => {
    let marketIndex = Helpers.randomJsInteger();
    let amountStaked = Helpers.randomTokenAmount();

    describe_skip("_withdraw", () => {
      let userWallet: ref(Ethers.Wallet.t) = ref(None->Obj.magic);
      let treasury = Helpers.randomAddress();
      let amountWithdrawn =
        amountStaked->div(Js.Math.random_int(1, 20)->bnFromInt);
      let fees = Helpers.randomRatio1e18();
      let call: ref(Promise.t(ContractHelpers.transaction)) =
        ref(None->Obj.magic);
      let connectedStaker: ref(Staker.t) = ref(None->Obj.magic);

      let setup = amountStaked => {
        userWallet := accounts.contents->Array.getUnsafe(5);
        let {staker, syntheticTokenSmocked} = contracts.contents;
        let%Await _ =
          staker->Staker.Exposed.set_withdrawGlobals(
            ~marketIndex,
            ~syntheticToken=syntheticTokenSmocked.address,
            ~user=userWallet.contents.address,
            ~amountStaked,
            ~fees,
            ~treasury,
          );
        syntheticTokenSmocked->SyntheticTokenSmocked.mockTransferToReturn(
          true,
        );

        connectedStaker :=
          staker->ContractHelpers.connect(~address=userWallet.contents);
        call :=
          connectedStaker.contents
          ->Staker.Exposed._withdrawExposed(
              ~token=syntheticTokenSmocked.address,
              ~marketIndex,
              ~amount=amountWithdrawn,
            );
      };

      describe("happy case", () => {
        before_once'(() => {
          let%AwaitThen _ = setup(amountStaked);
          call.contents;
        });
        let fees = amountWithdrawn->mul(fees)->div(tenToThe18);
        it("calls transfer on the synthetic token with correct args", () => {
          contracts.contents.syntheticTokenSmocked
          ->SyntheticTokenSmocked.transferCallCheck({
              recipient: treasury,
              amount: fees,
            });
          contracts.contents.syntheticTokenSmocked
          ->SyntheticTokenSmocked.transferCallCheck({
              recipient: userWallet.contents.address,
              amount: amountWithdrawn->sub(fees),
            });
        });

        it(
          "calls _mintAccumulatedFloatAndExecuteOutstandingActions with correct args",
          () => {
          StakerSmocked.InternalMock._mintAccumulatedFloatAndExecuteOutstandingActionsCallCheck({
            user: userWallet.contents.address,
            marketIndex,
          })
        });

        it("mutates userAmountStaked", () => {
          let%Await amountStakedAfter =
            contracts.contents.staker
            ->Staker.userAmountStaked(
                contracts.contents.syntheticTokenSmocked.address,
                userWallet.contents.address,
              );

          amountStakedAfter->Chai.bnEqual(
            amountStaked->sub(amountWithdrawn),
          );
        });

        it("emits a StakeWithdrawn event with correct args", () =>
          Chai.callEmitEvents(
            ~call=call.contents,
            ~contract=connectedStaker.contents->Obj.magic,
            ~eventName="StakeWithdrawn",
          )
          ->Chai.withArgs4(
              userWallet.contents.address,
              contracts.contents.syntheticTokenSmocked.address,
              amountWithdrawn,
            )
        );

        it("emits a StakeWithdrawn event with correct args & fees", () =>
          Chai.callEmitEvents(
            ~call=call.contents,
            ~contract=connectedStaker.contents->Obj.magic,
            ~eventName="StakeWithdrawnWithFees",
          )
          ->Chai.withArgs4(
              userWallet.contents.address,
              contracts.contents.syntheticTokenSmocked.address,
              amountWithdrawn,
              fees,
            )
        );
      });

      describe("sad case", () => {
        before_once'(() => setup(amountWithdrawn->sub(oneBn)));
        it("reverts if nothing to withdraw", () => {
          Chai.expectRevert(
            ~transaction=call.contents,
            ~reason="not enough to withdraw",
          )
        });
      });
    });

    describe_skip("withdraw", () => {
      let token = Helpers.randomAddress();
      let amountWithdrawn = Helpers.randomTokenAmount();

      before_once'(() => {
        let%AwaitThen _ =
          contracts.contents.staker
          ->Staker.Exposed.setWithdrawGlobals(
              ~longShort=contracts.contents.longShortSmocked.address,
              ~marketIndex,
              ~token,
            );

        contracts.contents.staker
        ->Staker.withdraw(
            ~marketIndex,
            ~isWithdrawFromLong=true,
            ~amount=amountWithdrawn,
          );
      });

      it("calls updateSystemState on longShort with correct args", () => {
        contracts.contents.longShortSmocked
        ->LongShortSmocked.updateSystemStateCallCheck({
            marketIndex: marketIndex,
          })
      });
      it("calls _withdraw with correct args", () =>
        StakerSmocked.InternalMock._withdrawCallCheck({
          marketIndex,
          token,
          amount: amountWithdrawn,
        })
      );

      it("should not allow shifts > userAmountStaked", () => {
        let adminWallet = accounts.contents->Array.getUnsafe(0);

        let%Await _ =
          contracts.contents.staker
          ->ContractHelpers.connect(~address=adminWallet)
          ->Staker.Exposed.setWithdrawAllGlobals(
              ~marketIndex,
              ~longShort=contracts.contents.longShortSmocked.address,
              ~user=adminWallet.address,
              ~amountStaked=bnFromInt(0),
              ~token,
              ~userNextPrice_stakedActionIndex=bnFromInt(777),
              ~syntheticTokens=token,
              ~userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom_long=
                Helpers.randomTokenAmount(),
              ~userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom_short=
                Helpers.randomTokenAmount(),
            );

        Chai.expectRevert(
          ~transaction=
            contracts.contents.staker
            ->ContractHelpers.connect(~address=adminWallet)
            ->Staker.withdraw(
                ~marketIndex,
                ~isWithdrawFromLong=true,
                ~amount=amountWithdrawn,
              ),
          ~reason="Outstanding next price stake shifts too great",
        );
      });
    });

    describe_skip("withdrawAll", () => {
      let token = Helpers.randomAddress();
      let userWallet: ref(Ethers.Wallet.t) = ref(None->Obj.magic);
      before_once'(() => {
        userWallet := accounts.contents->Array.getUnsafe(5);
        let%AwaitThen _ =
          contracts.contents.staker
          ->Staker.Exposed.setWithdrawAllGlobals(
              ~longShort=contracts.contents.longShortSmocked.address,
              ~marketIndex,
              ~token,
              ~user=userWallet.contents.address,
              ~amountStaked,
              ~userNextPrice_stakedActionIndex=bnFromInt(1),
              ~syntheticTokens=Helpers.randomAddress(),
              ~userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom_long=
                bnFromInt(0),
              ~userNextPrice_amountStakedSyntheticToken_toShiftAwayFrom_short=
                bnFromInt(0),
            );

        contracts.contents.staker
        ->ContractHelpers.connect(~address=userWallet.contents)
        ->Staker.withdrawAll(~marketIndex, ~isWithdrawFromLong=true);
      });

      it("calls updateSystemState on longShort with correct args", () => {
        contracts.contents.longShortSmocked
        ->LongShortSmocked.updateSystemStateCallCheck({
            marketIndex: marketIndex,
          })
      });
      it("calls _withdraw with correct args", () =>
        StakerSmocked.InternalMock._withdrawCallCheck({
          marketIndex,
          token,
          amount: amountStaked,
        })
      );
    });
  });
};
