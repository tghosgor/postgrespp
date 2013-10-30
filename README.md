postgrespp
==========

postgres++ is a thin libpq wrapper that aims to make libpq easier to use.

### Asynchronous
It provides asynchronous database operations using Boost.ASIO under the hood.

### Cleaner Usage
It tries to take advantage of RAII where possible. It is more difficult to suffer from a memory leak and naturally leads to easier and cleaner code.
