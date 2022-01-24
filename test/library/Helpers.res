open Globals
type markets = {
  paymentToken: ERC20Mock.t,
  oracleManager: OracleManagerMock.t,
  yieldManager: YieldManagerMock.t,
  longSynth: SyntheticToken.t,
  shortSynth: SyntheticToken.t,
  marketIndex: int,
}
type coreContracts = {
  floatCapital_v0: FloatCapital_v0.t,
  tokenFactory: TokenFactory.t,
  treasury: Treasury_v0.t,
  floatToken: FloatToken.t,
  staker: Staker.t,
  longShort: LongShort.t,
  gems: GEMS.t,
  markets: array<markets>,
}

module Tuple = {
  let make2 = fn => (fn(), fn())
  let make3 = fn => (fn(), fn(), fn())
  let make4 = fn => (fn(), fn(), fn(), fn())
  let make5 = fn => (fn(), fn(), fn(), fn(), fn())
  let make6 = fn => (fn(), fn(), fn(), fn(), fn(), fn())
  let make7 = fn => (fn(), fn(), fn(), fn(), fn(), fn(), fn())
  let make8 = fn => (fn(), fn(), fn(), fn(), fn(), fn(), fn(), fn())
}

@ocaml.doc(`Generates random BigNumber between 1 and 2147483647 (max js int)`)
let randomInteger = () => Js.Math.random_int(1, Js.Int.max)->Ethers.BigNumber.fromInt

@ocaml.doc(`Generates random BigNumber between x and y`)
let randomBigIntInRange = (x, y) => Js.Math.random_int(x, y)->Ethers.BigNumber.fromInt

@ocaml.doc(`Generates a random JS integer between 0 and 2147483647 (max js int)`)
let randomJsInteger = () => Js.Math.random_int(0, Js.Int.max)

let randomRatio1e18 = () =>
  bnFromString(
    Js.Math.random_int(0, 1000000000)->Int.toString ++
      Js.Math.random_int(0, 1000000000)->Int.toString,
  )

let adjustNumberRandomlyWithinRange = (~basisPointsMin, ~basisPointsMax, number) => {
  let numerator = Js.Math.random_int(basisPointsMin, basisPointsMax)->bnFromInt

  number->add(number->mul(numerator)->div(bnFromInt(100000)))
}

let accessControlErrorMessage = (~address, ~roleBytesStr) => {
  "AccessControl: account " ++
  address->Ethers.Utils.ethAdrToLowerStr ++
  " is missing role " ++
  roleBytesStr
}

let adminRoleBytesString = "0xa49807205ce4d355092ef5a8a18f56e8913cf4a201fbe287825b095693c21775"
let adminErrorMessage = (~address) =>
  accessControlErrorMessage(~address, ~roleBytesStr=adminRoleBytesString)

@ocaml.doc(`Generates random BigNumber between 0.01 and 21474836.47 of a token (10^18 in BigNumber units)`)
let randomTokenAmount = () =>
  randomInteger()->Ethers.BigNumber.mul(Ethers.BigNumber.fromUnsafe("10000000000000000"))

type mint =
  | Long(Ethers.BigNumber.t)
  | Short(Ethers.BigNumber.t)
  | Both(Ethers.BigNumber.t, Ethers.BigNumber.t)

let randomMintLongShort = () => {
  switch Js.Math.random_int(0, 3) {
  | 0 => Long(randomTokenAmount())
  | 1 => Short(randomTokenAmount())
  | 2
  | _ =>
    Both(randomTokenAmount(), randomTokenAmount())
  }
}

let randomAddress = () => Ethers.Wallet.createRandom().address

let createSyntheticMarket = (
  ~admin,
  ~initialMarketSeedForEachMarketSide=CONSTANTS.tenToThe18,
  ~paymentToken: ERC20Mock.t,
  ~treasury,
  ~marketName,
  ~marketSymbol,
  longShort: LongShort.t,
) => {
  Promise.all3((
    OracleManagerMock.make(~admin, ~maxUpdateIntervalSeconds=bnFromInt(0)),
    YieldManagerMock.make(~longShort=longShort.address, ~token=paymentToken.address, ~treasury),
    paymentToken
    ->ERC20Mock.mint(~_to=admin, ~amount=initialMarketSeedForEachMarketSide->mul(bnFromInt(100)))
    ->Promise.then(_ =>
      paymentToken->ERC20Mock.approve(
        ~spender=longShort.address,
        ~amount=initialMarketSeedForEachMarketSide->mul(bnFromInt(100)),
      )
    ),
  ))->Promise.then(((oracleManager, yieldManager, _)) => {
    let _ignorePromise =
      paymentToken
      ->ERC20Mock.mINTER_ROLE
      ->Promise.thenResolve(minterRole =>
        paymentToken->ERC20Mock.grantRole(~role=minterRole, ~account=yieldManager.address)
      )
    longShort
    ->LongShort.createNewSyntheticMarket(
      ~syntheticName=marketName,
      ~syntheticSymbol=marketSymbol,
      ~paymentToken=paymentToken.address,
      ~oracleManager=oracleManager.address,
      ~yieldManager=yieldManager.address,
    )
    ->Promise.then(_ => longShort->LongShort.latestMarket)
    ->Promise.then(marketIndex => {
      longShort->LongShort.initializeMarket(
        ~marketIndex,
        ~kInitialMultiplier=CONSTANTS.tenToThe18,
        ~kPeriod=Ethers.BigNumber.fromInt(0),
        ~unstakeFee_e18=Ethers.BigNumber.fromInt(50),
        ~initialMarketSeedForEachMarketSide,
        ~balanceIncentiveCurve_exponent=bnFromInt(5),
        ~balanceIncentiveCurve_equilibriumOffset=bnFromInt(0),
        ~marketTreasurySplitGradient_e18=bnFromInt(1),
        ~marketLeverage=CONSTANTS.tenToThe18,
      )
    })
  })
}

let getAllMarkets = longShort => {
  longShort
  ->LongShort.latestMarket
  ->Promise.then(nextMarketIndex => {
    let marketIndex = nextMarketIndex

    Belt.Array.range(1, marketIndex)
    ->Array.map(marketIndex =>
      Promise.all5((
        longShort
        ->LongShort.syntheticTokens(marketIndex, true /* long */)
        ->Promise.then(SyntheticToken.at),
        longShort
        ->LongShort.syntheticTokens(marketIndex, false /* short */)
        ->Promise.then(SyntheticToken.at),
        longShort->LongShort.paymentTokens(marketIndex)->Promise.then(ERC20Mock.at),
        longShort->LongShort.oracleManagers(marketIndex)->Promise.then(OracleManagerMock.at),
        longShort->LongShort.yieldManagers(marketIndex)->Promise.then(YieldManagerMock.at),
      ))->Promise.thenResolve(((
        longSynth,
        shortSynth,
        paymentToken,
        oracleManager,
        yieldManager,
      )) => {
        {
          paymentToken: paymentToken,
          oracleManager: oracleManager,
          yieldManager: yieldManager,
          longSynth: longSynth,
          shortSynth: shortSynth,
          marketIndex: marketIndex,
        }
      })
    )
    ->Promise.all
  })
}

@scope("Promise") @val
external promiseAll7: (
  (
    Promise.t<'a>,
    Promise.t<'b>,
    Promise.t<'c>,
    Promise.t<'d>,
    Promise.t<'e>,
    Promise.t<'f>,
    Promise.t<'g>,
  )
) => Promise.t<('a, 'b, 'c, 'd, 'e, 'f, 'g)> = "all"

let initialize = (~admin: Ethers.Wallet.t, ~exposeInternals: bool) => {
  promiseAll7((
    FloatCapital_v0.make(),
    Treasury_v0.make(),
    FloatToken.make(),
    exposeInternals ? Staker.Exposed.make() : Staker.make(),
    exposeInternals ? LongShort.Exposed.make() : LongShort.make(),
    Promise.all2((
      ERC20Mock.make(~name="Pay Token 1", ~symbol="PT1"),
      ERC20Mock.make(~name="Pay Token 2", ~symbol="PT2"),
    )),
    GEMS.make(),
  ))->Promise.then(((
    floatCapital,
    treasury,
    floatToken,
    staker,
    longShort,
    (payToken1, payToken2),
    gems,
  )) => {
    TokenFactory.make(~longShort=longShort.address)->Promise.then(tokenFactory => {
      Promise.all5((
        floatToken->FloatToken.initialize(
          ~name="Float token",
          ~symbol="FLOAT TOKEN",
          ~stakerAddress=staker.address,
        ),
        treasury->Treasury_v0.initialize(
          ~admin=admin.address,
          ~paymentToken=payToken1.address,
          ~floatToken=floatToken.address,
          ~longShort=longShort.address,
        ),
        gems->GEMS.initialize(
          ~admin=admin.address,
          ~longShort=longShort.address,
          ~staker=staker.address,
        ),
        longShort->LongShort.initialize(
          ~admin=admin.address,
          ~tokenFactory=tokenFactory.address,
          ~staker=staker.address,
          ~gems=gems.address,
        ),
        staker->Staker.initialize(
          ~admin=admin.address,
          ~longShort=longShort.address,
          ~floatToken=floatToken.address,
          ~floatCapital=floatCapital.address,
          // NOTE: for now using the floatCapital address as the float treasury
          ~floatTreasury=floatCapital.address,
          // NOTE: for now using the admin address as the discount signer
          ~discountSigner=admin.address,
          ~floatPercentage=bnFromString("250000000000000000"),
          ~gems=gems.address,
        ),
      ))
      ->Promise.then(_ => {
        [payToken1, payToken1, payToken2, payToken1]
        ->Array.reduceWithIndex(Promise.resolve(), (previousPromise, paymentToken, index) => {
          previousPromise->Promise.then(() =>
            longShort->createSyntheticMarket(
              ~admin=admin.address,
              ~treasury=treasury.address,
              ~paymentToken,
              ~marketName=`Test Market ${index->Int.toString}`,
              ~marketSymbol=`TM${index->Int.toString}`,
            )
          )
        })
        ->Promise.then(_ => {
          longShort->getAllMarkets
        })
      })
      ->Promise.thenResolve(markets => {
        staker: staker,
        longShort: longShort,
        floatToken: floatToken,
        tokenFactory: tokenFactory,
        treasury: treasury,
        markets: markets,
        floatCapital_v0: floatCapital,
        gems: gems,
      })
    })
  })
}

type stakerUnitTestContracts = {
  staker: Staker.t,
  longShortSmocked: LongShortSmocked.t,
  floatTokenSmocked: FloatTokenSmocked.t,
  syntheticTokenSmocked: SyntheticTokenSmocked.t,
  floatCapitalSmocked: FloatCapital_v0.t,
  gems: GEMS.t,
}

let initializeStakerUnit = () => {
  Promise.all6((
    Staker.Exposed.makeSmock()->Promise.then(staker => {
      staker->StakerSmocked.InternalMock.setup->Promise.thenResolve(_ => staker)
    }),
    LongShortSmocked.make(),
    FloatTokenSmocked.make(),
    SyntheticTokenSmocked.make(),
    FloatCapital_v0.make(),
    GEMS.make(),
  ))->Promise.then(((
    staker,
    longShortSmocked,
    floatTokenSmocked,
    syntheticTokenSmocked,
    floatCapitalSmocked,
    gems,
  )) =>
    staker
    ->Staker.setVariable(~name="longShort", ~value=longShortSmocked.address)
    ->Promise.thenResolve(_ => staker->Staker.setVariable(~name="gems", ~value=gems.address))
    ->Promise.thenResolve(_ => {
      staker: staker,
      longShortSmocked: longShortSmocked,
      floatTokenSmocked: floatTokenSmocked,
      syntheticTokenSmocked: syntheticTokenSmocked,
      floatCapitalSmocked: floatCapitalSmocked,
      gems: gems,
    })
  )
}

type longShortUnitTestContracts = {
  longShort: LongShort.t,
  stakerSmocked: StakerSmocked.t,
  floatTokenSmocked: FloatTokenSmocked.t,
  syntheticToken1Smocked: SyntheticTokenSmocked.t,
  syntheticToken2Smocked: SyntheticTokenSmocked.t,
  tokenFactorySmocked: TokenFactorySmocked.t,
  yieldManagerSmocked: YieldManagerAaveBasicSmocked.t,
  oracleManagerSmocked: OracleManagerMockSmocked.t,
}

let deployAYieldManager = (~longShort: Ethers.ethAddress, ~lendingPoolAddressesProvider) => {
  ERC20Mock.make(~name="Pay Token 1", ~symbol="PT1")->Promise.then(paymentToken =>
    YieldManagerAaveBasic.make()->Promise.then(manager =>
      manager
      ->YieldManagerAaveBasic.initialize(
        ~longShort,
        ~treasury=randomAddress(),
        ~paymentToken=paymentToken.address,
        ~aToken=randomAddress(),
        ~lendingPoolAddressesProvider,
        ~aaveIncentivesController=randomAddress(),
        ~aaveReferralCode=0,
        ~admin=randomAddress(),
      )
      ->Promise.thenResolve(_ => manager)
    )
  )
}

@scope("Promise") @val
external promiseAll9: (
  (
    Promise.t<'a>,
    Promise.t<'b>,
    Promise.t<'c>,
    Promise.t<'d>,
    Promise.t<'e>,
    Promise.t<'f>,
    Promise.t<'g>,
    Promise.t<'h>,
    Promise.t<'i>,
  )
) => Promise.t<('a, 'b, 'c, 'd, 'e, 'f, 'g, 'h, 'i)> = "all"

let initializeLongShortUnit = () => {
  promiseAll9((
    LongShort.Exposed.makeSmock()->Promise.then(staker => {
      staker->LongShortSmocked.InternalMock.setup->Promise.thenResolve(_ => staker)
    }),
    StakerSmocked.make(),
    FloatTokenSmocked.make(),
    SyntheticTokenSmocked.make(),
    SyntheticTokenSmocked.make(),
    TokenFactorySmocked.make(),
    YieldManagerAaveBasicSmocked.make(),
    OracleManagerMockSmocked.make(),
    GEMS.make(),
  ))->Promise.thenResolve(((
    longShort,
    stakerSmocked,
    floatTokenSmocked,
    syntheticToken1Smocked,
    syntheticToken2Smocked,
    tokenFactorySmocked,
    yieldManagerSmocked,
    oracleManagerSmocked,
    _gems,
  )) => {
    {
      longShort: longShort,
      stakerSmocked: stakerSmocked,
      floatTokenSmocked: floatTokenSmocked,
      yieldManagerSmocked: yieldManagerSmocked,
      oracleManagerSmocked: oracleManagerSmocked,
      syntheticToken1Smocked: syntheticToken1Smocked,
      syntheticToken2Smocked: syntheticToken2Smocked,
      tokenFactorySmocked: tokenFactorySmocked,
    }
  })
}

let increaseTime: int => Promise.t<
  unit,
> = %raw(`(seconds) => ethers.provider.send("evm_increaseTime", [seconds])`)

type block = {timestamp: int}
let getBlock: unit => Promise.t<block> = %raw(`() => ethers.provider.getBlock()`)

let getRandomTimestampInPast = () => {
  getBlock()->Promise.then(({timestamp}) => {
    (timestamp - Js.Math.random_int(200, 630720000))->Ethers.BigNumber.fromInt->Promise.resolve
  })
}
