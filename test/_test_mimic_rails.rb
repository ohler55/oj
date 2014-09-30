#!/usr/bin/env ruby
# encoding: UTF-8

$: << File.dirname(__FILE__)

require 'helper'
Oj.mimic_JSON
require 'rails/all'

class MimicRails < Minitest::Test

  def test_mimic_exception
    begin
      ActiveSupport::JSON.decode("{")
      puts "Failed"
    rescue ActiveSupport::JSON.parse_error
      assert(true)
    rescue Exception
      assert(false, 'Expected a JSON::ParserError')
    end
  end

  def test_dump_string
    Oj.default_options= {:indent => 2} # JSON this will not change anything
    json = ActiveSupport::JSON.encode([1, true, nil])
    assert_equal(%{[
  1,
  true,
  null
]
}, json)
  end

  def test_dump_rational
    Oj.default_options= {:indent => 2} # JSON this will not change anything
    json = ActiveSupport::JSON.encode([1, true, Rational(1)])
    assert_equal(%{[
  1,
  true,
  "1/1"
]
}, json)
  end

end # MimicRails
