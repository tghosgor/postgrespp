#pragma once

#include "result.hpp"

#include <boost/asio/async_result.hpp>
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
  auto handle_exec(ResultCallableT&& handler) {
    auto initiation = [this](auto&& handler) {
      auto wrapped_handler = [handler = std::move(handler),
           r = std::make_shared<result>(nullptr)](auto&& res) mutable {
        if (!res.done()) {
          if (!r->done()) throw std::runtime_error{"expected one result"};
          *r = std::move(res);
        } else {
          handler(std::move(*r));
        }
      };

      wait_read_ready(std::move(wrapped_handler));
      wait_write_ready();
    };

    return boost::asio::async_initiate<
      ResultCallableT, void(result_t)>(
          initiation, handler);
  }

  template <class ResultCallableT>
  auto handle_exec_all(ResultCallableT&& handler) {
    auto initiation = [this](auto&& handler) {
      wait_read_ready(std::forward<ResultCallableT>(handler));
      wait_write_ready();
    };

    return boost::asio::async_initiate<
      ResultCallableT, void(result_t)>(
          initiation, handler);
  }

private:
  template <class ResultCallableT>
  void wait_read_ready(ResultCallableT&& handler) {
    namespace ph = std::placeholders;

    derived().socket().async_wait(std::decay_t<decltype(derived().socket())>::wait_read,
        [this, handler = std::move(handler)](auto&& ec) mutable {
          on_read_ready(std::move(handler), ec); });
  }

  void wait_write_ready() {
    namespace ph = std::placeholders;

    derived().socket().async_wait(std::decay_t<decltype(derived().socket())>::wait_write,
        std::bind(&socket_operations::on_write_ready, this, ph::_1));
  }

  template <class ResultCallableT>
  void on_read_ready(ResultCallableT&& handler, const error_code_t& ec) {
    while (true) {
      if (PQconsumeInput(derived().connection().underlying_handle()) != 1) {
        // TODO: convert this to some kind of error via the callback
        throw std::runtime_error{
          "consume input failed: " + std::string{derived().connection().last_error_message()}};
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
        // TODO: convert this to some kind of error via the callback
        throw std::runtime_error{
          "flush failed: " + std::string{derived().connection().last_error_message()}};
      }
    }
  }

  void on_write_ready(const error_code_t& ec) {
    const auto ret = PQflush(derived().connection().underlying_handle());
    if (ret == 1) {
      wait_write_ready();
    } else if (ret != 0) {
      // TODO: ignore or convert this to some kind of error via the callback
      throw std::runtime_error{
        "flush failed: " + std::string{derived().connection().last_error_message()}};
    }
  }

  derived_t& derived() { return *static_cast<derived_t*>(this); }
};

}
