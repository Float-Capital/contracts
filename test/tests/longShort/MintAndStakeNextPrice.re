open Globals;
open LetOps;
open Mocha;
let testIntegration =
    (
      ~contracts: ref(Helpers.coreContracts),
      ~accounts: ref(array(Ethers.Wallet.t)),
    ) =>
  describe("MintAndStakeNextPrice", () => {
    let testMintAndStakeNextPrice = (~isLong, ()) => {
      let testUser = accounts.contents->Array.getUnsafe(8);
      let amountToNextPriceMintAndStake = Helpers.randomTokenAmount();

      let {longShort, markets, staker} = contracts.contents;
      let {paymentToken, oracleManager, longSynth, shortSynth, marketIndex} =
        markets->Array.getUnsafe(0);

      let currentSynth = isLong ? longSynth : shortSynth;

      let%AwaitThen _ =
        paymentToken->ERC20Mock.mint(
          ~_to=testUser.address,
          ~amount=amountToNextPriceMintAndStake->add(CONSTANTS.oneBn),
        );

      let%AwaitThen _ =
        paymentToken
        ->ContractHelpers.connect(~address=testUser)
        ->ERC20Mock.approve(
            ~spender=longShort.address,
            ~amount=amountToNextPriceMintAndStake->add(CONSTANTS.oneBn),
          );

      let%AwaitThen _ =
        longShort
        ->ContractHelpers.connect(~address=testUser)
        ->LongShort.mintAndStakeNextPrice(
            ~marketIndex,
            ~amount=amountToNextPriceMintAndStake,
            ~isLong,
          );

      let%AwaitThen previousPrice =
        oracleManager->OracleManagerMock.getLatestPrice;

      let nextPrice =
        previousPrice
        ->mul(bnFromInt(12)) // 20% increase
        ->div(bnFromInt(10));

      let%AwaitThen _ =
        oracleManager->OracleManagerMock.setPrice(~newPrice=nextPrice);

      let%AwaitThen _ = longShort->LongShort.updateSystemState(~marketIndex);

      // This triggers the _calculateAccumulatedFloatAndExecuteOutstandingActions function which settles the accouting for the user
      let%AwaitThen _ =
        staker
        ->ContractHelpers.connect(~address=testUser)
        ->Staker.claimFloatCustom(~marketIndexes=[|marketIndex|]);

      let%AwaitThen usersStakeAfterClaimingTokens =
        staker->Staker.userAmountStaked(
          currentSynth.address,
          testUser.address,
        );

      let%Await longTokenPrice =
        longShort->Contract.LongShortHelpers.getSyntheticTokenPrice(
          ~marketIndex,
          ~isLong,
        );

      let expectedNumberOfTokensToRecieve =
        amountToNextPriceMintAndStake
        ->mul(CONSTANTS.tenToThe18)
        ->div(longTokenPrice);

      Chai.bnEqual(
        ~message="Stake recieved is incorrect",
        expectedNumberOfTokensToRecieve,
        usersStakeAfterClaimingTokens,
      );
    };

    it(
      "should correctly be able to stake their long tokens next price",
      testMintAndStakeNextPrice(~isLong=true),
    );
    it(
      "should correctly be able to stake their short tokens next price",
      testMintAndStakeNextPrice(~isLong=false),
    );
  });
