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

require 'data_compatibility_rails.rb'

OJ_RAILS_COMPAT = {
  mode: :compat,
  use_as_json: false,
  float_precision: 16,
  bigdecimal_as_decimal: false,
  time_format: :xmlschema,
  second_precision: 3,
  escape_mode: :unicode_xss,
  nan: :null,
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
      rails_value = ActiveSupport::JSON.encode(value)
      oj_value = begin
        Oj.dump(value)
      rescue NoMemoryError => e
        e
      rescue NotImplementedError => e
        e
      end
      assert_equal(rails_value, oj_value, key.to_s)
    end
  end
end 
