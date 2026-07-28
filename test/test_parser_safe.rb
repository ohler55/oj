# frozen_string_literal: true

$LOAD_PATH << __dir__

require 'helper'

class ParserSafeTest < Minitest::Test
  def test_invalid_max_array_size_option
    error = assert_raises(ArgumentError) { Oj::Parser.safe(max_array_size: :foo) }

    assert_equal('Incorrect value provided for `max_array_size`', error.message)
  end

  def test_json_array_with_fewer_items
    parser = Oj::Parser.safe(max_array_size: 2)
    result = parser.parse('[1, 2]')

    assert_equal(result, [1, 2])
  end

  def test_json_array_with_too_many_items
    parser = Oj::Parser.safe(max_array_size: 2)

    error = assert_raises(Oj::Parser::ArraySizeError) { parser.parse('[1, 2, 3]') }

    assert_equal('Too many array items!', error.message)
  end

  def test_invalid_max_hash_size_option
    error = assert_raises(ArgumentError) { Oj::Parser.safe(max_hash_size: :foo) }

    assert_equal('Incorrect value provided for `max_hash_size`', error.message)
  end

  def test_json_object_with_fewer_members
    parser = Oj::Parser.safe(max_hash_size: 2)
    result = parser.parse('{ "foo": 1, "bar": 2 }')

    assert_equal(result, { 'foo' => 1, 'bar' => 2 })
  end

  def test_json_object_with_too_many_members
    parser = Oj::Parser.safe(max_hash_size: 2)

    error = assert_raises(Oj::Parser::HashSizeError) { parser.parse('{ "foo": 1, "bar": 2, "zoo": 3 }') }

    assert_equal('Too many object items!', error.message)
  end

  def test_invalid_max_depth_option
    error = assert_raises(ArgumentError) { Oj::Parser.safe(max_depth: :foo) }

    assert_equal('Incorrect value provided for `max_depth`', error.message)
  end

  def test_shallow_json_document
    parser = Oj::Parser.safe(max_depth: 2)
    result = parser.parse('[[]]')

    assert_equal(result, [[]])
  end

  def test_deep_json_document
    parser = Oj::Parser.safe(max_depth: 2)

    error = assert_raises(Oj::Parser::DepthError) { parser.parse('[[[]]]') }

    assert_equal('JSON is too deep!', error.message)
  end

  def test_invalid_max_total_elements_option
    error = assert_raises(ArgumentError) { Oj::Parser.safe(max_total_elements: :foo) }

    assert_equal('Incorrect value provided for `max_total_elements`', error.message)
  end

  def test_json_with_fewer_elements
    parser = Oj::Parser.safe(max_total_elements: 3)
    result = parser.parse('{ "foo": "bar" }')

    assert_equal(result, { 'foo' => 'bar' })
  end

  def test_json_with_too_many_elements
    parser = Oj::Parser.safe(max_total_elements: 3)

    error = assert_raises(Oj::Parser::TotalElementsError) { parser.parse('{ "foo": [1] }') }

    assert_equal('Too many elements!', error.message)
  end

  # The limits are kept in a long int and Qnil, the value that stood for
  # unset, is 4 as a number, so a limit of exactly 4 was ignored.
  def test_limit_of_four_is_still_a_limit
    assert_raises(Oj::Parser::ArraySizeError) do
      Oj::Parser.safe(max_array_size: 4).parse('[1, 2, 3, 4, 5]')
    end
    assert_raises(Oj::Parser::HashSizeError) do
      Oj::Parser.safe(max_hash_size: 4).parse('{ "a": 1, "b": 2, "c": 3, "d": 4, "e": 5 }')
    end
    assert_raises(Oj::Parser::DepthError) do
      Oj::Parser.safe(max_depth: 4).parse('[[[[[1]]]]]')
    end
    assert_raises(Oj::Parser::TotalElementsError) do
      Oj::Parser.safe(max_total_elements: 4).parse('[1, 2, 3, 4, 5]')
    end
  end

  def test_no_limit_when_not_given
    parser = Oj::Parser.safe

    assert_equal([1, 2, 3, 4, 5], parser.parse('[1, 2, 3, 4, 5]'))
  end

  # The limits are applied by wrapping the parser function table, and the
  # option setters of the usual parser write to the same slots, so setting an
  # option used to take the counting back out for that kind of value.
  def test_limits_survive_setting_an_option
    floats = "[#{(['1.5'] * 100).join(',')}]"
    parser = Oj::Parser.safe(max_array_size: 10)
    parser.decimal = :float
    assert_raises(Oj::Parser::ArraySizeError) { parser.parse(floats) }
    assert_equal(Float, parser.parse('[1.234567890123456789]').first.class)

    strings = "{#{(0...100).map { |i| %("k#{i}":"v") }.join(',')}}"
    parser = Oj::Parser.safe(max_hash_size: 10)
    parser.create_id = '^'
    assert_raises(Oj::Parser::HashSizeError) { parser.parse(strings) }

    nulls = "{#{(0...100).map { |i| %("k#{i}":null) }.join(',')}}"
    parser = Oj::Parser.safe(max_total_elements: 10)
    parser.omit_null = true
    assert_raises(Oj::Parser::TotalElementsError) { parser.parse(nulls) }
  end

  # omit_null is reported by looking at the add_null slot, which is one of the
  # slots the limits are applied through.
  def test_omit_null_reports_itself
    parser = Oj::Parser.safe(max_hash_size: 10)
    refute(parser.omit_null)

    parser.omit_null = true
    assert(parser.omit_null)
    assert_empty(parser.parse('{"a":null,"b":null}'))

    parser.omit_null = false
    refute(parser.omit_null)
    assert_equal({'a' => nil, 'b' => nil}, parser.parse('{"a":null,"b":null}'))
  end
end
