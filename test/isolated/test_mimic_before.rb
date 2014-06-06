#!/usr/bin/env ruby
# encoding: UTF-8

$: << File.join(File.dirname(__FILE__), '..')

require 'helper'
Oj.mimic_JSON
require 'json'

class MimicBefore < Minitest::Test

  # correct exception
  def test_mimic_exception
    begin
      JSON.parse("{")
      puts "Failed"
    rescue JSON::ParserError
      assert(true)
    rescue Exception
      assert(false, 'Expected a JSON::ParserError')
    end
  end

  # Make sure to_json is define for object.
  def test_mimic_to_json
    {'a' => 1}.to_json()
    Object.new().to_json()
  end

  # dump
  def test_dump_string
    Oj.default_options= {:indent => 2} # JSON this will not change anything
    json = JSON.dump([1, true, nil])
    assert_equal(%{[
  1,
  true,
  null
]
}, json)
  end


end # MimicAfter
