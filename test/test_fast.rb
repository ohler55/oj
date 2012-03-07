#!/usr/bin/env ruby -wW1
# encoding: UTF-8

$: << File.join(File.dirname(__FILE__), "../lib")
$: << File.join(File.dirname(__FILE__), "../ext")

require 'test/unit'
require 'oj'

class FastTest < ::Test::Unit::TestCase
  def test_nil
    json = %{null}
    Oj::Fast.open(json) do |f|
      assert_equal(NilClass, f.type)
      assert_equal(nil, f.value_at())
    end
  end

  def test_true
    json = %{true}
    Oj::Fast.open(json) do |f|
      assert_equal(TrueClass, f.type)
      assert_equal(true, f.value_at())
    end
  end

  def test_false
    json = %{false}
    Oj::Fast.open(json) do |f|
      assert_equal(FalseClass, f.type)
      assert_equal(false, f.value_at())
    end
  end

  def test_string
    json = %{"a string"}
    Oj::Fast.open(json) do |f|
      assert_equal(String, f.type)
      assert_equal('a string', f.value_at())
    end
  end

  def test_fixnum
    json = %{12345}
    Oj::Fast.open(json) do |f|
      assert_equal(Fixnum, f.type)
      assert_equal(12345, f.value_at())
    end
  end

  def test_float
    json = %{12345.6789}
    Oj::Fast.open(json) do |f|
      assert_equal(Float, f.type)
      assert_equal(12345.6789, f.value_at())
    end
  end

  def test_float_exp
    json = %{12345.6789e7}
    Oj::Fast.open(json) do |f|
      assert_equal(Float, f.type)
      assert_equal(12345.6789e7, f.value_at())
    end
  end

  def test_array_empty
    json = %{[]}
    Oj::Fast.open(json) do |f|
      assert_equal(Array, f.type)
      assert_equal([], f.value_at())
    end
  end

  def test_array
    json = %{[true,false]}
    Oj::Fast.open(json) do |f|
      assert_equal(Array, f.type)
      assert_equal([true, false], f.value_at())
    end
  end

  def test_hash_empty
    json = %{{}}
    Oj::Fast.open(json) do |f|
      assert_equal(Hash, f.type)
      assert_equal({}, f.value_at())
    end
  end

  def test_hash
    json = %{{"one":true,"two":false}}
    Oj::Fast.open(json) do |f|
      assert_equal(Hash, f.type)
      assert_equal({'one' => true, 'two' => false}, f.value_at())
    end
  end

end # FastTest
