postgres++
==========

postgres++ is a thin libpq wrapper that aims to make libpq easier to use.

It is designed to be exteremely simple and thin. It has the following basic but time saving features:

### # _Connection Pooling_
It has an internal connection pool with minimum and maximum connection limits and unused connection killer.

### # _Asynchronous_
It provides asynchronous database operations using Boost.ASIO under the hood.

### # _Safer & Easier_
It uses the power of C++11 variadic templates in conjunction with PQsendQueryParam. It is both extremely easier and safer to use the parametered send query with variadic templates.

### # _Cleaner Code_
It tries to take advantage of RAII where possible. It is more difficult to suffer from a memory leak and naturally leads to easier and cleaner code.

### _More Information_
The project uses CMake, builds and installs as a "Release" static library by default.

There are tags but **_master_** branch is also safe to use.
