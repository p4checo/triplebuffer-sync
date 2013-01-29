triplebuffer-sync
=================

C++ Template class for a Triple Buffer as a concurrency mechanism, using C++11 atomic operations.

Ported from C code made by [remis-thoughts](https://github.com/remis-thoughts/blog/blob/master/triple-buffering/src/main/md/triple-buffering.md) on his [blog](http://remis-thoughts.blogspot.pt/2012/01/triple-buffering-as-concurrency_30.html)


####Compilation:

* use -std=c++11 or -std=gnu++11
* specify -march or equivalent for your architecture (for atomic implementation)
