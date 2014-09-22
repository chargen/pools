pools
=====

```
Jeff Koftinoff
jeffk@jdkoftinoff.com
```

Simple Back End for pool allocator in C.

Released under the Open Source BSD license

(See the LICENSE file for details)

Quick Start
===========

With command linux configure script (unix):

```
git clone https://github.com/jdkoftinoff/pools.git
cd pools
./configure --debug=1
make && make test
cat tmp-target/build/tests/pool_test.out
```

With cmake:

```
cmake -G "Unix Makefiles"
make pool_test
make test
```

or use the Xcode or Visual Studio generator with cmake


