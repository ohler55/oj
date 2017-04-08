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

DEBUG = !!ENV['DEBUG']

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

#$encoder = Oj::Rails::Encoder.new()
#ActiveSupport.json_encoder = Oj::Rails::Encoder

class CompatibilityRails < Minitest::Test
  def setup
    @default_options = Oj.default_options
    Oj::Rails.optimize(Foo, Time, Range, Regexp)
  end

  def teardown
    Oj.default_options = @default_options
  end

  def test_compat_dump
    Oj.default_options = OJ_RAILS_COMPAT
    RAILS_TEST_DATA.each do |key, value|
      puts "Checking #{key} #{value}" if DEBUG
      rails_value = begin
                      ActiveSupport::JSON.encode(value)
                    rescue Exception => e
                      puts "  Rails Error #{e.class}: #{e.message}"  if DEBUG
                      e.class
                    end
      puts "  Rails: #{rails_value}"  if DEBUG
      oj_value = begin
                   Oj::Rails.encode(value)
                   #$encoder.encode(value)
                 rescue Exception => e
                   puts "     Oj Error #{e.class}: #{e.message}"  if DEBUG
                   e.class
                 end
      puts "     Oj: #{rails_value}"  if DEBUG
      assert_equal(rails_value, oj_value, key.to_s)
    end
  end
end 
