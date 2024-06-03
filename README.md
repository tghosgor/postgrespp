# postgres++

postgres++ is an asynchronous c++ libpq wrapper that aims to make libpq easier
to use.

postgres++ makes use of boost.asio for async operations and c++11 variadic
templates for functions like `PQsendQueryParams` for ease of use.
It is designed to be exteremely simple and thin. Also, it tries to follow
the libpqxx interface _very_ loosely.

## Safer & Easier

It takes advantage of c++11 variadic templates in conjunction with
PQsendQueryParam. This makes the library both easier and safer to use.

## Requirements

### c++17

The library currently requires the c++17 standard. It makes little use of c++17
features as of writing this so it should be relatively simple to port it to c++14
or c++11.

### libpq

The project is based on libpq.

### boost

Some of the boost libaries are used such as boost.asio and boost.endian. Their
usage is not heavy and I believe all of the boost can be replaced without much
effort. As an improvement, maybe socket ready signal interface can be abstracted
to allow users to plug their own versions via templates.

Through proper integration with boost.asio, the interface that this library provides can also be used with c++ coroutines just like any other boost.asio interface!

### cmake

Build system is based on CMake.

### gtest

Unit tests are based on gtest.

## Examples

```c++
using namespace postgrespp;

boost::asio::io_context ioc;

connection c{ioc, "host=127.0.0.1 user=postgres"};

// via callback
async_exec(c, "SELECT * FROM tbl_test", [](auto&& result) {
  assert(result.ok());
});

// use_future is also supported
auto result_future = async_exec(c, "SELECT * FROM tbl_test", use_future);

ioc.run();

assert(result_future.get().ok());
```

```c++
using namespace postgrespp;

boost::asio::io_context ioc;

connection c{ioc, "host=127.0.0.1 user=postgres"};

// via callback
async_exec(c,
  "INSERT INTO tbl_test (si, i, bi) VALUES ($1, $2, $3)",
  [](auto&& result) {
      assert(result.ok());
  },
  static_cast<std::int16_t>(1), 2, static_cast<std::int64_t>(3));

// use_future is also supported
auto result_future = async_exec(c,
  "INSERT INTO tbl_test (si, i, bi) VALUES ($1, $2, $3)",
  use_future,
  static_cast<std::int16_t>(1), 2, static_cast<std::int64_t>(3));

ioc.run();

assert(result_future.get().ok());
```

```c++
using namespace postgrespp;

boost::asio::io_context ioc;

connection c{ioc, "host=127.0.0.1 user=postgres"};

c.async_transaction<>([](auto txn) {
  // move transaction into a std::shared_ptr to keep it alive in lambdas.
  auto shared_txn = std::make_shared<work>(std::move(txn));

  shared_txn->async_exec(
    "SELECT * FROM tbl_test WHERE id = $1",
    [shared_txn](auto&& result) mutable {
      assert(result.ok());

      shared_txn->commit([shared_txn](auto&& commit_result) mutable {
        assert(commit_result.ok());

        std::cout << "We are done." << std::endl;
      });
    },
    1);
});

ioc.run();
```

More usage can be seen in [test/connection_test.cpp](test/connection_test.cpp)
and other tests.
