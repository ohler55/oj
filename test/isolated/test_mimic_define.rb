#!/usr/bin/env ruby
$: << File.join(File.dirname(__FILE__), '..')

require 'helper'

class MimicDefine < Minitest::Test
  def test_mimic_define
    assert_nil(defined?(JSON))
    Oj.mimic_JSON

    # Test constants
    assert(!defined?(JSON).nil?)
    assert(!defined?(JSON::ParserError).nil?)
    assert(Object.respond_to?(:to_json))

    # Test loaded features
    assert(!require('json'))

    begin
      require('json_spec')
      assert(false, '** should raise LoadError')
    rescue LoadError
      assert(true)
    end
  end
end # MimicSingle
