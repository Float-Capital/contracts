open LetOps;
open TestnetDeployHelpers;
open Globals;
open ProtocolInteractionHelpers;

type allContracts = {
  staker: Staker.t,
  longShort: LongShort.t,
  paymentToken: ERC20Mock.t,
  treasury: TreasuryAlpha.t,
  syntheticToken: SyntheticToken.t,
};

let runTestTransactions =
    (
      {longShort, treasury, paymentToken, staker},
      _deploymentArgs: Hardhat.hardhatDeployArgument,
    ) => {
  let%Await loadedAccounts = Ethers.getSigners();

  let admin = loadedAccounts->Array.getUnsafe(1);
  let user1 = loadedAccounts->Array.getUnsafe(2);
  let user2 = loadedAccounts->Array.getUnsafe(3);
  let user3 = loadedAccounts->Array.getUnsafe(4);
  let user7 = loadedAccounts->Array.getUnsafe(8);
  let user8 = loadedAccounts->Array.getUnsafe(9);

  let%AwaitThen _ = topupBalanceIfLow(~from=admin, ~to_=user1);
  let%AwaitThen _ = topupBalanceIfLow(~from=admin, ~to_=user2);
  let%AwaitThen _ = topupBalanceIfLow(~from=admin, ~to_=user3);

  Js.log("deploying markets");

  let%AwaitThen _ =
    deployTestMarket(
      ~syntheticName="Eth Market",
      ~syntheticSymbol="FL_ETH",
      ~longShortInstance=longShort,
      ~treasuryInstance=treasury,
      ~admin,
      ~paymentToken: ERC20Mock.t,
    );

  let%AwaitThen _ =
    deployTestMarket(
      ~syntheticName="The Flippening",
      ~syntheticSymbol="FL_FLIP",
      ~longShortInstance=longShort,
      ~treasuryInstance=treasury,
      ~admin,
      ~paymentToken: ERC20Mock.t,
    );

  let%AwaitThen _ =
    deployTestMarket(
      ~syntheticName="Doge Market",
      ~syntheticSymbol="FL_DOGE",
      ~longShortInstance=longShort,
      ~treasuryInstance=treasury,
      ~admin,
      ~paymentToken: ERC20Mock.t,
    );
  let initialMarkets = [|1, 2, 3|];

  let longMintAmount = bnFromString("10000000000000000000");
  let shortMintAmount = longMintAmount->div(bnFromInt(2));
  let redeemShortAmount = shortMintAmount->div(bnFromInt(2));
  let longStakeAmount = bnFromString("100000000000000000");
  let shortStakeAmount = bnFromString("500000000000000000"); //0.5

  let priceAndStateUpdate = () => {
    let%AwaitThen _ =
      executeOnMarkets(
        initialMarkets,
        setOracleManagerPrice(~longShort, ~marketIndex=_, ~admin),
      );

    let%AwaitThen _ = Helpers.increaseTime(5);
    Js.log("Executing update system state");

    executeOnMarkets(
      initialMarkets,
      updateSystemState(~longShort, ~admin, ~marketIndex=_),
    );
  };

  Js.log("Executing Long Mints");
  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      mintLongNextPriceWithSystemUpdate(
        ~amount=longMintAmount,
        ~marketIndex=_,
        ~paymentToken,
        ~longShort,
        ~user=user1,
        ~admin,
      ),
    );

  Js.log("Executing Short Mints");
  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      mintShortNextPriceWithSystemUpdate(
        ~amount=shortMintAmount,
        ~marketIndex=_,
        ~paymentToken,
        ~longShort,
        ~user=user1,
        ~admin,
      ),
    );

  Js.log("Executing Short Position Redeem");
  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      redeemShortNextPriceWithSystemUpdate(
        ~amount=redeemShortAmount,
        ~marketIndex=_,
        ~longShort,
        ~user=user1,
        ~admin,
      ),
    );

  let%AwaitThen _ = priceAndStateUpdate();

  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      mintLongNextPriceWithSystemUpdate(
        ~amount=longMintAmount,
        ~marketIndex=_,
        ~paymentToken,
        ~longShort,
        ~user=user1,
        ~admin,
      ),
    );

  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      shiftFromShortNextPriceWithSystemUpdate(
        ~amountSyntheticTokensToShift=redeemShortAmount,
        ~marketIndex=_,
        ~longShort,
        ~user=user1,
        ~admin,
      ),
    );

  Js.log("Staking long position");
  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      stakeSynthLong(
        ~amount=longStakeAmount,
        ~longShort,
        ~marketIndex=_,
        ~user=user1,
      ),
    );

  Js.log("multiple synth shift from long, same price update");

  let%AwaitThen _ =
    shiftNextPrice(
      ~amountSyntheticTokensToShift=twoBn,
      ~marketIndex=1,
      ~longShort: LongShort.t,
      ~user=user1,
      ~isLong=true,
    );

  let%AwaitThen _ =
    shiftNextPrice(
      ~amountSyntheticTokensToShift=twoBn,
      ~marketIndex=1,
      ~longShort: LongShort.t,
      ~user=user1,
      ~isLong=true,
    );

  let%AwaitThen _ = priceAndStateUpdate();

  let longShiftAmount = longStakeAmount->div(twoBn);

  Js.log("Shift stake with system updated all markets");

  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      shiftStakeNextPriceWithSystemUpdate(
        ~amount=longShiftAmount,
        ~marketIndex=_,
        ~longShort,
        ~staker,
        ~isShiftFromLong=true,
        ~user=user1,
        ~admin,
      ),
    );

  let%AwaitThen _ = topupBalanceIfLow(~from=admin, ~to_=user7);

  let%AwaitThen _ = priceAndStateUpdate();

  Js.log("Multiple users minting long and short - all markets");
  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      mintLongNextPriceWithSystemUpdate(
        ~amount=bnFromInt(1),
        ~marketIndex=_,
        ~paymentToken,
        ~longShort,
        ~user=user1,
        ~admin,
      ),
    );
  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      mintShortNextPriceWithSystemUpdate(
        ~amount=bnFromInt(1),
        ~marketIndex=_,
        ~paymentToken,
        ~longShort,
        ~user=user1,
        ~admin,
      ),
    );
  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      mintLongNextPriceWithSystemUpdate(
        ~amount=bnFromInt(1),
        ~marketIndex=_,
        ~paymentToken,
        ~longShort,
        ~user=user2,
        ~admin,
      ),
    );
  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      mintLongNextPriceWithSystemUpdate(
        ~amount=bnFromInt(1),
        ~marketIndex=_,
        ~paymentToken,
        ~longShort,
        ~user=user8,
        ~admin,
      ),
    );
  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      mintLongNextPriceWithSystemUpdate(
        ~amount=bnFromInt(1),
        ~marketIndex=_,
        ~paymentToken,
        ~longShort,
        ~user=user7,
        ~admin,
      ),
    );
  Js.log("Stake short and shift to long");
  //Test Short Shifting
  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      mintShortNextPriceWithSystemUpdate(
        ~amount=shortMintAmount,
        ~marketIndex=_,
        ~paymentToken,
        ~longShort,
        ~user=user2,
        ~admin,
      ),
    );

  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      stakeSynthShort(
        ~amount=shortStakeAmount,
        ~longShort,
        ~marketIndex=_,
        ~user=user2,
      ),
    );
  let%AwaitThen _ = priceAndStateUpdate();

  let shortShiftAmount = shortStakeAmount->div(twoBn);
  Js.log("multiple shifts short same price update");

  Js.log2("Amount to shift is", shortShiftAmount->div(twoBn)->bnToString);
  let%AwaitThen _ =
    shiftNextPrice(
      ~amountSyntheticTokensToShift=shortShiftAmount->div(twoBn),
      ~marketIndex=1,
      ~longShort,
      ~user=user2,
      ~isLong=false,
    );

  Js.log2("Amount to shift is", shortShiftAmount->div(twoBn)->bnToString);
  let%AwaitThen _ =
    shiftNextPrice(
      ~amountSyntheticTokensToShift=shortShiftAmount->div(twoBn),
      ~marketIndex=1,
      ~longShort,
      ~user=user2,
      ~isLong=false,
    );

  Js.log("Multiple mints same price update");
  let mintAmount = bnFromString("20000000000000000");

  let%AwaitThen _ =
    mintNextPrice(
      ~amount=mintAmount,
      ~marketIndex=1,
      ~paymentToken,
      ~longShort: LongShort.t,
      ~user=user3,
      ~isLong=true,
    );
  let%AwaitThen _ =
    mintNextPrice(
      ~amount=mintAmount,
      ~marketIndex=1,
      ~paymentToken,
      ~longShort: LongShort.t,
      ~user=user3,
      ~isLong=true,
    );

  let%AwaitThen _ = priceAndStateUpdate();

  Js.log("Redeem Long market ID 1");

  let%AwaitThen _ =
    redeemNextPrice(
      ~amount=mintAmount,
      ~marketIndex=1,
      ~longShort,
      ~user=user3,
      ~isLong=true,
    );

  let%AwaitThen _ = priceAndStateUpdate();

  Js.log("Multiple mints same price update");
  let%AwaitThen _ =
    mintNextPrice(
      ~amount=mintAmount,
      ~marketIndex=1,
      ~paymentToken,
      ~longShort: LongShort.t,
      ~user=user3,
      ~isLong=true,
    );
  let%AwaitThen _ =
    mintNextPrice(
      ~amount=mintAmount,
      ~marketIndex=1,
      ~paymentToken,
      ~longShort: LongShort.t,
      ~user=user3,
      ~isLong=true,
    );

  let%AwaitThen _ = priceAndStateUpdate();

  Js.log("Redeem Long market ID 1");

  let%AwaitThen _ =
    redeemNextPrice(
      ~amount=mintAmount,
      ~marketIndex=1,
      ~longShort,
      ~user=user3,
      ~isLong=true,
    );

  let%AwaitThen _ = priceAndStateUpdate();

  Js.log("Update treasury base price");
  let%AwaitThen _ =
    treasury
    ->ContractHelpers.connect(~address=admin)
    ->TreasuryAlpha.updateBasePrice(
        ~newBasePrice=bnFromString("300000000000000000"),
      );

  Js.log2("User1 minting float:", user1.address);
  let%AwaitThen _ =
    claimFloatForUser(~marketIndexes=initialMarkets, ~staker, ~user=user1);

  Js.log("Withdraw long stake");

  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      withdrawStakeSynthLong(
        ~longShort,
        ~staker,
        ~marketIndex=_,
        ~user=user1,
      ),
    );

  Js.log("Transfer tokens from user1 to user2");
  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      transferSynthLong(
        ~longShort,
        ~userFrom=user1,
        ~userTo=user2,
        ~marketIndex=_,
      ),
    );

  Js.log("Executing Long Mint and Stakes");
  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      mintAndStakeNextPriceWithSystemStateUpdate(
        ~amount=longMintAmount,
        ~marketIndex=_,
        ~paymentToken,
        ~longShort,
        ~user=user1,
        ~admin,
        ~isLong=true,
      ),
    );

  Js.log("Executing Short Mint and Stakes");
  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      mintAndStakeNextPriceWithSystemStateUpdate(
        ~amount=shortMintAmount,
        ~marketIndex=_,
        ~paymentToken,
        ~longShort,
        ~user=user1,
        ~admin,
        ~isLong=false,
      ),
    );

  Js.log("Update float percentage");

  let%AwaitThen _ =
    updateFloatPercentage(
      ~staker,
      ~admin,
      ~newFloatPercentage=bnFromString("333333333333333333"),
    );

  Js.log("Change Balance incentive paramaters");

  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      updateBalanceIncentiveParameters(
        ~staker,
        ~admin,
        ~marketIndex=_,
        ~balanceIncentiveCurve_exponent=bnFromInt(5),
        ~balanceIncentiveCurve_equilibriumOffset=bnFromInt(0),
        ~safeExponentBitShifting=bnFromInt(50),
      ),
    );

  Js.log("Change stake withdrawal fees");

  let%AwaitThen _ =
    executeOnMarkets(
      initialMarkets,
      updateStakeWithdrawalFee(
        ~staker,
        ~admin,
        ~marketIndex=_,
        ~newMarketUnstakeFee_e18=bnFromString("4000000000000000"),
      ),
    ); //40 basis points

  Promise.resolve();
};
