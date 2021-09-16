%%raw(`
require("chai").use(require('@defi-wonderland/smock').smock.matchers);
`)

type expectation
@module("chai")
external expect: 'a => expectation = "expect"

@send @scope(("to", "have"))
external toHaveCallCount: (expectation, int) => unit = "callCount"

@module("@defi-wonderland/smock") @scope("smock")
external getMockContractFactory: string => JsPromise.t<'a> = "mock"

@send
external setVariableRaw: ('smockedContract, ~name: string, ~value: 'a) => JsPromise.t<unit> =
  "setVariable"

@send
external reset: 'smartContractFunction => unit = "reset"
