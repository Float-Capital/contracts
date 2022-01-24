open ContractHelpers;
open Globals;
open LetOps;

module PaymentTokenHelpers = {
  let mintAndApprove =
      (
        t: ERC20Mock.t,
        ~user: Ethers.Wallet.t,
        ~amount: Ethers.BigNumber.t,
        ~spender: Ethers.ethAddress,
      ) =>
    t
    ->ERC20Mock.mint(~amount, ~_to=user.address)
    ->promiseThen(_ => {
        t->connect(~address=user)->ERC20Mock.approve(~amount, ~spender)
      });
};

module DataFetchers = {
  let marketIndexOfSynth =
      (longShort: LongShort.t, ~syntheticToken: SyntheticToken.t)
      : Promise.t(int) =>
    longShort
    ->LongShort.staker
    ->promiseThen(Staker.at)
    ->promiseThen(Staker.marketIndexOfToken(_, syntheticToken.address));
};

module LongShortHelpers = {
  let getSyntheticTokenPrice = (longShort, ~marketIndex, ~isLong) => {
    let%AwaitThen syntheticTokenAddress =
      longShort->LongShort.syntheticTokens(marketIndex, isLong);
    let%AwaitThen synthContract =
      ContractHelpers.attachToContract(
        "SyntheticToken",
        ~contractAddress=syntheticTokenAddress,
      );
    let%AwaitThen totalSupply =
      synthContract->Obj.magic->SyntheticToken.totalSupply;

    let%Await marketValuesInPaymentToken =
      longShort->LongShort.marketSideValueInPaymentToken(marketIndex);
    let marketSideValueInPaymentToken =
      isLong
        ? marketValuesInPaymentToken.value_long
        : marketValuesInPaymentToken.value_short;

    let syntheticTokenPrice =
      marketSideValueInPaymentToken
      ->mul(CONSTANTS.tenToThe18)
      ->div(totalSupply);

    syntheticTokenPrice;
  };
  let calcSyntheticTokenPrice = (~amountPaymentToken, ~amountSyntheticToken) => {
    amountPaymentToken->mul(CONSTANTS.tenToThe18)->div(amountSyntheticToken);
  };
  let calcAmountPaymentToken = (~amountSyntheticToken, ~price) => {
    amountSyntheticToken->mul(price)->div(CONSTANTS.tenToThe18);
  };
  let calcAmountSyntheticToken = (~amountPaymentToken, ~price) => {
    amountPaymentToken->mul(CONSTANTS.tenToThe18)->div(price);
  };
  let calcEquivalentAmountSyntheticTokensOnTargetSide =
      (~amountSyntheticTokenOriginSide, ~priceOriginSide, ~priceTargetSide) => {
    amountSyntheticTokenOriginSide
    ->mul(priceOriginSide)
    ->div(priceTargetSide);
  };
};

module SyntheticTokenHelpers = {
  let getIsLong = syntheticToken => {
    let%Await isLong = syntheticToken->SyntheticToken.isLong;
    isLong == true /*long*/;
  };
};

module YieldManagerAaveBasicHelpers = {
  type contractsType = {
    .
    "aToken": ERC20MockSmocked.t,
    "yieldManagerAave": YieldManagerAaveBasic.t,
    "paymentToken": ERC20MockSmocked.t,
    "treasury": Ethers.Wallet.t,
    "aaveIncentivesController": AaveIncentivesControllerMockSmocked.t,
  };
};
