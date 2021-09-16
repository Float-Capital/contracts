open Mocha;
open LetOps;

let testUnit =
    (
      ~contracts: ref(Helpers.longShortUnitTestContracts),
      ~accounts: ref(array(Ethers.Wallet.t)),
    ) => {
  describe("depositing funds", () => {
    let amount = Helpers.randomTokenAmount();
    let marketIndex = 1;

    describe("_transferPaymentTokensFromUserToYieldManager", () => {
      let paymentTokenSmocked = ref(ERC20MockSmocked.uninitializedValue);
      let testYieldManager = Helpers.randomAddress();

      let setup = (~testWallet: Ethers.walletType) => {
        let%Await _ =
          contracts.contents.longShort
          ->LongShortSmocked.InternalMock.setupFunctionForUnitTesting(
              ~functionName="_transferPaymentTokensFromUserToYieldManager",
            );

        let%AwaitThen smockedPaymentToken = ERC20MockSmocked.make();
        smockedPaymentToken->ERC20MockSmocked.mockTransferFromToReturn(true);
        paymentTokenSmocked := smockedPaymentToken;

        let%AwaitThen _ =
          contracts.contents.longShort
          ->LongShort.Exposed.setDepositFundsGlobals(
              ~marketIndex,
              ~paymentToken=smockedPaymentToken.address,
              ~yieldManager=testYieldManager,
            );

        let longShort =
          contracts.contents.longShort
          ->ContractHelpers.connect(~address=testWallet);

        longShort->LongShort.Exposed._transferPaymentTokensFromUserToYieldManagerExposed(
          ~marketIndex,
          ~amount,
        );
      };

      it("calls paymentToken.transferFrom with correct arguments", () => {
        let testWallet = accounts.contents->Array.getUnsafe(1);
        let%Await _ = setup(~testWallet);

        paymentTokenSmocked.contents
        ->ERC20MockSmocked.transferFromCallCheck({
            sender: testWallet.address,
            recipient: testYieldManager,
            amount,
          });
      });
    });
  });
};
