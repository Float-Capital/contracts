open Ethers;
open LetOps;
open Globals;

let minSenderBalance = bnFromString("50000000000000000");
let minRecieverBalance = bnFromString("20000000000000000");

let topupBalanceIfLow = (~from: Wallet.t, ~to_: Wallet.t) => {
  let%AwaitThen senderBalance = from->Wallet.getBalance;

  if (senderBalance->bnLt(minSenderBalance)) {
    Js.Exn.raiseError(
      "WARNING - Sender doesn't have enough eth - need at least 0.05 ETH! (top up to over 1 ETH to be safe)",
    );
  };
  let%Await recieverBalance = to_->Wallet.getBalance;
  if (recieverBalance->bnLt(minRecieverBalance)) {
    let _ =
      from->Wallet.sendTransaction({
        to_: to_.address,
        value: minRecieverBalance,
      });
    ();
  };
};

let setOracleManagerPrice = (~longShort, ~marketIndex, ~admin) => {
  let%AwaitThen oracleManagerAddr =
    longShort->LongShort.oracleManagers(marketIndex);
  let%AwaitThen oracleManager = OracleManagerMock.at(oracleManagerAddr);

  let%AwaitThen currentPrice = oracleManager->OracleManagerMock.getLatestPrice;
  let nextPrice = currentPrice->mul(bnFromInt(101))->div(bnFromInt(100));

  oracleManager
  ->ContractHelpers.connect(~address=admin)
  ->OracleManagerMock.setPrice(~newPrice=nextPrice);
};

let updateSystemState = (~longShort, ~admin, ~marketIndex) => {
  let%AwaitThen _ = setOracleManagerPrice(~longShort, ~marketIndex, ~admin);

  let%AwaitThen _ = Helpers.increaseTime(5);
  longShort
  ->ContractHelpers.connect(~address=admin)
  ->LongShort.updateSystemState(~marketIndex);
};

let mintAndApprove = (~paymentToken, ~amount, ~user, ~approvedAddress) => {
  let%AwaitThen _ = paymentToken->ERC20Mock.mint(~_to=user.address, ~amount);

  paymentToken
  ->ContractHelpers.connect(~address=user)
  ->ERC20Mock.approve(~spender=approvedAddress, ~amount);
};

let mintNextPrice =
    (
      ~amount,
      ~marketIndex,
      ~paymentToken,
      ~longShort: LongShort.t,
      ~user,
      ~isLong,
    ) => {
  let%AwaitThen _ =
    mintAndApprove(
      ~paymentToken,
      ~amount,
      ~user,
      ~approvedAddress=longShort.address,
    );

  let mintFunction =
    isLong ? LongShort.mintLongNextPrice : LongShort.mintShortNextPrice;

  longShort
  ->ContractHelpers.connect(~address=user)
  ->mintFunction(~marketIndex, ~amount);
};

let mintLongNextPriceWithSystemUpdate =
    (
      ~amount,
      ~marketIndex,
      ~paymentToken,
      ~longShort: LongShort.t,
      ~user,
      ~admin,
    ) => {
  let%AwaitThen _ =
    mintNextPrice(
      ~amount,
      ~marketIndex,
      ~paymentToken,
      ~longShort: LongShort.t,
      ~user,
      ~isLong=true,
    );

  updateSystemState(~longShort, ~admin, ~marketIndex);
};

let mintShortNextPriceWithSystemUpdate =
    (
      ~amount,
      ~marketIndex,
      ~paymentToken,
      ~longShort: LongShort.t,
      ~user,
      ~admin,
    ) => {
  let%AwaitThen _ =
    mintNextPrice(
      ~amount,
      ~marketIndex,
      ~paymentToken,
      ~longShort: LongShort.t,
      ~user,
      ~isLong=false,
    );

  updateSystemState(~longShort, ~admin, ~marketIndex);
};

let mintAndStakeNextPriceWithSystemStateUpdate =
    (
      ~amount,
      ~marketIndex,
      ~paymentToken,
      ~longShort: LongShort.t,
      ~user,
      ~admin,
      ~isLong,
    ) => {
  let%AwaitThen _ =
    mintAndApprove(
      ~paymentToken,
      ~amount,
      ~user,
      ~approvedAddress=longShort.address,
    );
  let%AwaitThen _ =
    longShort
    ->ContractHelpers.connect(~address=user)
    ->LongShort.mintAndStakeNextPrice(~marketIndex, ~amount, ~isLong);

  updateSystemState(~longShort, ~admin, ~marketIndex);
};

let redeemNextPrice = (~amount, ~marketIndex, ~longShort, ~user, ~isLong) => {
  let redeemFunction =
    isLong ? LongShort.redeemLongNextPrice : LongShort.redeemShortNextPrice;
  longShort
  ->ContractHelpers.connect(~address=user)
  ->redeemFunction(~marketIndex, ~tokens_redeem=amount);
};

let redeemShortNextPriceWithSystemUpdate =
    (~amount, ~marketIndex, ~longShort, ~user, ~admin) => {
  let%AwaitThen _ =
    redeemNextPrice(~amount, ~marketIndex, ~longShort, ~user, ~isLong=false);
  updateSystemState(~longShort, ~admin, ~marketIndex);
};

let redeemLongNextPriceWithSystemUpdate =
    (~amount, ~marketIndex, ~longShort, ~user, ~admin) => {
  let%AwaitThen _ =
    redeemNextPrice(~amount, ~marketIndex, ~longShort, ~user, ~isLong=true);
  updateSystemState(~longShort, ~admin, ~marketIndex);
};

let shiftNextPrice =
    (
      ~amountSyntheticTokensToShift,
      ~marketIndex,
      ~longShort: LongShort.t,
      ~user,
      ~isLong,
    ) => {
  let shiftFunction =
    isLong
      ? LongShort.shiftPositionFromLongNextPrice
      : LongShort.shiftPositionFromShortNextPrice;

  longShort
  ->ContractHelpers.connect(~address=user)
  ->shiftFunction(~marketIndex, ~amountSyntheticTokensToShift);
};

let shiftFromShortNextPriceWithSystemUpdate =
    (~amountSyntheticTokensToShift, ~marketIndex, ~longShort, ~user, ~admin) => {
  let%AwaitThen _ =
    shiftNextPrice(
      ~amountSyntheticTokensToShift,
      ~marketIndex,
      ~longShort: LongShort.t,
      ~user,
      ~isLong=false,
    );

  updateSystemState(~longShort, ~admin, ~marketIndex);
};
let shiftFromLongNextPriceWithSystemUpdate =
    (~amountSyntheticTokensToShift, ~marketIndex, ~longShort, ~user, ~admin) => {
  let%AwaitThen _ =
    shiftNextPrice(
      ~amountSyntheticTokensToShift,
      ~marketIndex,
      ~longShort: LongShort.t,
      ~user,
      ~isLong=true,
    );

  updateSystemState(~longShort, ~admin, ~marketIndex);
};

let shiftStakeNextPriceWithSystemUpdate =
    (
      ~amount,
      ~isShiftFromLong,
      ~marketIndex,
      ~longShort,
      ~staker,
      ~user,
      ~admin,
    ) => {
  Js.log2("Amount to shift is", amount->bnToString);
  let%AwaitThen _ =
    staker
    ->ContractHelpers.connect(~address=user)
    ->Staker.shiftTokens(
        ~isShiftFromLong,
        ~marketIndex,
        ~amountSyntheticTokensToShift=amount,
      );
  updateSystemState(~longShort, ~admin, ~marketIndex);
};

let stakeSynthLong = (~amount, ~longShort, ~marketIndex, ~user) => {
  let%AwaitThen longAddress =
    longShort->LongShort.syntheticTokens(marketIndex, true);
  let%AwaitThen synth = SyntheticToken.at(longAddress);
  let%Await usersSyntheticTokenBalance =
    synth->SyntheticToken.balanceOf(~account=user.address);
  if (usersSyntheticTokenBalance->bnGt(bnFromString("0"))) {
    let _ =
      synth
      ->ContractHelpers.connect(~address=user)
      ->SyntheticToken.stake(~amount);
    ();
  };
};

let executeOnMarkets =
    (marketIndexes: array(int), functionToExecute: int => Js.Promise.t('a)) => {
  marketIndexes->Array.reduce(
    Promise.resolve(),
    (previousPromise, marketIndex) => {
      let%AwaitThen _ = previousPromise;
      functionToExecute(marketIndex);
    },
  );
};

let stakeSynthShort = (~amount, ~longShort, ~marketIndex, ~user) => {
  let%AwaitThen shortAddress =
    longShort->LongShort.syntheticTokens(marketIndex, false);
  let%AwaitThen synth = SyntheticToken.at(shortAddress);
  let%Await usersSyntheticTokenBalance =
    synth->SyntheticToken.balanceOf(~account=user.address);
  if (usersSyntheticTokenBalance->bnGt(bnFromString("0"))) {
    let _ =
      synth
      ->ContractHelpers.connect(~address=user)
      ->SyntheticToken.stake(~amount);
    ();
  };
};

let withdrawStakeSynthLong = (~longShort, ~staker, ~marketIndex, ~user) => {
  let%AwaitThen longAddress =
    longShort->LongShort.syntheticTokens(marketIndex, true);

  let%Await longStakeBalance =
    staker->Staker.userAmountStaked(longAddress, user.address);
  if (longStakeBalance->bnGt(bnFromString("0"))) {
    let _ =
      staker
      ->ContractHelpers.connect(~address=user)
      ->Staker.withdraw(
          ~marketIndex,
          ~isWithdrawFromLong=true,
          ~amount=longStakeBalance,
        );
    ();
  };
};

let transferSynthLong = (~longShort, ~userFrom, ~userTo, ~marketIndex) => {
  let%AwaitThen longAddress =
    longShort->LongShort.syntheticTokens(marketIndex, true);
  let%AwaitThen synth = SyntheticToken.at(longAddress);
  let%AwaitThen balance =
    synth->SyntheticToken.balanceOf(~account=userFrom.address);

  synth
  ->ContractHelpers.connect(~address=userFrom)
  ->SyntheticToken.transfer(
      ~recipient=userTo.address,
      ~amount=balance->BigNumber.div(bnFromString("2")),
    );
};

let claimFloatForUser = (~marketIndexes, ~staker, ~user) => {
  let%Await _ =
    staker
    ->ContractHelpers.connect(~address=user)
    ->Staker.claimFloatCustom(~marketIndexes);
  ();
};

let updateFloatPercentage = (~staker, ~admin, ~newFloatPercentage) => {
  let%Await _ =
    staker
    ->ContractHelpers.connect(~address=admin)
    ->Staker.changeFloatPercentage(~newFloatPercentage);
  ();
};

let updateBalanceIncentiveParameters =
    (
      ~staker,
      ~admin,
      ~marketIndex,
      ~balanceIncentiveCurve_exponent,
      ~balanceIncentiveCurve_equilibriumOffset,
      ~safeExponentBitShifting,
    ) => {
  let%Await _ =
    staker
    ->ContractHelpers.connect(~address=admin)
    ->Staker.changeBalanceIncentiveParameters(
        ~marketIndex,
        ~balanceIncentiveCurve_exponent,
        ~balanceIncentiveCurve_equilibriumOffset,
        ~safeExponentBitShifting,
      );
  ();
};

let updateStakeWithdrawalFee =
    (~staker, ~admin, ~marketIndex, ~newMarketUnstakeFee_e18) => {
  let%Await _ =
    staker
    ->ContractHelpers.connect(~address=admin)
    ->Staker.changeUnstakeFee(~marketIndex, ~newMarketUnstakeFee_e18);
  ();
};
