#!/bin/bash

echo "LINES: $(wc -l < "$1")"
echo "STATUS_COUNTS:"
cut -d' ' -f2 "$1" | sort | uniq -c | sort -nr | awk '{print $2, $1}'