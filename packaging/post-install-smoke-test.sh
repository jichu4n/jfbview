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

echo 'Checking jfbview help message...'
if ! jfbview -h 2>&1 | grep -q 'Options:'; then
  echo 'Invalid jfbview help message: '
  jfbview -h
  exit 1
fi

echo 'Checking jpdfgrep...'
num_search_results=$(jpdfgrep ./bash.pdf 'HISTIGNORE' | wc -l)
if [ $? -ne 0 ] || [ $num_search_results -ne 8 ]; then
  echo 'Invalid output from jpdfgrep:'
  jpdfgrep ./bash.pdf 'HISTIGNORE'
  exit 1
fi

echo 'Checking jpdfcat...'
num_search_results=$(jpdfcat ./bash.pdf | grep 'HISTIGNORE' | wc -l)
if [ $? -ne 0 ] || [ $num_search_results -ne 8 ]; then
  echo 'Invalid output from jpdfcat:'
  jpdfcat ./bash.pdf
  exit 1
fi

