#!/bin/sh

echo "----- General tests (test_various.rb) -----"
ruby test_various.rb
echo "----- Strict parser tests (test_strict.rb) -----"
ruby test_strict.rb
echo "----- Compat parser tests (test_compat.rb) -----"
ruby test_compat.rb
echo "----- Object parser tests (test_object.rb) -----"
ruby test_object.rb
echo "----- Fast tests (test_fast.rb) -----"
ruby test_fast.rb
echo "----- SAJ parser tests (test_saj.rb) -----"
ruby test_saj.rb
echo "----- SC Parser tests (test_scp.rb) -----"
ruby test_scp.rb
echo "----- GC tests (test_gc.rb) -----"
ruby test_gc.rb
echo "----- Writer tests (test_writer.rb) -----"
ruby test_writer.rb
echo "----- File loading tests (test_file.rb) -----"
ruby test_file.rb
echo "----- Hash loading tests (test_hash.rb) -----"
ruby test_hash.rb
