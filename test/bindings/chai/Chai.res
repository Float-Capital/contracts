// Quick and dirty bindings from here: https://ethereum-waffle.readthedocs.io/en/latest/matchers.html

// NOTE: no effort in making these binding proffessional. Mostly just raw javascript.
%%raw(`
const { expect } = require("chai");
`)

let bnEqual: (
  ~message: string=?,
  Ethers.BigNumber.t,
  Ethers.BigNumber.t,
) => unit = %raw(`(message, number1, number2) => expect(number1, message).to.equal(number2)`)

let intEqual: (
  ~message: string=?,
  int,
  int,
) => unit = %raw(`(message, number1, number2) => expect(number1, message).to.equal(number2)`)

let recordEqualFlat: ('a, 'a) => unit = (expected, actual) => {
  let a = %raw("(expected, actual) => {
    for(const key of Object.keys(actual)){
      expect(actual[key]).to.equal(expected[key])
    }
  }")
  a(expected, actual)
}
let recordEqualDeep: (~message: string=?, 'a, 'a) => unit = (~message="", expected, actual) => {
  let a = %raw("(message, expected, actual) => {
    for(const key of Object.keys(actual)){
      expect(actual[key], message + ` at key \"${key}\"`).to.deep.equal(expected[key])
    }
  }")
  a(message, expected, actual)
}
/* // Keeping these for reference.
let recordEqualFlatLabeled: (~expected: 'a, ~actual: 'a) => unit = (~expected, ~actual) => {
  let a = %raw("(expected, actual) => {
    for(const key of Object.keys(actual)){
      expect(actual[key]).to.equal(expected[key])
    }
  }")
  a(expected, actual)
}
let recordArrayEqualFlat: (array<'a>, array<'a>) => unit = (expected, actual) => {
  intEqual(
    ~message="cannot compare arrays of integers with different lengths",
    expected->Array.length,
    actual->Array.length,
  )
  expected->Array.forEachWithIndex((i, expectedResult) =>
    recordEqualFlat(expectedResult, actual->Array.getUnsafe(i))
  )
}
let recordArrayDeepEqualFlat: (~message: string=?, array<'a>, array<'a>) => unit = (
  ~message="record array equality check",
  expected,
  actual,
) => {
  intEqual(expected->Array.length, actual->Array.length)
  expected->Array.forEachWithIndex((i, expectedResult) =>
    recordEqualDeep(
      ~message=`${message}: at index #${i->Int.toString}`,
      expectedResult,
      actual->Array.getUnsafe(i),
    )
  )
} */

let addressEqual: (
  ~message: string=?,
  ~otherAddress: Ethers.ethAddress,
  Ethers.ethAddress,
) => unit = %raw(`(message, address1, address2) => expect(address1, message).to.equal(address2)`)

let boolEqual: (
  ~message: string=?,
  bool,
  bool,
) => unit = %raw(`(message, number1, number2) => expect(number1, message).to.equal(number2)`)

let bnWithin: (
  Ethers.BigNumber.t,
  ~min: Ethers.BigNumber.t,
  ~max: Ethers.BigNumber.t,
) => unit = %raw(`(number1, min, max) => expect(number1).to.be.within(min, max)`)

let bnCloseTo: (
  ~message: string=?,
  ~distance: int,
  Ethers.BigNumber.t,
  Ethers.BigNumber.t,
) => unit = %raw(`(message, distance, number1, number2) => expect(number1, message).to.be.closeTo(number2, distance)`)

type eventCheck
let callEmitEvents: (
  ~call: Promise.t<ContractHelpers.transaction>,
  ~contract: ContractHelpers.t,
  ~eventName: string,
) => eventCheck = %raw(`(call, contract, eventName) => expect(call).to.emit(contract, eventName)`)
@send external withArgs0: eventCheck => Promise.t<unit> = "withArgs"
@send external withArgs1: (eventCheck, 'a) => Promise.t<unit> = "withArgs"
@send external withArgs2: (eventCheck, 'a, 'b) => Promise.t<unit> = "withArgs"
@send external withArgs3: (eventCheck, 'a, 'b, 'c) => Promise.t<unit> = "withArgs"
@send external withArgs4: (eventCheck, 'a, 'b, 'c, 'd) => Promise.t<unit> = "withArgs"
@send external withArgs5: (eventCheck, 'a, 'b, 'c, 'd, 'e) => Promise.t<unit> = "withArgs"

@send external withArgs5Return: (eventCheck, 'a, 'b, 'c, 'd, 'e) => eventCheck = "withArgs"

@send external withArgs6: (eventCheck, 'a, 'b, 'c, 'd, 'e, 'f) => Promise.t<unit> = "withArgs"
@send external withArgs7: (eventCheck, 'a, 'b, 'c, 'd, 'e, 'f, 'g) => Promise.t<unit> = "withArgs"
@send
external withArgs8: (eventCheck, 'a, 'b, 'c, 'd, 'e, 'f, 'g, 'h) => Promise.t<unit> = "withArgs"

@send
external withArgs9: (eventCheck, 'a, 'b, 'c, 'd, 'e, 'f, 'g, 'h, 'i) => Promise.t<unit> = "withArgs"

let expectToNotEmit: eventCheck => Promise.t<unit> = %raw(`eventCheck => {  let shouldRevert = true;
  return (eventCheck
    .catch(() =>
      shouldRevert = false
    ))
    .then(() => { if (shouldRevert) { require("chai").assert.fail('An event was emitted when it should not have been') } }
    );
}`)

let expectRevertNoReason: (
  ~transaction: Promise.t<ContractHelpers.transaction>,
) => Promise.t<unit> = %raw(`(transaction) => expect(transaction).to.be.reverted`)
let expectRevert: (
  ~transaction: Promise.t<ContractHelpers.transaction>,
  ~reason: string,
) => Promise.t<
  unit,
> = %raw(`(transaction, reason) => expect(transaction).to.be.revertedWith(reason)`)

let changeBalance: (
  ~transaction: unit => Promise.t<ContractHelpers.transaction>,
  ~token: ContractHelpers.t,
  ~to_: Ethers.ethAddress,
  ~amount: Ethers.BigNumber.t,
) => Promise.t<
  unit,
> = %raw(`(transaction, token, to, amount) => expect(transaction).to.changeTokenBalance(token, to, amount)`)
// TODO: implement changeBalanceMulti to test transactions that change the balance of multiple accounts
// let changeBalanceMulti = %raw(`expect(transaction).to.changeTokenBalance(token, wallets, amounts)`)

let expectToBeAddress: (
  ~address: Ethers.ethAddress,
) => Promise.t<unit> = %raw(`(address) => expect(address).to.be.properAddress`)
let expectToBePrivateKey: (
  ~privateKey: string,
) => Promise.t<unit> = %raw(`(privateKey) => expect(privateKey).to.be.properAddress`)
let expectToBeHex: (
  ~hexStr: string,
  ~length: int,
) => Promise.t<unit> = %raw(`(hexStr, hexLength) => expect(hexStr).to.be.properHex(hexLength)`)
let expectHexEqual: (
  ~hex1: string,
  ~hex2: string,
) => Promise.t<unit> = %raw(`(hex1, hex2) => expect(hex1).to.be.hexEqual(hex2)`)

let expectTrue: bool => unit = %raw(`(value) => expect(value).to.be.true`)
