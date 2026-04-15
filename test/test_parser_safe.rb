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
end
