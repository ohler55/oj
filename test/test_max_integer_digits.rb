#!/usr/bin/env ruby
# frozen_string_literal: true

$LOAD_PATH << __dir__
@oj_dir = File.dirname(File.expand_path(__dir__))
%w(lib ext).each do |dir|
  $LOAD_PATH << File.join(@oj_dir, dir)
end

require 'minitest'
require 'minitest/autorun'
require 'oj'

class MaxIntegerDigitsTest < Minitest::Test
  def setup
    @default_options = Oj.default_options
  end

  def teardown
    Oj.default_options = @default_options
  end

  def test_default_is_zero
    assert_equal(0, Oj.default_options[:max_integer_digits])
  end

  def test_default_unlimited_allows_huge_integer
    huge = '1' * 10_000
    result = Oj.load(huge)

    assert_equal(huge.to_i, result)
  end

  def test_limit_allows_value_within_limit
    Oj.default_options = { max_integer_digits: 100 }
    val = '9' * 100

    assert_equal(val.to_i, Oj.load(val))
  end

  def test_limit_rejects_value_over_limit
    Oj.default_options = { max_integer_digits: 100 }

    assert_raises(Oj::ParseError) do
      Oj.load('1' * 101)
    end
  end

  def test_limit_rejects_bignum_over_limit
    Oj.default_options = { max_integer_digits: 50 }

    assert_raises(Oj::ParseError) do
      Oj.load('9' * 1000)
    end
  end

  def test_negative_integer_does_not_count_minus_sign
    Oj.default_options = { max_integer_digits: 10 }
    val = '-' + '1' * 10

    assert_equal(val.to_i, Oj.load(val))
  end

  def test_negative_integer_over_limit_raises
    Oj.default_options = { max_integer_digits: 10 }
    val = '-' + '1' * 11

    assert_raises(Oj::ParseError) do
      Oj.load(val)
    end
  end

  def test_floats_not_affected
    Oj.default_options = { max_integer_digits: 5 }

    refute_nil(Oj.load('0.' + '1' * 100))
    refute_nil(Oj.load('1.' + '1' * 100))
  end

  def test_numbers_with_exponent_not_affected
    Oj.default_options = { max_integer_digits: 5 }

    refute_nil(Oj.load('1e10'))
    refute_nil(Oj.load('1.5e100'))
  end

  def test_per_call_option
    Oj.default_options = { max_integer_digits: 0 }

    assert_raises(Oj::ParseError) do
      Oj.load('1' * 50, max_integer_digits: 20)
    end
  end

  def test_per_call_overrides_default
    Oj.default_options = { max_integer_digits: 5 }
    val = '1' * 10

    assert_equal(val.to_i, Oj.load(val, max_integer_digits: 20))
  end

  def test_nil_resets_to_zero
    Oj.default_options = { max_integer_digits: 100 }
    Oj.default_options = { max_integer_digits: nil }

    assert_equal(0, Oj.default_options[:max_integer_digits])
  end

  def test_false_resets_to_zero
    Oj.default_options = { max_integer_digits: 100 }
    Oj.default_options = { max_integer_digits: false }

    assert_equal(0, Oj.default_options[:max_integer_digits])
  end

  def test_negative_value_raises_argument_error
    assert_raises(ArgumentError) do
      Oj.default_options = { max_integer_digits: -1 }
    end
  end

  def test_non_integer_raises_argument_error
    assert_raises(ArgumentError) do
      Oj.default_options = { max_integer_digits: 'abc' }
    end
  end

  def test_applies_across_legacy_modes
    # All legacy parse modes should refuse to parse an integer that exceeds the
    # configured limit. Different modes raise different error classes (e.g.
    # compat mode raises EncodingError to mimic the json gem), so we accept any
    # StandardError that includes our marker message.
    [:strict, :null, :compat, :object, :custom, :rails, :wab].each do |mode|
      err = assert_raises(StandardError, "mode #{mode} did not enforce limit") do
        Oj.load('1' * 100, mode: mode, max_integer_digits: 50)
      end

      assert_match(/max_integer_digits/, err.message, "mode #{mode} raised unexpected error")
    end
  end

  def test_oj_doc_honors_default_options_limit
    Oj.default_options = { max_integer_digits: 50 }

    assert_raises(Oj::ParseError) do
      Oj::Doc.open('9' * 100) { |doc| doc.fetch }
    end
  end

  def test_oj_doc_within_limit_works
    Oj.default_options = { max_integer_digits: 100 }
    val = '9' * 50
    result = Oj::Doc.open(val) { |doc| doc.fetch }

    assert_equal(val.to_i, result)
  end

  def test_oj_parser_is_unaffected
    # The new Oj::Parser API does not convert big numeric tokens via
    # rb_cstr_to_inum and so is not subject to this limit. Confirm that
    # setting :max_integer_digits does not break Oj::Parser parsing.
    Oj.default_options = { max_integer_digits: 5 }
    result = Oj::Parser.new(:usual).parse('9' * 50)

    refute_nil(result)
  end

  def test_integer_in_array_enforced
    Oj.default_options = { max_integer_digits: 10 }

    assert_raises(Oj::ParseError) do
      Oj.load('[1, 2, ' + ('9' * 100) + ']')
    end
  end

  def test_integer_in_hash_enforced
    Oj.default_options = { max_integer_digits: 10 }

    assert_raises(Oj::ParseError) do
      Oj.load('{"a": ' + ('9' * 100) + '}')
    end
  end

  def test_get_option_via_hash
    Oj.default_options = { max_integer_digits: 42 }

    assert_equal(42, Oj.default_options[:max_integer_digits])
  end
end
