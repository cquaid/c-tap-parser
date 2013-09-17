#!/bin/bash

# required for pragma support
echo "TAP version 13"

# Have to have at least a test.
echo "1..1"

echo "pragma +test"
# expect "test_pragma: 1"
echo "pragma +print_test"
echo "pragma -test"
# nothing should print here
echo "pragma -print_test"

# for giggs make sure it doesn't parse tests
echo "ok 1 pragma +print_test"

# expected output:
# strict: 0
# strict: 1
echo "pragma -strict,+print_strict,+strict,+print_strict"

# test_pragma: 0
echo "pragma +print_test"

# Nothing / parse error
echo "pragma print_test"
# parse error
echo "pragma -test,+test,"
# test_pragma: 1
echo "pragma +print_test"
# parse error
echo "pragma -test,test"
# test_pragma: 0
echo "pragma +print_test"

# parse_errors: 3
echo "pragma +print_parse_errors"

# vim:ts=4:sw=4:syntax=sh
