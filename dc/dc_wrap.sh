#!/bin/bash
(while read line; do
  echo "[${line}]"
 done) | stdbuf -i0 -o0 dc $1 -

