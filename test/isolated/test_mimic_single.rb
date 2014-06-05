# encoding: UTF-8

require 'helper'

class MimicSingle < Minitest::Test

  def test_mimic_single
    assert(defined?(JSON).nil?)
    Oj.mimic_JSON
    assert(!defined?(JSON).nil?)
  end

end # MimicSingle
