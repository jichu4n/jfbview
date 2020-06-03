#!/bin/bash

cd "$(dirname "$0")/../tests/testdata"

echo 'Checking binaries...'
for executable in jfbview jpdfgrep jpdfcat; do
  executable_path=$(which $executable)
  if [ $? -eq 0 ]; then
    echo "Found $executable: $executable_path"
  else
    echo "$executable not found in PATH!"
    exit 1
  fi
done
jfbview_path="$(which jfbview)"
(set -x; ldd "$jfbview_path")

echo
echo 'Checking jfbview help message...'
if ! jfbview -h 2>&1 | grep -q 'Options:'; then
  echo 'Invalid jfbview help message'
  exit 1
fi

echo
echo 'Checking jpdfgrep help message...'
if ! jpdfgrep -h 2>&1 | grep -q 'Options:'; then
  echo 'Invalid jpdfgrep help message'
  exit 1
fi

echo 'Checking jpdfgrep with test PDF file...'
num_search_results=$(jpdfgrep --width=80 ./bash.pdf 'HISTIGNORE' | wc -l)
if [ $? -ne 0 ] || [ $num_search_results -ne 8 ]; then
  echo 'Invalid output from jpdfgrep'
  exit 1
fi

echo 'Checking jpdfgrep with password-protected PDF file...'
num_search_results=$(jpdfgrep --width=80 -P abracadabra ./password-test.pdf 'SUCCESS' | wc -l)
if [ $? -ne 0 ] || [ $num_search_results -ne 1 ]; then
  echo 'Invalid output from jpdfgrep'
  exit 1
fi

echo
echo 'Checking jpdfcat help message...'
if ! jpdfcat -h 2>&1 | grep -q 'Options:'; then
  echo 'Invalid jpdfcat help message'
  exit 1
fi

echo 'Checking jpdfcat with test PDF file...'
num_search_results=$(jpdfcat ./bash.pdf | grep -o 'HISTIGNORE' | wc -l)
if [ $? -ne 0 ] || [ $num_search_results -ne 8 ]; then
  echo 'Invalid output from jpdfcat'
  exit 1
fi

echo 'Checking jpdfcat with page arguments...'
num_search_results=$(jpdfcat ./bash.pdf 86 131 | grep -o 'HISTIGNORE' | wc -l)
if [ $? -ne 0 ] || [ $num_search_results -ne 4 ]; then
  echo 'Invalid output from jpdfcat'
  exit 1
fi

echo 'Checking jpdfcat with password-protected PDF file...'
num_search_results=$(jpdfcat -P abracadabra ./password-test.pdf | grep 'SUCCESS' | wc -l)
if [ $? -ne 0 ] || [ $num_search_results -ne 1 ]; then
  echo 'Invalid output from jpdfcat'
  exit 1
fi

