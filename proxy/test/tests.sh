#!/bin/bash
echo > test_log.txt
for ((i = 1; i < 301 ; i++)) do
  ./../bin/test /bin/bash $i
  DIFF=$(diff output.txt /bin/bash)
  echo "TEST (child_num = $i) | DIFF: $DIFF" >> test_log.txt
done