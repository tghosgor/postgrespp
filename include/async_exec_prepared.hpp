#pragma once

#include "work.hpp"
#include "statement_name.hpp"

#include <boost/asio/async_result.hpp>

#include <memory>
#include <utility>

namespace postgrespp {

/**
 * Asynchronously executes a prepared query.
 * This function must not be called again before the handler is called.
 */
template <class RWT, class IsolationT, class ResultCallableT, class... Params>
auto async_exec_prepared(basic_transaction<RWT, IsolationT>& t, const statement_name& name,
    ResultCallableT&& handler, Params&&... params) {
  return t.async_exec_prepared(name, std::forward<ResultCallableT>(handler), std::forward<Params>(params)...);
}

/**
 * Starts a transaction, asynchronously executes a prepared query and commits
 * the transaction.
 * This function must not be called again before the handler is called.
 */
template <class ResultCallableT, class... Params>
auto async_exec_prepared(basic_connection& c, statement_name name,
    ResultCallableT&& handler, Params... params) {
  auto initiation = [](auto&& handler, basic_connection& c, statement_name name, auto&&... params) mutable {
   c.template async_transaction<>([
      handler = std::move(handler),
      name = std::move(name),
      params...](auto txn) mutable {
        auto ptxn = std::make_unique<work>(std::move(txn));
        auto& txn_ref = *ptxn;

        auto wrapped_handler = [handler = std::move(handler), ptxn = std::move(ptxn)](auto&& result) mutable {
            if (result.ok()) {
              auto& txn_ref = *ptxn;
              txn_ref.commit([ptxn = std::move(ptxn), handler = std::move(handler), result = std::move(result)]
                            (auto&& commit_result) mutable {
                  if (commit_result.ok()) {
                    handler(std::move(result));
                  } else {
                    handler(std::move(commit_result));
                  }
                });
            } else {
              handler(std::move(result));
            }
          };

        async_exec_prepared(txn_ref, name, std::move(wrapped_handler),
            std::move(params)...);
      });
  };

  return boost::asio::async_initiate<
    ResultCallableT, void(result)>(
        initiation, handler, std::ref(c), std::move(name),
        std::forward<decltype(params)>(params)...);
}

}
