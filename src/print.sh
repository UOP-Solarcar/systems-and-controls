#! /usr/bin/env sh

if [-z "$1"]; then
  echo "usage $0 filename"
  exit 1
fi
while IFS= read -r line; do
  echo $line
done < "$1"
