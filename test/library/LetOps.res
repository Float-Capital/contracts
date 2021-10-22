module AwaitThen = {
  let let_ = Promise.then
}
module Await = {
  let let_ = Promise.thenResolve
}
