open Globals;
open LetOps;
open Mocha;

let test =
    (
      ~contracts: ref(Helpers.stakerUnitTestContracts),
      ~accounts as _: ref(array(Ethers.Wallet.t)),
    ) => {
  describe("_mintFloat", () => {
    let user = Helpers.randomAddress();
    let floatToMint = Helpers.randomTokenAmount();

    let floatPercentage = Helpers.randomJsInteger() / 65536; // divide by 2^16 to keep in range of uint16 I think?

    before_once'(() => {
      let {staker, floatCapitalSmocked, floatTokenSmocked} =
        contracts.contents;

      let%Await _ =
        staker->StakerSmocked.InternalMock.setupFunctionForUnitTesting(
          ~functionName="_mintFloat",
        );

      let%Await _ =
        staker->Staker.setVariable(
          ~name="floatCapital",
          ~value=floatCapitalSmocked.address,
        );

      let%AwaitThen _ =
        staker->Staker.Exposed.set_mintFloatParams(
          ~floatToken=floatTokenSmocked.address,
          ~floatPercentage,
        );

      staker->Staker.Exposed._mintFloatExposed(~user, ~floatToMint);
    });

    it("calls mint on floatToken for user for amount floatToMint", () =>
      contracts.contents.floatTokenSmocked
      ->FloatTokenSmocked.mintCallCheck({_to: user, amount: floatToMint})
    );

    it(
      "calls mint on floatTokens for floatCapital for amount (floatToMint * floatPercentage) / 1e18",
      () => {
        let {floatCapitalSmocked, floatTokenSmocked} = contracts.contents;

        floatTokenSmocked->FloatTokenSmocked.mintCallCheck({
          _to: floatCapitalSmocked.address,
          amount:
            floatToMint
            ->mul(floatPercentage->bnFromInt)
            ->div(CONSTANTS.tenToThe18),
        });
      },
    );
  });
};
