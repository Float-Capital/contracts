open LetOps;
open Globals;
open Mocha;

describe("Float System", () => {
  describeIntegration("LongShort", () => {
    let contracts: ref(Helpers.coreContracts) = ref(None->Obj.magic);
    let accounts: ref(array(Ethers.Wallet.t)) = ref(None->Obj.magic);

    before_each(() => {
      let%Await loadedAccounts = Ethers.getSigners();
      accounts := loadedAccounts;

      let%AwaitThen deployedContracts =
        Helpers.initialize(
          ~admin=accounts.contents->Array.getUnsafe(0),
          ~exposeInternals=false,
        );
      contracts := deployedContracts;
      let setupUser = accounts.contents->Array.getUnsafe(2);

      HelperActions.stakeRandomlyInBothSidesOfMarket(
        ~marketsToStakeIn=deployedContracts.markets,
        ~userToStakeWith=setupUser,
        ~longShort=deployedContracts.longShort,
      );
    });

    UpdateSystemState.testIntegration(~contracts, ~accounts);

    MintNextPrice.testIntegration(~contracts, ~accounts);
    MintAndStakeNextPrice.testIntegration(~contracts, ~accounts);
    ShiftNextPrice.testIntegration(~contracts, ~accounts);
    RedeemNextPrice.testIntegration(~contracts, ~accounts);
    InitializeMarket.testIntegration(~contracts, ~accounts);
  });

  describeBoth("LongShort - Admin functions", () => {
    let contracts: ref(Helpers.coreContracts) = ref(None->Obj.magic);
    let accounts: ref(array(Ethers.Wallet.t)) = ref(None->Obj.magic);

    before(() => {
      let%Await loadedAccounts = Ethers.getSigners();
      accounts := loadedAccounts;

      let%Await deployedContracts =
        Helpers.initialize(
          ~admin=accounts.contents->Array.getUnsafe(0),
          ~exposeInternals=false,
        );
      contracts := deployedContracts;
    });

    describe("updateMarketOracle", () => {
      let newOracleManager = Ethers.Wallet.createRandom().address;
      let marketIndex = 1;
      it("should allow admin to update the oracle", () => {
        let%Await originalOracleAddress =
          contracts.contents.longShort->LongShort.oracleManagers(marketIndex);
        let%Await _ =
          contracts.contents.longShort
          ->LongShort.updateMarketOracle(~marketIndex, ~newOracleManager);
        let%Await updatedOracleAddress =
          contracts.contents.longShort->LongShort.oracleManagers(marketIndex);

        Chai.addressEqual(
          ~otherAddress=updatedOracleAddress,
          newOracleManager,
        );

        // Reset for the next tests
        let%Await _ =
          contracts.contents.longShort
          ->LongShort.updateMarketOracle(
              ~marketIndex,
              ~newOracleManager=originalOracleAddress,
            );
        ();
      });

      it("shouldn't allow non admin to update the oracle", () => {
        let attackerAddress = accounts.contents->Array.getUnsafe(5);

        Chai.expectRevert(
          ~transaction=
            contracts.contents.longShort
            ->ContractHelpers.connect(~address=attackerAddress)
            ->LongShort.updateMarketOracle(~marketIndex, ~newOracleManager),
          ~reason=Helpers.adminErrorMessage(~address=attackerAddress.address),
        );
      });
    });

    describe("changeMarketTreasurySplitGradient", () => {
      let newGradient = twoBn;
      let marketIndex = 1;

      it("should allow admin to update a gradient", () => {
        let%Await _ =
          contracts.contents.longShort
          ->LongShort.changeMarketTreasurySplitGradient(
              ~marketIndex,
              ~marketTreasurySplitGradient_e18=newGradient,
            );

        let%Await updatedGradient =
          contracts.contents.longShort
          ->LongShort.marketTreasurySplitGradient_e18(marketIndex);

        Chai.bnEqual(updatedGradient, newGradient);
      });

      it("shouldn't allow non admin to update the treasury address", () => {
        let attackerAddress = accounts.contents->Array.getUnsafe(5);

        Chai.expectRevert(
          ~transaction=
            contracts.contents.longShort
            ->ContractHelpers.connect(~address=attackerAddress)
            ->LongShort.changeMarketTreasurySplitGradient(
                ~marketIndex,
                ~marketTreasurySplitGradient_e18=newGradient,
              ),
          ~reason=Helpers.adminErrorMessage(~address=attackerAddress.address),
        );
      });
    });
  });

  describe("Smocked", () => {
    let contracts = ref("NOT INITIALIZED"->Obj.magic);
    let accounts = ref("NOT INITIALIZED"->Obj.magic);

    before(() => {
      let%Await loadedAccounts = Ethers.getSigners();
      accounts := loadedAccounts;

      let%Await deployedContracts = Helpers.initializeLongShortUnit();

      contracts := deployedContracts;
    });
    describeUnit("Unit tests", () => {
      ExecuteOutstandingNextPriceSettlements.testUnit(~contracts, ~accounts);
      ExecuteOutstandingNextPriceSettlementsUserMulti.testUnit(
        ~contracts,
        ~accounts,
      );
      ExecuteNextPriceAction.testUnit(~contracts, ~accounts);
      PriceCalculationFunctions.testUnit(~contracts, ~accounts);
      DepositFunds.testUnit(~contracts, ~accounts);
      GetUsersConfirmedButNotSettledBalance.testUnit(~contracts, ~accounts);
      InitializeMarket.testUnit(~contracts, ~accounts);
      UpdateSystemState.testUnit(~contracts, ~accounts);
      ClaimAndDistributeYieldThenRebalanceMarket.testUnit(
        ~contracts,
        ~accounts,
      );
      UtilsHelpers.testUnit(~contracts, ~accounts);
      RedeemNextPrice.testUnit(~contracts, ~accounts);
      ShiftNextPrice.testUnit(~contracts, ~accounts);
      MintNextPrice.testUnit(~contracts, ~accounts);
      BatchedSettlement.testUnit(~contracts, ~accounts);
      CreateNewSyntheticMarket.testUnit(~contracts, ~accounts);
      CalculateFundingRate.testUnit(~contracts, ~accounts);
    });
  });
});
