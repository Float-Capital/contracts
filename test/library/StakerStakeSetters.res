// The type system isn't very happy here, but it seems to work for now (even if it is fragile)
let createValueAtKey = %raw(`(key, value) => {
  let result = {}
  result[key] = value
  return result
}`)

let turnOffMocking = staker => {
  staker->Staker.setVariable(~name="shouldUseMock", ~value=false)
}

let setUserIndexOfLastClaimedReward = (
  staker,
  ~marketIndex: int,
  ~user: Ethers.ethAddress,
  ~rewardIndex: Ethers.BigNumber.t,
) => {
  staker->Staker.setVariable(
    ~name="userIndexOfLastClaimedReward",
    ~value=createValueAtKey(marketIndex, createValueAtKey(user, rewardIndex)),
  )
}
