#!/usr/bin/env ruby
# frozen_string_literal: true

$LOAD_PATH << __dir__

require 'helper'

# This is to force testing 'rails_friendly_size'.
begin
  require 'active_support'
  HAS_RAILS = true
rescue LoadError
  puts 'ActiveSupport not found. Skipping ActiveSupport tests.'
  HAS_RAILS = false
end

# The tests in this file are to specifically handle testing the ARM Neon code
# that is used to speed up the dumping of long strings. The tests are likely
# redundant with respect to correctness. However, they are designed specifically
# to exercise the code paths that operate on vectors of 16 bytes. Additionally,
# we need to ensure that not all tests are exactly multiples of 16 bytes in length
# so that we can test the code that handles the remainder of the string.
class LongStringsTest < Minitest::Test

  def test_escapes    
    run_basic_tests(:compat)
    run_basic_tests(:rails)

    if HAS_RAILS
      Oj.optimize_rails()
      ActiveSupport::JSON::Encoding.escape_html_entities_in_json = false
      run_basic_tests(:rails)
    end
  end

  def run_basic_tests(mode)
    str = '\n'*15
    expected = "\"#{'\\\\n'*15}\""
    out = Oj.dump(str, mode: mode)
    assert_equal(expected, out)

    str = '\n'*16
    expected = "\"#{'\\\\n'*16}\""
    out = Oj.dump(str, mode: mode)
    assert_equal(expected, out)

    str = '\n'*17
    expected = "\"#{'\\\\n'*17}\""
    out = Oj.dump(str, mode: mode)
    assert_equal(expected, out)

    str = '\n'*1700
    expected = "\"#{'\\\\n'*1700}\""
    out = Oj.dump(str, mode: mode)
    assert_equal(expected, out)

    str = '\n'*32
    expected = "\"#{'\\\\n'*32}\""
    out = Oj.dump(str, mode: mode)
    assert_equal(expected, out)

    str = '\f'*63
    expected = "\"#{'\\\\f'*63}\""
    out = Oj.dump(str, mode: mode)
    assert_equal(expected, out)

    str = '\t'*127
    expected = "\"#{'\\\\t'*127}\""
    out = Oj.dump(str, mode: mode)
    assert_equal(expected, out)

    str = '\t'*500
    expected = "\"#{'\\\\t'*500}\""
    out = Oj.dump(str, mode: mode)
    assert_equal(expected, out)

    str = "\u0001" * 16
    out = Oj.dump(str, mode: mode)
    expected = "\"#{"\\u0001" * 16}\""
    assert_equal(expected, out)

    str = "\u0001" * 1024
    out = Oj.dump(str, mode: mode)
    expected = "\"#{"\\u0001" * 1024}\""
    assert_equal(expected, out)

    str = "\u0001\u0002" * 8
    out = Oj.dump(str, mode: mode)
    expected = "\"#{"\\u0001\\u0002" * 8}\""
    assert_equal(expected, out)

    str = "\u0001\u0002" * 2000
    out = Oj.dump(str, mode: mode)
    expected = "\"#{"\\u0001\\u0002" * 2000}\""
    assert_equal(expected, out)

    str = "\u0001a" * 8
    out = Oj.dump(str, mode: mode)
    expected = "\"#{"\\u0001a" * 8}\""
    assert_equal(expected, out)

    str = "abc\u0010" * 4
    out = Oj.dump(str, mode: mode)
    expected = "\"#{"abc\\u0010" * 4}\""
    assert_equal(expected, out)

    str = "abc\u0010" * 5
    out = Oj.dump(str, mode: mode)
    expected = "\"#{"abc\\u0010" * 5}\""
    assert_equal(expected, out)

    str = "\u0001\u0002" * 9
    out = Oj.dump(str, mode: mode)
    expected = "\"#{"\\u0001\\u0002" * 9}\""
    assert_equal(expected, out)

    str = "\u0001\u0002" * 2048
    out = Oj.dump(str, mode: mode)
    expected = "\"#{"\\u0001\\u0002" * 2048}\""
    assert_equal(expected, out)

    str = '\"'
    out = Oj.dump(str, mode: mode)
    expected =  "\"\\\\\\\"\""
    assert_equal(expected, out)

    str = '"'*16
    out = Oj.dump(str, mode: mode)
    expected =  '"\\"\\"\\"\\"\\"\\"\\"\\"\\"\\"\\"\\"\\"\\"\\"\\""'
    assert_equal(expected, out)

    str = '"'*20
    out = Oj.dump(str, mode: mode)
    expected =  '"\\"\\"\\"\\"\\"\\"\\"\\"\\"\\"\\"\\"\\"\\"\\"\\"\\"\\"\\"\\""'
    assert_equal(expected, out)
  end

  def test_dump_long_str_no_escapes
    str = 'This is a test of the emergency broadcast system. This is only a test.'
    out = Oj.dump(str)
    assert_equal(%|"#{str}"|, out)
  end

  def test_dump_long_str_with_escapes
    str = 'This is a\ntest of the emergency broadcast system. This is only a test.'
    out = Oj.dump(str)
    expected = %|"This is a\\\\ntest of the emergency broadcast system. This is only a test."|
    assert_equal(expected, out)
  end

  def test_dump_long_str_with_quotes
    str = 'This is a "test" of the emergency broadcast system. This is only a "test".'
    out = Oj.dump(str)
    expected = %|"This is a \\\"test\\\" of the emergency broadcast system. This is only a \\\"test\\\"."|
    assert_equal(expected, out)
  end

  def test_dump_long_str_no_escapes_rails
    str = 'This is a test of the emergency broadcast system. This is only a test.'
    out = Oj.dump(str, mode: :rails)
    assert_equal(%|"#{str}"|, out)
  end

  def test_dump_long_str_with_escapes_rails
    str = 'This is a\ntest of the emergency broadcast system. This is only a test.'
    out = Oj.dump(str, mode: :rails)
    expected = %|"This is a\\\\ntest of the emergency broadcast system. This is only a test."|
    assert_equal(expected, out)
  end

  def test_dump_long_str_with_quotes_rails
    str = 'This is a "test" of the emergency broadcast system. This is only a "test".'
    out = Oj.dump(str, mode: :rails)
    expected = %|"This is a \\\"test\\\" of the emergency broadcast system. This is only a \\\"test\\\"."|
    assert_equal(expected, out)
  end

  def test_long_string_with_high_byte_set
    str = 'This item will cost €1000.00. I hope you have a great day!'
    out = Oj.dump(str)
    expected = %["This item will cost €1000.00. I hope you have a great day!"]
    assert_equal(expected, out)

    out = Oj.dump(str, mode: :rails)
    assert_equal(expected, out)
  end

  def test_high_byte_set
    str = "€"*15
    out = Oj.dump(str)
    expected = %["#{"€"*15}"]
    assert_equal(expected, out)
    out = Oj.dump(str, mode: :rails)
    assert_equal(expected, out)

    str = "€"*16
    out = Oj.dump(str)
    expected = %["#{"€"*16}"]
    assert_equal(expected, out)
    out = Oj.dump(str, mode: :rails)
    assert_equal(expected, out)

    str = "€"*17
    out = Oj.dump(str)
    expected = %["#{"€"*17}"]
    assert_equal(expected, out)
    out = Oj.dump(str, mode: :rails)
    assert_equal(expected, out)

    str = "€"*1700
    out = Oj.dump(str)
    expected = %["#{"€"*1700}"]
    assert_equal(expected, out)
    out = Oj.dump(str, mode: :rails)
    assert_equal(expected, out)

    str = "€abcdefghijklmnop"
    out = Oj.dump(str)
    expected = %["#{"€abcdefghijklmnop"}"]
    assert_equal(expected, out)
    out = Oj.dump(str, mode: :rails)
    assert_equal(expected, out)

    str = "€" + "abcdefghijklmnop"*1000
    out = Oj.dump(str)
    expected = %["#{"€" + "abcdefghijklmnop"*1000}"]
    assert_equal(expected, out)
    out = Oj.dump(str, mode: :rails)
    assert_equal(expected, out)

    str = "€" + "abcdefghijklmnop" * 2000 + "€"
    out = Oj.dump(str)
    expected = %["#{"€" + "abcdefghijklmnop" * 2000 + "€"}"]
    assert_equal(expected, out)
    out = Oj.dump(str, mode: :rails)
    assert_equal(expected, out)

    str = "abcdefghijklmnop"*3000 + "€"
    out = Oj.dump(str)
    expected = %["#{"abcdefghijklmnop"*3000 + "€"}"]
    assert_equal(expected, out)
    out = Oj.dump(str, mode: :rails)
    assert_equal(expected, out)
  end
end