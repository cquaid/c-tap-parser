#!/bin/bash

function plan() {
	local upper=$1
	shift 1
	echo "1..$upper $*"
}

function ok() {
	echo "ok $*"
}

function not_ok() {
	echo "not ok $*"
}

function skip() {
	local desc=$1
	local reason=$2
	ok "$desc # skip $reason"
}

function todo() {
	local desc=$1
	local reason=$2
	not_ok "$desc #TODO $reason"
}

function version() {
	echo "TAP version $1"
}

function pragma() {
	echo "pragma $*"
}

function comment() {
	echo "# $*"
}

function bailout() {
	echo "Bail out! $*"
}
