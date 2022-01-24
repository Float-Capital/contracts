open Globals;
open LetOps;
open Mocha;

describe("Float System", () => {
  let accounts: ref(array(Ethers.Wallet.t)) = ref(None->Obj.magic);
  let contracts: ref(Contract.YieldManagerAaveBasicHelpers.contractsType) =
    ref(None->Obj.magic);

  let setup = () => {
    let%AwaitThen loadedAccounts = Ethers.getSigners();
    accounts := loadedAccounts;

    let treasury = loadedAccounts->Array.getUnsafe(1);

    let longShortAddress = Ethers.Wallet.createRandom().address;

    let%Await paymentTokenSmocked = ERC20MockSmocked.make();

    let%Await aTokenSmocked = ERC20MockSmocked.make();

    let%Await aaveIncentivesControllerSmocked =
      AaveIncentivesControllerMockSmocked.make();
    let%Await lendingPoolAddressesProviderSmocked =
      LendingPoolAddressesProviderMockSmocked.make();

    let%Await yieldManagerAave = YieldManagerAaveBasic.make();

    let%Await _ =
      yieldManagerAave->YieldManagerAaveBasic.initialize(
        ~longShort=longShortAddress,
        ~treasury=treasury.address,
        ~paymentToken=paymentTokenSmocked.address,
        ~aToken=aTokenSmocked.address,
        ~lendingPoolAddressesProvider=
          lendingPoolAddressesProviderSmocked.address,
        ~aaveIncentivesController=aaveIncentivesControllerSmocked.address,
        ~aaveReferralCode=6543,
        ~admin=Helpers.randomAddress(),
      );

    contracts :=
      {
        "aToken": aTokenSmocked,
        "yieldManagerAave": yieldManagerAave,
        "paymentToken": paymentTokenSmocked,
        "treasury": treasury,
        "aaveIncentivesController": aaveIncentivesControllerSmocked,
      };
  };
  describeUnit("(un-optimised) YieldManagerAaveBasic - internals exposed", () => {
    before_each(setup)
  });
  describeUnit("(optimised) YieldManagerAaveBasic - internals exposed ", () => {
    before(setup);

    ClaimAaveRewards.testUnit(~contracts);
  });
});
