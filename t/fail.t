#!/bin/bash

echo "1..9"
echo ok 4
echo not ok 2
echo '  not ok 1'
echo ok 1
echo not ok 4
echo not ok
echo not ok
echo not ok 7
echo ok 8 '#skip'
echo not ok 9 '#todo'

# vim:ts=4:sw=4:syntax=sh
