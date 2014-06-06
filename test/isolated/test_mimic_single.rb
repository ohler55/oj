#!/usr/bin/env ruby
# encoding: UTF-8

$: << File.join(File.dirname(__FILE__), '..')

require 'helper'

class MimicSingle < Minitest::Test
  def test_mimic_single
    assert(defined?(JSON).nil?)
    Oj.mimic_JSON
    assert(!defined?(JSON).nil?)
    # Make sure to_json is define for object.
    {'a' => 1}.to_json()
    Object.new().to_json()
  end

end # MimicSingle
