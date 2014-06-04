#!/bin/sh

echo "----- General tests (test_various.rb) -----"
ruby -Itest test/test_various.rb
echo "----- Strict parser tests (test_strict.rb) -----"
ruby -Itest test/test_strict.rb
echo "----- Compat parser tests (test_compat.rb) -----"
ruby -Itest test/test_compat.rb
echo "----- Object parser tests (test_object.rb) -----"
ruby -Itest test/test_object.rb
echo "----- Mimic tests (isolated/test_mimic.rb) -----"
ruby -Itest isolated/test_mimic.rb
echo "----- Fast tests (test_fast.rb) -----"
ruby -Itest test/test_fast.rb
echo "----- SAJ parser tests (test_saj.rb) -----"
ruby -Itest test/test_saj.rb
echo "----- SC Parser tests (test_scp.rb) -----"
ruby -Itest test/test_scp.rb
echo "----- GC tests (test_gc.rb) -----"
ruby -Itest test/test_gc.rb
echo "----- Writer tests (test_writer.rb) -----"
ruby -Itest test/test_writer.rb
echo "----- File loading tests (test_file.rb) -----"
ruby -Itest test/test_file.rb
