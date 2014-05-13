#!/bin/bash
set -e
DEFAULT_CLANG_LIB=libstdc++
clang_lib=${1:-$DEFAULT_CLANG_LIB}

echo "Building clang .so"
clang++ -std=c++11  -stdlib=$clang_lib ../../implementation/cppcomponents_concurrency.cpp -I ../../../cppcomponents -I ../../ -shared -o cppcomponents_concurrency.so -lboost_coroutine -fPIC -fvisibility=hidden -D__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1 -D__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2 -D__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4 -D__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8  -Werror -Wpedantic -pedantic-errors  -Wl,--no-undefined -ldl -pthread -lboost_system -lboost_context

echo "Building g++ executable"
g++ -std=c++11 -U__STRICT_ANSI__ ../unit_test.cpp  -I ../../../cppcomponents -I ../../ ./external/googletest-read-only/src/gtest_main.cc ./external/googletest-read-only/src/gtest-all.cc -I ./external/googletest-read-only -I ./external/googletest-read-only/include -o unit_test_exe -ldl -pthread  


echo "Running g++(exe) with clang++(so)"

./unit_test_exe --gtest_print_time=0

#rm *.so
rm unit_test_exe


echo "Building g++ .so"

g++ -std=c++11 ../../implementation/cppcomponents_concurrency.cpp -I ../../../cppcomponents -I ../../ -shared -o cppcomponents_concurrency.so -lboost_coroutine -fPIC -fvisibility=hidden  -lboost_system -lboost_context -Wl,--no-undefined

echo "Building clang executable"

clang++ -std=c++11 -D__STRICT_ANSI__ -stdlib=$clang_lib ../unit_test.cpp -I ../../../cppcomponents -I ../../  ./external/googletest-read-only/src/gtest_main.cc ./external/googletest-read-only/src/gtest-all.cc -I ./external/googletest-read-only -I ./external/googletest-read-only/include -o unit_test_exe -ldl -pthread -lsupc++ -D__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1 -D__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2 -D__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4 -D__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8  -Wpedantic -pedantic-errors -Werror

echo "Running clang++(exe) with g++(so)"

./unit_test_exe --gtest_print_time=0

rm *.so

rm unit_test_exe

