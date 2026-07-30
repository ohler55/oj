#!/usr/bin/env ruby
# frozen_string_literal: true

$LOAD_PATH << __dir__

require 'helper'
require 'saj_handlers'

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

class SajTest < Minitest::Test

  def setup
    @default_options = Oj.default_options
  end

  def teardown
    Oj.default_options = @default_options
  end

  def test_nil
    handler = AllSaj.new()
    json = %{null}
    Oj.saj_parse(handler, json)
    assert_equal([[:add_value, nil, nil]], handler.calls)
  end

  def test_true
    handler = AllSaj.new()
    json = %{true}
    Oj.saj_parse(handler, json)
    assert_equal([[:add_value, true, nil]], handler.calls)
  end

  def test_false
    handler = AllSaj.new()
    json = %{false}
    Oj.saj_parse(handler, json)
    assert_equal([[:add_value, false, nil]], handler.calls)
  end

  def test_string
    handler = AllSaj.new()
    json = %{"a string"}
    Oj.saj_parse(handler, json)
    assert_equal([[:add_value, 'a string', nil]], handler.calls)
  end

  def test_fixnum
    handler = AllSaj.new()
    json = %{12345}
    Oj.saj_parse(handler, json)
    assert_equal([[:add_value, 12_345, nil]], handler.calls)
  end

  def test_float
    handler = AllSaj.new()
    json = %{12345.6789}
    Oj.saj_parse(handler, json)
    assert_equal([[:add_value, 12_345.6789, nil]], handler.calls)
  end

  def test_float_exp
    handler = AllSaj.new()
    json = %{12345.6789e7}
    Oj.saj_parse(handler, json)
    assert_equal(1, handler.calls.size)
    assert_equal(:add_value, handler.calls[0][0])
    assert_equal((12_345.6789e7 * 10_000).to_i, (handler.calls[0][1] * 10_000).to_i)
  end

  def test_array_empty
    handler = AllSaj.new()
    json = %{[]}
    Oj.saj_parse(handler, json)
    assert_equal([[:array_start, nil],
                  [:array_end, nil]], handler.calls)
  end

  def test_array
    handler = AllSaj.new()
    json = %{[true,false]}
    Oj.saj_parse(handler, json)
    assert_equal([[:array_start, nil],
                  [:add_value, true, nil],
                  [:add_value, false, nil],
                  [:array_end, nil]], handler.calls)
  end

  def test_hash_empty
    handler = AllSaj.new()
    json = %{{}}
    Oj.saj_parse(handler, json)
    assert_equal([[:hash_start, nil],
                  [:hash_end, nil]], handler.calls)
  end

  def test_hash
    handler = AllSaj.new()
    json = %{{"one":true,"two":false}}
    Oj.saj_parse(handler, json)
    assert_equal([[:hash_start, nil],
                  [:add_value, true, 'one'],
                  [:add_value, false, 'two'],
                  [:hash_end, nil]], handler.calls)
  end

  def test_full
    handler = AllSaj.new()
    Oj.saj_parse(handler, $json)
    assert_equal([[:hash_start, nil],
                  [:array_start, 'array'],
                  [:hash_start, nil],
                  [:add_value, 3, 'num'],
                  [:add_value, 'message', 'string'],
                  [:hash_start, 'hash'],
                  [:hash_start, 'h2'],
                  [:array_start, 'a'],
                  [:add_value, 1, nil],
                  [:add_value, 2, nil],
                  [:add_value, 3, nil],
                  [:array_end, 'a'],
                  [:hash_end, 'h2'],
                  [:hash_end, 'hash'],
                  [:hash_end, nil],
                  [:array_end, 'array'],
                  [:add_value, true, 'boolean'],
                  [:hash_end, nil]], handler.calls)
  end

  def test_file
    handler = AllSaj.new()
    filename = File.join(__dir__, 'saj_file_test.json')
    File.write(filename, $json)

    File.open(filename) do |f|
      Oj.saj_parse(handler, f)
    end

    assert_equal([[:hash_start, nil],
                  [:array_start, 'array'],
                  [:hash_start, nil],
                  [:add_value, 3, 'num'],
                  [:add_value, 'message', 'string'],
                  [:hash_start, 'hash'],
                  [:hash_start, 'h2'],
                  [:array_start, 'a'],
                  [:add_value, 1, nil],
                  [:add_value, 2, nil],
                  [:add_value, 3, nil],
                  [:array_end, 'a'],
                  [:hash_end, 'h2'],
                  [:hash_end, 'hash'],
                  [:hash_end, nil],
                  [:array_end, 'array'],
                  [:add_value, true, 'boolean'],
                  [:hash_end, nil]], handler.calls)
  ensure
    File.delete(filename) if filename && File.exist?(filename)
  end

  def test_fixnum_bad
    handler = AllSaj.new()
    json = %{12345xyz}
    Oj.saj_parse(handler, json)
    assert_equal([:add_value, 12_345, nil], handler.calls.first)
    type, message, line, column = handler.calls.last
    assert_equal([:error, 1, 6], [type, line, column])
    assert_match(%r{invalid format, extra characters at line 1, column 6 \[(?:[A-Za-z]:/)?(?:[a-z.]+/)*saj\.c:\d+\]}, message)
  end

  # A line comment may end on the last byte of the document. Scanning it must
  # stop on the null terminator instead of reading the byte after the buffer.
  def test_comment_at_end
    ['//', '// comment', '[1,2] // comment'].each do |json|
      handler = AllSaj.new()
      Oj.saj_parse(handler, json)
      refute_includes(handler.calls.map(&:first), :error, json)
    end
  end

  # read_quoted_value() steps over the opening quote before it checks anything,
  # so a document that ends where a key belongs has to be rejected before the
  # call rather than inside it.
  def test_key_expected_at_end
    ['{', '{ ', '[{"a":[{', '{"a":1,'].each do |json|
      err = assert_raises(Oj::ParseError, json) { Oj.saj_parse(Oj::Saj.new, json) }
      assert_match(/\Ainvalid format, expected a key at line 1, column #{json.size + 1} /, err.message, json)
    end
  end

  def test_comment_not_terminated
    ['/*', '/* comment', '[1,2] /* comment'].each do |json|
      assert_raises(Oj::ParseError, json) { Oj.saj_parse(Oj::Saj.new, json) }
    end
  end

end
