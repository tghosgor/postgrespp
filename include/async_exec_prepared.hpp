#pragma once

#include "work.hpp"
#include "statement_name.hpp"

#include <memory>
#include <utility>

namespace postgrespp {

/**
 * Asynchronously executes a prepared query.
 */
template <class RWT, class IsolationT, class TransactionHandlerT, class... Params>
void async_exec_prepared(basic_transaction<RWT, IsolationT>& t, const statement_name& name,
    TransactionHandlerT&& handler, Params&&... params) {
  t.async_exec_prepared(name, std::move(handler), std::forward<Params>(params)...);
}

/**
 * Starts a transaction, asynchronously executes a prepared query and commits
 * the transaction.
 */
template <class ResultCallableT, class... Params>
void async_exec_prepared(basic_connection& c, statement_name name,
    ResultCallableT&& handler, Params... params) {
  c.template async_transaction<>([handler = std::move(handler), name = std::move(name),
      params...](auto txn) mutable {
        auto s_txn = std::make_shared<work>(std::move(txn));

        auto wrapped_handler = [handler = std::move(handler), s_txn](auto&& result) mutable {
            if (result.ok()) {
              s_txn->commit([s_txn, handler = std::move(handler), result = std::move(result)]
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

        async_exec_prepared(*s_txn, name, std::move(wrapped_handler),
            std::move(params)...);
      });
}

}
