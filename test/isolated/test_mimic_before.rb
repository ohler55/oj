# encoding: UTF-8

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
