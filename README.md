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

## Troubleshooting

Please add your known troubles ;)
