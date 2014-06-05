# encoding: UTF-8

require 'helper'

class MimicSingle < Minitest::Test

  def test_mimic_single
    assert(defined?(JSON).nil?)
    Oj.mimic_JSON
    assert(defined?(JSON))
    assert(defined?(JSON::ParserError))
  end

end # MimicSingle
