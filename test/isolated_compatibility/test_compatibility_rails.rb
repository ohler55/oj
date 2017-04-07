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
require 'data_compatibility_rails.rb'

require 'oj'

if Rails::VERSION::MAJOR == 5 && RUBY_VERSION == "2.2.3"
  # see https://github.com/ohler55/oj/commit/050b4c70836394cffd96b63388ff0dedb8ed3558
  #require 'oj/active_support_helper'
end

Oj.mimic_JSON()
#Oj.add_to_json()

OJ_RAILS_COMPAT = {
  mode: :compat,
  use_as_json: false,
  float_precision: 16,
  bigdecimal_as_decimal: false,
  time_format: :xmlschema,
  second_precision: 3,
  escape_mode: :unicode_xss,
  nan: :null,
  nilnil: false
}.freeze

class CompatibilityRails < Minitest::Test
  def setup
    @default_options = Oj.default_options
  end

  def teardown
    Oj.default_options = @default_options
  end

  def test_compat_dump
    Oj.default_options = OJ_RAILS_COMPAT
    RAILS_TEST_DATA.each do |key, value|
      rails_value = begin
                      ActiveSupport::JSON.encode(value)
                    rescue Exception => e
                      e.class
                    end
      puts "*** #{key} #{value}\n  #{rails_value}"
      oj_value = begin
                   Oj.encode(value)
                 rescue Exception => e
                   e.class
                 end
      assert_equal(rails_value, oj_value, key.to_s)
    end
  end
end 
