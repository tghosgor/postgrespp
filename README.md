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
The project uses GYP as its build system. It is easy to figure out and shell script for building will be added.

This project is licensed under:

[![GPLv3](https://raw.githubusercontent.com/metherealone/qrive/misc/gplv3-127x51.png)](http://www.gnu.org/licenses/gpl-3.0.html)

##P.S.
**My bitcoin address:** 13vw56mgsFj6g2AV6j9RgS4hmWfnoMgc68
