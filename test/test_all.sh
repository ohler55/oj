#!/bin/sh

echo "----- General tests (tests.rb) -----"
ruby tests.rb

echo "----- Mimic tests (tests_mimic.rb) -----"
ruby tests_mimic.rb

echo "----- Mimic with additions tests (tests_mimic_addition.rb) -----"
ruby tests_mimic_addition.rb

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
