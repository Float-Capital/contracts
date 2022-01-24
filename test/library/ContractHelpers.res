type contractFactory
type t

type bytes4
type bytes32
type tuple // used by some open-zeppelin contracts
type transaction = unit // TODO: make this better
type unknownType // Just for unclassified ethereum type stuff in generated code

@val @scope("ethers")
external getContractFactory: string => Promise.t<contractFactory> = "getContractFactory"
@send
external attachAtAddress: (contractFactory, ~contractAddress: Ethers.ethAddress) => Promise.t<t> =
  "attach"
@send external deploy: contractFactory => Promise.t<t> = "deploy"
@send external deploy1: (contractFactory, 'a) => Promise.t<t> = "deploy"
@send external deploy2: (contractFactory, 'a, 'b) => Promise.t<t> = "deploy"
@send external deploy3: (contractFactory, 'a, 'b, 'c) => Promise.t<t> = "deploy"
@send external deploy4: (contractFactory, 'a, 'b, 'c, 'd) => Promise.t<t> = "deploy"
@send external deploy5: (contractFactory, 'a, 'b, 'c, 'd, 'e) => Promise.t<t> = "deploy"
@send external deploy6: (contractFactory, 'a, 'b, 'c, 'd, 'e, 'f) => Promise.t<t> = "deploy"
@send external deploy7: (contractFactory, 'a, 'b, 'c, 'd, 'e, 'f, 'g) => Promise.t<t> = "deploy"
@send
external deploy8: (contractFactory, 'a, 'b, 'c, 'd, 'e, 'f, 'g, 'h) => Promise.t<t> = "deploy"

@send external deployed: t => Promise.t<unit> = "deployed"

let attachToContract = (contractName, ~contractAddress) => {
  getContractFactory(contractName)->Promise.then(attachAtAddress(~contractAddress))
}
let deployContract0 = contractName => {
  getContractFactory(contractName)->Promise.then(deploy)->Promise.then(deployed)
}
let deployContract1 = (contractName, firstParam) => {
  getContractFactory(contractName)->Promise.then(deploy1(_, firstParam))->Promise.then(deployed)
}
let deployContract2 = (contractName, firstParam, secondParam) => {
  getContractFactory(contractName)
  ->Promise.then(deploy2(_, firstParam, secondParam))
  ->Promise.then(deployed)
}
let deployContract3 = (contractName, firstParam, secondParam, thirdParam) => {
  getContractFactory(contractName)
  ->Promise.then(deploy3(_, firstParam, secondParam, thirdParam))
  ->Promise.then(deployed)
}
let deployContract4 = (contractName, firstParam, secondParam, thirdParam, fourthParam) => {
  getContractFactory(contractName)
  ->Promise.then(deploy4(_, firstParam, secondParam, thirdParam, fourthParam))
  ->Promise.then(deployed)
}
let deployContract5 = (
  contractName,
  firstParam,
  secondParam,
  thirdParam,
  fourthParam,
  fifthParam,
) => {
  getContractFactory(contractName)
  ->Promise.then(deploy5(_, firstParam, secondParam, thirdParam, fourthParam, fifthParam))
  ->Promise.then(deployed)
}
let deployContract6 = (
  contractName,
  firstParam,
  secondParam,
  thirdParam,
  fourthParam,
  fifthParam,
  sixthParam,
) => {
  getContractFactory(contractName)
  ->Promise.then(
    deploy6(_, firstParam, secondParam, thirdParam, fourthParam, fifthParam, sixthParam),
  )
  ->Promise.then(deployed)
}
let deployContract7 = (
  contractName,
  firstParam,
  secondParam,
  thirdParam,
  fourthParam,
  fifthParam,
  sixthParam,
  seventhParam,
) => {
  getContractFactory(contractName)
  ->Promise.then(
    deploy7(
      _,
      firstParam,
      secondParam,
      thirdParam,
      fourthParam,
      fifthParam,
      sixthParam,
      seventhParam,
    ),
  )
  ->Promise.then(deployed)
}

let deployContract8 = (
  contractName,
  firstParam,
  secondParam,
  thirdParam,
  fourthParam,
  fifthParam,
  sixthParam,
  seventhParam,
  eighthParam,
) => {
  getContractFactory(contractName)
  ->Promise.then(
    deploy8(
      _,
      firstParam,
      secondParam,
      thirdParam,
      fourthParam,
      fifthParam,
      sixthParam,
      seventhParam,
      eighthParam,
    ),
  )
  ->Promise.then(deployed)
}
let deployMockContract0 = contractName => {
  SmockGeneral.getMockContractFactory(contractName)->Promise.then(deploy)->Promise.then(deployed)
}
let deployMockContract1 = (contractName, firstParam) => {
  SmockGeneral.getMockContractFactory(contractName)
  ->Promise.then(deploy1(_, firstParam))
  ->Promise.then(deployed)
}
let deployMockContract2 = (contractName, firstParam, secondParam) => {
  SmockGeneral.getMockContractFactory(contractName)
  ->Promise.then(deploy2(_, firstParam, secondParam))
  ->Promise.then(deployed)
}
let deployMockContract3 = (contractName, firstParam, secondParam, thirdParam) => {
  SmockGeneral.getMockContractFactory(contractName)
  ->Promise.then(deploy3(_, firstParam, secondParam, thirdParam))
  ->Promise.then(deployed)
}
let deployMockContract4 = (contractName, firstParam, secondParam, thirdParam, fourthParam) => {
  SmockGeneral.getMockContractFactory(contractName)
  ->Promise.then(deploy4(_, firstParam, secondParam, thirdParam, fourthParam))
  ->Promise.then(deployed)
}
let deployMockContract5 = (
  contractName,
  firstParam,
  secondParam,
  thirdParam,
  fourthParam,
  fifthParam,
) => {
  SmockGeneral.getMockContractFactory(contractName)
  ->Promise.then(deploy5(_, firstParam, secondParam, thirdParam, fourthParam, fifthParam))
  ->Promise.then(deployed)
}
let deployMockContract6 = (
  contractName,
  firstParam,
  secondParam,
  thirdParam,
  fourthParam,
  fifthParam,
  sixthParam,
) => {
  SmockGeneral.getMockContractFactory(contractName)
  ->Promise.then(
    deploy6(_, firstParam, secondParam, thirdParam, fourthParam, fifthParam, sixthParam),
  )
  ->Promise.then(deployed)
}
let deployMockContract7 = (
  contractName,
  firstParam,
  secondParam,
  thirdParam,
  fourthParam,
  fifthParam,
  sixthParam,
  seventhParam,
) => {
  SmockGeneral.getMockContractFactory(contractName)
  ->Promise.then(
    deploy7(
      _,
      firstParam,
      secondParam,
      thirdParam,
      fourthParam,
      fifthParam,
      sixthParam,
      seventhParam,
    ),
  )
  ->Promise.then(deployed)
}

let deployMockContract8 = (
  contractName,
  firstParam,
  secondParam,
  thirdParam,
  fourthParam,
  fifthParam,
  sixthParam,
  seventhParam,
  eighthParam,
) => {
  SmockGeneral.getMockContractFactory(contractName)
  ->Promise.then(
    deploy8(
      _,
      firstParam,
      secondParam,
      thirdParam,
      fourthParam,
      fifthParam,
      sixthParam,
      seventhParam,
      eighthParam,
    ),
  )
  ->Promise.then(deployed)
}

@send external connect: ('contract, ~address: Ethers.Wallet.t) => 'contract = "connect"
