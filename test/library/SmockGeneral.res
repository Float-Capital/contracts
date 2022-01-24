%%raw(`
require("chai").use(require('@defi-wonderland/smock').smock.matchers);
`)

type expectation
@module("chai")
external expect: 'a => expectation = "expect"

@send @scope(("to", "have"))
external toHaveCallCount: (expectation, int) => unit = "callCount"

@module("@defi-wonderland/smock") @scope("smock")
external getMockContractFactory: string => Promise.t<'a> = "mock"

@send
external setVariableRaw: ('smockedContract, ~name: string, ~value: 'a) => Promise.t<unit> =
  "setVariable"

@send
external reset: 'smartContractFunction => unit = "reset"

// An abstract type for structs, can use "Obj.magic" with it for now.
type abstractStructType
