#!/bin/bash

. "$(dirname $0)/../tap.sh"

ok foo
ok bar
plan 0 "# skip late skip all"

# vim:ts=4:sw=4:syntax=sh
