#!/usr/bin/env ruby
$: << File.join(File.dirname(__FILE__), '..')

require 'helper'

class MimicDefine < Minitest::Test
  def test_mimic_define
    assert_nil(defined?(JSON))
    Oj.mimic_JSON

    # Test constants
    refute_nil(defined?(JSON))
    refute_nil(defined?(JSON::ParserError))
    assert(Object.respond_to?(:to_json))

    # Test loaded features
    refute(require('json'))

    begin
      require('json_spec')
      assert(false, '** should raise LoadError')
    rescue LoadError
      assert(true)
    end
  end
end # MimicSingle
