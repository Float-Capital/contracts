// The type system isn't very happy here, but it seems to work for now (even if it is fragile)
let createValueAtKey = %raw(`(key, value) => {
  let result = {}
  result[key] = value
  return result
}`)

let turnOffMocking = longShort => {
  longShort->LongShort.setVariable(~name="shouldUseMock", ~value=false)
}

let setAssetPrice = (longShort, ~marketIndex: int, ~assetPrice: Ethers.BigNumber.t) => {
  longShort->LongShort.setVariable(
    ~name="assetPrice",
    ~value=createValueAtKey(marketIndex, assetPrice),
  )
}

let setStaker = (longShort, ~stakerAddress) => {
  longShort->LongShort.setVariable(~name="staker", ~value=stakerAddress)
}

let setFundingRateMultiplier = (longShort, ~marketIndex, ~fundingRate) => {
  longShort->LongShort.setVariable(
    ~name="fundingRateMultiplier_e18",
    ~value=createValueAtKey(marketIndex, fundingRate),
  )
}

let setMarketUpdateIndex = (
  longShort,
  ~marketIndex: int,
  ~marketUpdateIndex: Ethers.BigNumber.t,
) => {
  longShort->LongShort.setVariable(
    ~name="marketUpdateIndex",
    ~value=createValueAtKey(marketIndex, marketUpdateIndex),
  )
}
let setLatestMarket = (longShort, ~latestMarket: int) => {
  longShort->LongShort.setVariable(~name="latestMarket", ~value=latestMarket)
}

let setYieldManager = (longShort, ~marketIndex: int, ~yieldManager: Ethers.ethAddress) => {
  longShort->LongShort.setVariable(
    ~name="yieldManagers",
    ~value=createValueAtKey(marketIndex, yieldManager),
  )
}

let setMarketLeverage_e18 = (
  longShort,
  ~marketIndex: int,
  ~marketLeverage_e18: Ethers.BigNumber.t,
) => {
  longShort->LongShort.setVariable(
    ~name="marketLeverage_e18",
    ~value=createValueAtKey(marketIndex, marketLeverage_e18),
  )
}
