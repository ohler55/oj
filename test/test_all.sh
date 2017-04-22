#!/bin/sh

echo "----- General tests (tests.rb) -----"
ruby tests.rb

echo "----- Mimic tests (tests_mimic.rb) -----"
ruby tests_mimic.rb

echo "----- Mimic with additions tests (tests_mimic_addition.rb) -----"
ruby tests_mimic_addition.rb

#ruby test_various.rb
#echo "----- Strict parser tests (test_strict.rb) -----"
#ruby test_strict.rb
#echo "----- Compat parser tests (test_compat.rb) -----"
#ruby test_compat.rb
#echo "----- Object parser tests (test_object.rb) -----"
#ruby test_object.rb
#echo "----- Fast tests (test_fast.rb) -----"
#ruby test_fast.rb
#echo "----- SAJ parser tests (test_saj.rb) -----"
#ruby test_saj.rb
#echo "----- SC Parser tests (test_scp.rb) -----"
#ruby test_scp.rb
#echo "----- GC tests (test_gc.rb) -----"
#ruby test_gc.rb
#echo "----- Writer tests (test_writer.rb) -----"
#ruby test_writer.rb
#echo "----- File loading tests (test_file.rb) -----"
#ruby test_file.rb
#echo "----- Hash loading tests (test_hash.rb) -----"
#ruby test_hash.rb
## only run if <= 1.9.3
#echo "----- Mimic tests (isolated/test_mimic_after.rb) -----"
#ruby isolated/test_mimic_after.rb
#echo "----- Mimic tests (isolated/test_mimic_alone.rb) -----"
#ruby isolated/test_mimic_alone.rb
#echo "----- Mimic tests (isolated/test_mimic_before.rb) -----"
#ruby isolated/test_mimic_before.rb
#echo "----- Mimic tests (isolated/test_mimic_define.rb) -----"
#ruby isolated/test_mimic_define.rb
#echo "----- Mimic tests (isolated/test_mimic_redefine.rb) -----"
#ruby isolated/test_mimic_redefine.rb
#echo "----- Mimic tests (isolated/test_mimic_rails_after.rb) -----"
#ruby isolated/test_mimic_rails_after.rb
#echo "----- Mimic tests (isolated/test_mimic_rails_before.rb) -----"
#ruby isolated/test_mimic_rails_before.rb
#echo "----- Mimic tests (isolated/test_mimic_as_json.rb) -----"
#ruby isolated/test_mimic_as_json.rb
