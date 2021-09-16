let coverageReportOutputDirectory = "coverage-all"

let extraFilesToIgnore = []

let isUnitTests =
  !!process.env.DONT_RUN_INTEGRATION_TESTS && process.env.DONT_RUN_INTEGRATION_TESTS.toUpperCase() == "TRUE"
let isIntegrationTests =
  !!process.env.DONT_RUN_UNIT_TESTS && process.env.DONT_RUN_UNIT_TESTS.toUpperCase() == "TRUE"
if (isUnitTests) {
  coverageReportOutputDirectory = "coverage-unit"
  extraFilesToIgnore = [
    "FloatToken.sol",
    "SyntheticToken.sol",
    "TokenFactory.sol",
    "Treasury_v0.sol",
  ]
} else if (isIntegrationTests) {
  coverageReportOutputDirectory = "coverage-integration"
} else if (!isUnitTests && !isIntegrationTests) { // if it is neither then it is both (wierd logic but it works)
  // Don't do anything in this case
} else {
  throw Error("Invalid config, don't set both 'DONT_RUN_INTEGRATION_TESTS' and 'DONT_RUN_UNIT_TESTS' to true")
}

module.exports = {
  skipFiles: [
    "FloatCapital_v0.sol",
    "Treasury_v0.sol",
    "AlphaTestFLT.sol",
    "StakingStrategy.sol",
    "StrategyToken.sol",
    "SyntheticTokenUpgradeable.sol",
    "TreasuryAlpha.sol",
    "YieldManagerAave.sol",

    "deployment/UUPSProxy.sol",

    "interfaces/IFloatToken.sol",
    "interfaces/ILongShort.sol",
    "interfaces/IOracleManager.sol",
    "interfaces/IStaker.sol",
    "interfaces/ISyntheticToken.sol",
    "interfaces/ITokenFactory.sol",
    "interfaces/IYieldManager.sol",
    "interfaces/aave/DataTypes.sol",
    "interfaces/aave/ILendingPool.sol",
    "interfaces/aave/ILendingPoolAddressesProvider.sol",

    "mocks/BandOracleMock.sol",
    "mocks/Dai.sol",
    "mocks/MockERC20.sol",
    "mocks/OracleManagerMock.sol",
    "mocks/YieldManagerMock.sol",
    "mocks/AggregatorV3Mock.sol",
    "mocks/ERC20Mock.sol",
    "mocks/LendingPoolAaveMock.sol",
    "mocks/LendingPoolAddressesProviderMock.sol",
    "mocks/LendingPoolAddressesProvider.sol",

    "oracles/OracleManagerChainlink.sol",
    "oracles/OracleManagerEthKillerChainlink.sol",
    "oracles/OracleManagerEthKillerChainlinkTestnet.sol",
    "oracles/OracleManagerEthVsBtc.sol",
    "oracles/OracleManagerFlippening_V0.sol",
    "oracles/OracleManagerChainlinkTestnet.sol",
    "oracles/OracleManagerFlipp3ning.sol",

    "testing/StakerInternalStateSetters.sol",
    "testing/LongShortInternalStateSetters.sol",

    "testing/generated/LongShortForInternalMocking.sol",
    "testing/generated/StakerForInternalMocking.sol",
    "testing/generated/LongShortMockable.sol",
    "testing/generated/StakerMockable.sol",
  ].concat(extraFilesToIgnore),
  istanbulFolder: coverageReportOutputDirectory,
  configureYulOptimizer: true
};

