postgres++
==========

postgres++ is a thin libpq wrapper that aims to make libpq easier to use.

It has the following basic but time saving features:

### # _Asynchronous_
It provides asynchronous database operations using Boost.ASIO under the hood.

### # _Cleaner Code_
It tries to take advantage of RAII where possible. It is more difficult to suffer from a memory leak and naturally leads to easier and cleaner code.
