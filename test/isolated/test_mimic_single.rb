#!/usr/bin/env ruby
# encoding: UTF-8

$: << File.join(File.dirname(__FILE__), '..')

require 'helper'

class MimicSingle < Minitest::Test
  def test_mimic_single
    assert(defined?(JSON).nil?)
    Oj.mimic_JSON

    # Test constants
    assert(!defined?(JSON).nil?)
    assert(!defined?(JSON::ParserError).nil?)

    # Test loaded features
    assert(!require('json'))

    begin
      require('json_spec')
    rescue LoadError
      assert(true)
      return
    end
    assert(false, '** should raise LoadError')

    # Make sure to_json is define for object.
    {'a' => 1}.to_json()
    Object.new().to_json()
  end
end # MimicSingle
