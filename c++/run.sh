#!/bin/sh
clang++ -std=c++11 -O0 -g \
  -I/usr/local/Cellar/bdw-gc/7.4.2/include \
  -L/usr/local/Cellar/bdw-gc/7.4.2/lib \
  -lgc \
  repl.cpp reader.cpp types.cpp eval.cpp core.cpp -o repl || exit
for i in step2 step2_eval step3_env step4_if_fn_do step5_tco step6_file step7_quote step8_macros step9_try stepA_mal; do
  cp repl $i
done

#echo '(+ 2 2)' | ./repl

cd ..
make test^cpp^step2 test^cpp^step3 test^cpp^step4 test^cpp^step5 test^cpp^step6 test^cpp^step7 test^cpp^step8 test^cpp^step9
#make test^cpp^step9
