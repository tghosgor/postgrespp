# postgres++

postgres++ is an asynchronous c++ libpq wrapper that aims to make libpq easier
to use.

postgres++ makes use of boost.asio for async operations and c++11 variadic
templates for functions like `PQsendQueryParams` for ease of use.
It is designed to be exteremely simple and thin. Also, it tries to follow
the libpqxx interface _very_ loosely.

## Safer & Easier

It uses the power of c++11 variadic templates in conjunction with
PQsendQueryParam. This makes the library both easier and safer to use.

## Requirements

### c++17

The library currently requires c++17 standard. It makes little use of c++17
features as of writing this so it should be relatively simple to port it to c++14
or c++11.

### libpq

The project is based on libpq.

### boost

Some of the boost libaries are used such as boost.asio and boost.endian. Their
usage is not heavy and I believe all of the boost can be replaced without much
effort. As an improvement, maybe socket ready signal interface can be abstracted
to allow users to plug their own versions via templates.

### cmake

Build system is based on CMake.

### gtest

Unit tests are based on gtest.

## Examples

```c++
  boost::asio::io_context ioc;

  connection c{ioc, "host=127.0.0.1 user=postgres"};

  c.transaction([&](auto txn) {
    // move transaction into a std::shared_ptr to keep it alive in lambdas.
    auto shared_txn = std::make_shared<work>(std::move(txn));

    shared_txn->async_exec(
      "SELECT * FROM tbl_test WHERE id = $1",
      [shared_txn](auto&& result) {
        assert(result.ok());

        shared_txn->commit([shared_txn](auto&& commit_result) {
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
