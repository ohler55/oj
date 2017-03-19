#!/usr/bin/env ruby
# encoding: UTF-8

$: << File.dirname(__FILE__)

require 'helper'
require 'json'
require 'data_compatibility_json.rb'

OJ_JSON_COMPAT = {
  mode: :compat,
  use_as_json: false,
  float_precision: 16,
  bigdecimal_as_decimal: false,
  time_format: :xmlschema,
  second_precision: 3,
}.freeze

class CompatibilityJson < Minitest::Test
  def setup
    @default_options = Oj.default_options
  end

  def teardown
    Oj.default_options = @default_options
  end

  def test_compat_dump
    Oj.default_options = OJ_JSON_COMPAT
    JSON_TEST_DATA.each do |key, value|
      json_value = begin
        value.to_json
      rescue JSON::GeneratorError => e
        e
      end
      oj_value = begin
        Oj.dump(value)
      rescue NoMemoryError => e
        e
      rescue NotImplementedError => e
        e
      end
      assert_equal(json_value, oj_value, key.to_s)
    end
  end
end 
