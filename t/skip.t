#!/bin/bash

. "$(dirname $0)/tap.sh"

plan 2
skip "Passing Skip" "pass"
not_ok "Failing Skip #SKIP fail"


# vim:ts=4:sw=4:syntax=sh
