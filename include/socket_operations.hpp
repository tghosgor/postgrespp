#pragma once

#include "result.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>

#include <stdexcept>

namespace postgrespp {

class result;

template <class DerivedT>
class socket_operations {
public:
  using result_t = result;

private:
  using error_code_t = boost::system::error_code;

protected:
  using derived_t = DerivedT;

protected:
  socket_operations() = default;

  ~socket_operations() = default;

  template <class ResultCallableT>
  void handle_exec(ResultCallableT&& handler) {
    auto wrapped_handler = [handler = std::move(handler), r = std::make_shared<result>(nullptr)](auto&& res) mutable {
      if (!res.done()) {
        if (!r->done()) throw std::runtime_error{"expected one result"};
        *r = std::move(res);
      } else {
        handler(std::move(*r));
      }
    };

    wait_read_ready(std::move(wrapped_handler));
    wait_write_ready();
  }

  template <class ResultCallableT>
  void handle_exec_all(ResultCallableT&& handler) {
    wait_read_ready(std::forward<ResultCallableT>(handler));
    wait_write_ready();
  }

private:
  template <class ResultCallableT>
  void wait_read_ready(ResultCallableT&& handler) {
    namespace ph = std::placeholders;

    derived().socket().async_read_some(boost::asio::null_buffers(),
        [this, handler = std::move(handler)](auto&& ec, auto&& bt) mutable {
          on_read_ready(std::move(handler), ec); });
  }

  void wait_write_ready() {
    namespace ph = std::placeholders;

    derived().socket().async_write_some(boost::asio::null_buffers(),
        std::bind(&socket_operations::on_write_ready, this, ph::_1));
  }

  template <class ResultCallableT>
  void on_read_ready(ResultCallableT&& handler, const error_code_t& ec) {
    if (!ec) {
      while (true) {
        if (PQconsumeInput(derived().connection().underlying_handle()) != 1) {
          throw std::runtime_error{"consume input failed"};
        }

        const auto flush = PQflush(derived().connection().underlying_handle());
        if (flush == 1) {
          wait_read_ready(std::forward<ResultCallableT>(handler));
          break;
        } else if (flush == 0) {
          if (PQisBusy(derived().connection().underlying_handle()) == 1) {
            wait_read_ready(std::forward<ResultCallableT>(handler));
            break;
          } else {
            const auto pqres = PQgetResult(derived().connection().underlying_handle());

            result res{pqres};
            handler(std::move(res));

            if (!pqres) {
              break;
            }
          }
        } else {
          throw std::runtime_error{"flush failed"};
        }
      }
    }
  }

  void on_write_ready(const error_code_t& ec) {
    const auto ret = PQflush(derived().connection().underlying_handle());
    if (ret == 1) {
      wait_write_ready();
    }
  }

  derived_t& derived() { return *static_cast<derived_t*>(this); }
};

}
