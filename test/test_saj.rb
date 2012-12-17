#!/usr/bin/env ruby
# encoding: UTF-8

# Ubuntu does not accept arguments to ruby when called using env. To get warnings to show up the -w options is
# required. That can be set in the RUBYOPT environment variable.
# export RUBYOPT=-w

$VERBOSE = true

$: << File.join(File.dirname(__FILE__), "../lib")
$: << File.join(File.dirname(__FILE__), "../ext")

require 'test/unit'
require 'oj'
require 'pp'

$json = %{{
  "array": [
    {
      "num"   : 3,
      "string": "message",
      "hash"  : {
        "h2"  : {
          "a" : [ 1, 2, 3 ]
        }
      }
    }
  ],
  "boolean" : true
}}

class AllSaj < Oj::Saj
  attr_accessor :calls

  def initialize()
    @calls = []
  end

  def hash_start(key)
    @calls << [:hash_start, key]
  end

  def hash_end(key)
    @calls << [:hash_end, key]
  end

  def array_start(key)
    @calls << [:array_start, key]
  end

  def array_end(key)
    @calls << [:array_end, key]
  end

  def add_value(value, key)
    @calls << [:add_value, value, key]
  end

end # AllSaj

class SajTest < ::Test::Unit::TestCase
  def test_nil
    handler = AllSaj.new()
    json = %{null}
    Oj.saj_parse(handler, json)
    assert_equal(1, handler.calls.size)
    assert_equal(:add_value, handler.calls[0][0])
    assert_equal(nil, handler.calls[0][1])
  end

  # TBD other tests

  def test_full
    handler = AllSaj.new()
    Oj.saj_parse(handler, $json)
    pp handler.calls
  end
end


