## Build FleCSPH on Marylou Supercomputer

This document explains how to build FleCSPH on marylou supercomputer. 
Marylou is operated under RHEL system. Specification about Marylou supercomputer 
can be found [here](https://marylou.byu.edu/documentation/resources).

If your system is similar as this, you can build FleCSPH in similar way


### Module List
Below is working module for latest flecsph. 
```
  1) gcc/7.3               4) git/2.14              7) boost/1.59.0
  2) gdb/8.1               5) python/3.6_gcc-7
  3) cmake/3.12(default)   6) openmpi/3.1_gcc-7
```
Once you have all these module, remaining procedure is exactly same as described in 
[README.md](https://github.com/laristra/flecsph/blob/master/README.md)

## Possible Errors

1. During configuration for `FleCSI` and `FleCSPH`, you may encounter error message about

  `Cannot find c++17 compatible complier` 

even you called correct module. This causes because
marylou can have different priority of complier i.e. use `c++` rather than `g++` for system
complier. 

To fix this, you simply change:
```
 CMAKE_CXX_COMPILER     g++
 CMAKE_C_COMPILER       gcc
```
Then cmake should find correct path of complier. If it still cannot find it you can specify 
the path:
```
 CMAKE_CXX_COMPILER     /apps/gcc/7.3.0/bin/g++
 CMAKE_C_COMPILER       /apps/gcc/7.3.0/bin/gcc
```
2. TODO : Some problem for id_generation about `noh_2d`

## Contact
If you have questions or find more problems, please contact Hyun Lim (hylim1988@gmail.com)
