module Mumbai = {
  let ethOracleChainlink =
    "0x0715A7794a1dc8e42615F059dD6e406A6594651A"->Ethers.Utils.getAddressUnsafe
  let maticOracleChainlink =
    "0xd0D5e3DB44DE05E9F294BB0a3bEEaF030DE24Ada"->Ethers.Utils.getAddressUnsafe
  let btcOracleChainlink =
    "0x007A22900a3B98143368Bd5906f8E17e9867581b"->Ethers.Utils.getAddressUnsafe
}
