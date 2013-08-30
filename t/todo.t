#!/bin/bash

echo 1..5
echo not ok#todo something
echo not ok 2 '#TODO'
echo ok '#ToDo fail'
echo not ok 4 'stuff  #          todo   stuff'
echo ok todo

# vim:ts=4:sw=4:syntax=sh
