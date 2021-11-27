## General

## Testing

the current javascript test are going to be refactored into rescript a some point

## Commands

format the solidity code:

```bash
yarn format-contracts
```

lint the solidity code:

```bash
yarn lint-contracts
```

## Network forking

It is sometimes useful to fork the network to test deployments that rely on other contracts etc before running those deployments on production networks. Note, network forking has only been tested with alchemy RPC API endpoints, but others may work too.

To use this feature run `HARDHAT_FORK=<network name> yarn deploy`. So for example to run this on mumbai run `HARDHAT_FORK="mumbai" yarn deploy`.

You can test that this is working correctly by validating some data from the blockchain such as the blocknumber or a token balance.

eg you could use code like below.:

```javascript
let pTokenBalance = await paymentToken.balanceOf(accounts[2].address);
console.log(
  "The paymentToken balance is",
  accounts[2].address,
  pTokenBalance.toString()
);

let blockNumber = await accounts[0].provider.getBlockNumber();
console.log("The balance is", blockNumber.toString());
```

## Verifying contracts

[Hardhat verify](https://hardhat.org/plugins/nomiclabs-hardhat-etherscan.html)
`npx hardhat verify --network mumbai CONTRACT_ADDRESS_HERE`

`npx hardhat verify --network <network name> CONTRACT_ADDRESS_HERE <constructor args strings seperate by spaces>`

> Where contract address is implementation address & upgradeable contracts take no constructor args
> To verify all deployed contracts use (this only works with etherscan):

`yarn hardhat --network <network name> etherscan-verify --api-key <your etherscan api key> --force-license --license UNLICENSED`

For some more obscure networks (like avalanche c-chain) that don't have etherscan you can drag and drop the generated json deployment artifacts from hardhat-deploy into their web-portal.

An alternative UI that can be used for contract verification: https://sourcify.dev/

## Troubleshooting

Please add your known troubles ;)
