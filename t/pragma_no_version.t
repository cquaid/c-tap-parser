#!/bin/bash

# Have to have at least a test.
echo "1..1"

# Nothing should happen here/unknown lines
# pragmas are a TAP 13 only feature
echo "pragma +test"
echo "pragma +print_test"

echo "ok no pragmas"

# vim:ts=4:sw=4:syntax=sh
