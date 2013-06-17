#!/bin/sh

echo "----- General tests (tests.rb) -----"
./tests.rb
echo "----- Strict parser tests (test_strict.rb) -----"
./test_strict.rb
echo "----- Compat parser tests (test_compat.rb) -----"
./test_compat.rb
echo "----- Object parser tests (test_object.rb) -----"
./test_object.rb
echo "----- Mimic tests (test_mimic.rb) -----"
./test_mimic.rb
echo "----- Fast tests (test_fast.rb) -----"
./test_fast.rb
echo "----- SAJ parser tests (test_saj.rb) -----"
./test_saj.rb
echo "----- SC Parser tests (test_scp.rb) -----"
./test_scp.rb
