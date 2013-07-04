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

class NoHandler < Oj::ScHandler
  def initialize()
  end
end

class AllHandler < Oj::ScHandler
  attr_accessor :calls

  def initialize()
    @calls = []
  end

  def hash_start()
    @calls << [:hash_start]
    {}
  end

  def hash_end()
    @calls << [:hash_end]
  end

  def array_start()
    @calls << [:array_start]
    []
  end

  def array_end()
    @calls << [:array_end]
  end

  def add_value(value)
    @calls << [:add_value, value]
  end

  def hash_set(h, key, value)
    @calls << [:hash_set, key, value]
  end
  
  def array_append(a, value)
    @calls << [:array_append, value]
  end

end # AllHandler

class ScpTest < ::Test::Unit::TestCase

  def test_nil
    handler = AllHandler.new()
    json = %{null}
    Oj.sc_parse(handler, json)
    assert_equal([[:add_value, nil]], handler.calls)
  end

  def test_true
    handler = AllHandler.new()
    json = %{true}
    Oj.sc_parse(handler, json)
    assert_equal([[:add_value, true]], handler.calls)
  end

  def test_false
    handler = AllHandler.new()
    json = %{false}
    Oj.sc_parse(handler, json)
    assert_equal([[:add_value, false]], handler.calls)
  end

  def test_string
    handler = AllHandler.new()
    json = %{"a string"}
    Oj.sc_parse(handler, json)
    assert_equal([[:add_value, 'a string']], handler.calls)
  end

  def test_fixnum
    handler = AllHandler.new()
    json = %{12345}
    Oj.sc_parse(handler, json)
    assert_equal([[:add_value, 12345]], handler.calls)
  end

  def test_float
    handler = AllHandler.new()
    json = %{12345.6789}
    Oj.sc_parse(handler, json)
    assert_equal([[:add_value, 12345.6789]], handler.calls)
  end

  def test_float_exp
    handler = AllHandler.new()
    json = %{12345.6789e7}
    Oj.sc_parse(handler, json)
    assert_equal(1, handler.calls.size)
    assert_equal(:add_value, handler.calls[0][0])
    assert_equal((12345.6789e7 * 10000).to_i, (handler.calls[0][1] * 10000).to_i)
  end

  def test_array_empty
    handler = AllHandler.new()
    json = %{[]}
    Oj.sc_parse(handler, json)
    assert_equal([[:array_start],
                  [:array_end],
                  [:add_value, []]], handler.calls)
  end

  def test_array
    handler = AllHandler.new()
    json = %{[true,false]}
    Oj.sc_parse(handler, json)
    assert_equal([[:array_start],
                  [:array_append, true],
                  [:array_append, false],
                  [:array_end],
                  [:add_value, []]], handler.calls)
  end

  def test_hash_empty
    handler = AllHandler.new()
    json = %{{}}
    Oj.sc_parse(handler, json)
    assert_equal([[:hash_start],
                  [:hash_end],
                  [:add_value, {}]], handler.calls)
  end

  def test_hash
    handler = AllHandler.new()
    json = %{{"one":true,"two":false}}
    Oj.sc_parse(handler, json)
    assert_equal([[:hash_start],
                  [:hash_set, 'one', true],
                  [:hash_set, 'two', false],
                  [:hash_end],
                  [:add_value, {}]], handler.calls)
  end

  def test_hash_sym
    handler = AllHandler.new()
    json = %{{"one":true,"two":false}}
    Oj.sc_parse(handler, json, :symbol_keys => true)
    assert_equal([[:hash_start],
                  [:hash_set, :one, true],
                  [:hash_set, :two, false],
                  [:hash_end],
                  [:add_value, {}]], handler.calls)
  end

  def test_full
    handler = AllHandler.new()
    Oj.sc_parse(handler, $json)
    assert_equal([[:hash_start],
                  [:array_start],
                  [:hash_start],
                  [:hash_set, "num", 3],
                  [:hash_set, "string", "message"],
                  [:hash_start],
                  [:hash_start],
                  [:array_start],
                  [:array_append, 1],
                  [:array_append, 2],
                  [:array_append, 3],
                  [:array_end],
                  [:hash_set, "a", []],
                  [:hash_end],
                  [:hash_set, "h2", {}],
                  [:hash_end],
                  [:hash_set, "hash", {}],
                  [:hash_end],
                  [:array_append, {}],
                  [:array_end],
                  [:hash_set, "array", []],
                  [:hash_set, "boolean", true],
                  [:hash_end],
                  [:add_value, {}]], handler.calls)
  end

  def test_none
    handler = NoHandler.new()
    Oj.sc_parse(handler, $json)
  end

  def test_fixnum_bad
    handler = AllHandler.new()
    json = %{12345xyz}
    begin
      Oj.sc_parse(handler, json)
    rescue Exception => e
      assert_equal("unexpected character at line 1, column 6 [parse.c:625]", e.message)
    end
  end

end
