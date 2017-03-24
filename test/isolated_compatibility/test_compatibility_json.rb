#!/usr/bin/env ruby
# encoding: UTF-8

$: << File.dirname(__FILE__)
$oj_dir = File.dirname(File.dirname(File.expand_path(File.dirname(__FILE__))))
%w(lib ext).each do |dir|
  $: << File.join($oj_dir, dir)
end

require 'minitest'
require 'minitest/autorun'
require 'date'
require 'bigdecimal'
require 'oj'
require 'json'
require 'data_compatibility_json.rb'

OJ_JSON_COMPAT = { mode: :compat }.freeze

class CompatibilityJson < Minitest::Test
  def setup
    @default_options = Oj.default_options
  end

  def teardown
    Oj.default_options = @default_options
  end

  def test_rails_not_loaded
    assert(!defined?(ActiveRecord))
  end

  # Compares the results of to_json and Oj.dump. If an exception is raised the
  # exception class is compared but not the exception message.
  TEST_DATA_TO_JSON.each do |key, value|
    define_method "test_to_#{key}" do 
      Oj.default_options = OJ_JSON_COMPAT
      json_value = begin
                     value.to_json
                   rescue Exception => e
                     e
                   end
      oj_value = begin
                   Oj.to_json(value)
                 rescue Exception => e
                   e
                 end
      if json_value.is_a?(Exception)
        assert_equal(json_value.class, oj_value.class, key.to_s)
      else
        assert_equal(json_value, oj_value, key.to_s)
      end
    end
  end

  TEST_DATA_GENERATE.each do |key, value|
    define_method "test_gen_#{key}" do 
      Oj.default_options = OJ_JSON_COMPAT
      json_value = begin
                     JSON.generate(*value)
                   rescue Exception => e
                     e
                   end
      oj_value = begin
                   Oj.to_json(*value)
                 rescue Exception => e
                   e
                 end
      if json_value.is_a?(Exception)
        assert_equal(json_value.class, oj_value.class, key.to_s)
      else
        assert_equal(json_value, oj_value, key.to_s)
      end
    end
  end

end 
