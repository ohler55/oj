#!/usr/bin/env ruby
# frozen_string_literal: true

$LOAD_PATH << __dir__

require 'helper'
require 'scp_handlers'
require 'socket'
require 'stringio'

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

class ScpTest < Minitest::Test

  def setup
    @default_options = Oj.default_options
  end

  def teardown
    Oj.default_options = @default_options
  end

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
    assert_equal([[:add_value, 12_345]], handler.calls)
  end

  def test_float
    handler = AllHandler.new()
    json = %{12345.6789}
    Oj.sc_parse(handler, json)
    assert_equal([[:add_value, 12_345.6789]], handler.calls)
  end

  def test_float_exp
    handler = AllHandler.new()
    json = %{12345.6789e7}
    Oj.sc_parse(handler, json)
    assert_equal(1, handler.calls.size)
    assert_equal(:add_value, handler.calls[0][0])
    assert_equal((12_345.6789e7 * 10_000).to_i, (handler.calls[0][1] * 10_000).to_i)
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
                  [:hash_key, 'one'],
                  [:hash_set, 'one', true],
                  [:hash_key, 'two'],
                  [:hash_set, 'too', false],
                  [:hash_end],
                  [:add_value, {}]], handler.calls)
  end

  def test_hash_sym
    handler = AllHandler.new()
    json = %{{"one":true,"two":false}}
    Oj.sc_parse(handler, json, :symbol_keys => true)
    assert_equal([[:hash_start],
                  [:hash_key, 'one'],
                  [:hash_set, 'one', true],
                  [:hash_key, 'two'],
                  [:hash_set, 'too', false],
                  [:hash_end],
                  [:add_value, {}]], handler.calls)
  end

  def test_symbol_hash_key_without_symbol_keys
    handler = AllHandler.new()
    json = %{{"one":true,"symbol":false}}
    Oj.sc_parse(handler, json)
    assert_equal([[:hash_start],
                  [:hash_key, 'one'],
                  [:hash_set, 'one', true],
                  [:hash_key, 'symbol'],
                  [:hash_set, :symbol, false],
                  [:hash_end],
                  [:add_value, {}]], handler.calls)
  end

  def test_full
    handler = AllHandler.new()
    Oj.sc_parse(handler, $json)
    assert_equal([[:hash_start],
                  [:hash_key, 'array'],
                  [:array_start],
                  [:hash_start],
                  [:hash_key, 'num'],
                  [:hash_set, 'num', 3],
                  [:hash_key, 'string'],
                  [:hash_set, 'string', 'message'],
                  [:hash_key, 'hash'],
                  [:hash_start],
                  [:hash_key, 'h2'],
                  [:hash_start],
                  [:hash_key, 'a'],
                  [:array_start],
                  [:array_append, 1],
                  [:array_append, 2],
                  [:array_append, 3],
                  [:array_end],
                  [:hash_set, 'a', []],
                  [:hash_end],
                  [:hash_set, 'h2', {}],
                  [:hash_end],
                  [:hash_set, 'hash', {}],
                  [:hash_end],
                  [:array_append, {}],
                  [:array_end],
                  [:hash_set, 'array', []],
                  [:hash_key, 'boolean'],
                  [:hash_set, 'boolean', true],
                  [:hash_end],
                  [:add_value, {}]], handler.calls)
  end

  def test_double
    handler = AllHandler.new()
    json = %{{"one":true,"two":false}{"three":true,"four":false}}
    Oj.sc_parse(handler, json)
    assert_equal([[:hash_start],
                  [:hash_key, 'one'],
                  [:hash_set, 'one', true],
                  [:hash_key, 'two'],
                  [:hash_set, 'too', false],
                  [:hash_end],
                  [:add_value, {}],
                  [:hash_start],
                  [:hash_key, 'three'],
                  [:hash_set, 'three', true],
                  [:hash_key, 'four'],
                  [:hash_set, 'four', false],
                  [:hash_end],
                  [:add_value, {}]], handler.calls)
  end

  def test_double_io
    handler = AllHandler.new()
    json = %{{"one":true,"two":false}{"three":true,"four":false}}
    Oj.sc_parse(handler, StringIO.new(json))
    assert_equal([[:hash_start],
                  [:hash_key, 'one'],
                  [:hash_set, 'one', true],
                  [:hash_key, 'two'],
                  [:hash_set, 'too', false],
                  [:hash_end],
                  [:add_value, {}],
                  [:hash_start],
                  [:hash_key, 'three'],
                  [:hash_set, 'three', true],
                  [:hash_key, 'four'],
                  [:hash_set, 'four', false],
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
    assert_raises Oj::ParseError do
      Oj.sc_parse(handler, json)
    end
  end

  def test_null_string
    handler = AllHandler.new()
    json = %{"\0"}
    assert_raises Oj::ParseError do
      Oj.sc_parse(handler, json)
    end
  end

  def test_bad_bignum
    handler = AllHandler.new()
    json = %|{"big":-e123456789}|
    assert_raises Exception do # Can be either Oj::ParseError or ArgumentError depending on Ruby version
      Oj.sc_parse(handler, json)
    end
  end

  def test_socket_close
    json = %{{"one":true,"two":false}}
    begin
      server = TCPServer.new(8080)
    rescue
      # Not able to open a socket to run the test. Might be Travis.
      return
    end
    Thread.start(json) do |_j|
      c = server.accept()
      c.puts json[0..11]
      10.times {
        break if c.closed?

        sleep(0.1)
      }
      unless c.closed?
        c.puts json[12..]
        c.close
      end
    end
    begin
      sock = TCPSocket.new('localhost', 8080)
    rescue
      # Not able to open a socket to run the test. Might be Travis.
      return
    end
    handler = Closer.new(sock)
    err = nil
    begin
      Oj.sc_parse(handler, sock)
    rescue Exception => e
      err = e.class.to_s
    end
    assert_equal('IOError', err)
    assert_equal([[:hash_start],
                  [:hash_key, 'one'],
                  [:hash_set, 'one', true]], handler.calls)
  end

end
