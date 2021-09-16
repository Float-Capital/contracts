open LetOps;
open DeployHelpers;
open Globals;

type allContracts = {
  staker: Staker.t,
  longShort: LongShort.t,
  paymentToken: ERC20Mock.t,
  treasury: TreasuryAlpha.t,
  syntheticToken: SyntheticToken.t,
};

let runTestTransactions =
    (
      {longShort, treasury, paymentToken},
      _deploymentArgs: Hardhat.hardhatDeployArgument,
    ) => {
  let%Await loadedAccounts = Ethers.getSigners();

  let admin = loadedAccounts->Array.getUnsafe(1);
  let user1 = loadedAccounts->Array.getUnsafe(2);
  let user2 = loadedAccounts->Array.getUnsafe(3);
  let user3 = loadedAccounts->Array.getUnsafe(4);

  let%AwaitThen _ = DeployHelpers.topupBalanceIfLow(~from=admin, ~to_=user1);
  let%AwaitThen _ = DeployHelpers.topupBalanceIfLow(~from=admin, ~to_=user2);
  let%AwaitThen _ = DeployHelpers.topupBalanceIfLow(~from=admin, ~to_=user3);

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
  let longStakeAmount = bnFromInt(1);

  let priceAndStateUpdate = () => {
    let%AwaitThen _ =
      executeOnMarkets(
        initialMarkets,
        setOracleManagerPrice(~longShort, ~marketIndex=_, ~admin),
      );

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
        ~amount=redeemShortAmount,
        ~marketIndex=_,
        ~longShort,
        ~user=user1,
        ~admin,
      ),
    );

  let%AwaitThen _ = priceAndStateUpdate();

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

  let%AwaitThen _ = priceAndStateUpdate();

  Js.log("Update treasury base price");
  let%AwaitThen _ =
    treasury
    ->ContractHelpers.connect(~address=admin)
    ->TreasuryAlpha.updateBasePrice(
        ~newBasePrice=bnFromString("300000000000000000"),
      );

  JsPromise.resolve();
};
