#!/bin/bash

. "$(dirname $0)/../tap.sh"

plan 9
echo 'ok    4'
ok 2
ok 1
ok 5 '# skip stuff'
echo 'ok 3#SKIp'
ok 6 '      #skip'
ok 7 '# skip'
ok 8 '# SKIP'
ok 9 'skip'


# vim:ts=4:sw=4:syntax=sh
