#!/usr/bin/env ruby
# encoding: UTF-8

$: << File.dirname(__FILE__)

require 'helper'

class UsualTest < Minitest::Test

  def test_nil
    p = Oj::Parser.new(:usual)
    doc = p.parse('nil')
    assert_nil(doc)
  end

  def test_primitive
    p = Oj::Parser.new(:usual)
    [
      ['true', true],
      ['false', false],
      ['123', 123],
      ['1.25', 1.25],
      ['"abc"', 'abc'],
    ].each { |x|
      doc = p.parse(x[0])
      assert_equal(x[1], doc)
    }
  end

  def test_big
    p = Oj::Parser.new(:usual)
    doc = p.parse('12345678901234567890123456789')
    assert_equal(BigDecimal, doc.class)
    doc = p.parse('1234567890.1234567890123456789')
    assert_equal(BigDecimal, doc.class)
  end

  def test_array
    p = Oj::Parser.new(:usual)
    [
      ['[]', []],
      ['[false]', [false]],
      ['[true,false]', [true,false]],
      ['[[]]', [[]]],
      ['[true,[],false]', [true,[],false]],
      ['[true,[true],false]', [true,[true],false]],
    ].each { |x|
      doc = p.parse(x[0])
      assert_equal(x[1], doc)
    }
  end

  def test_hash
    p = Oj::Parser.new(:usual)
    [
      ['{}', {}],
      ['{"a": null}', {'a' => nil}],
      ['{"t": true, "f": false, "s": "abc"}', {'t' => true, 'f' => false, 's' => 'abc'}],
      ['{"a": {}}', {'a' => {}}],
      ['{"a": {"b": 2}}', {'a' => {'b' => 2}}],
      ['{"a": [true]}', {'a' => [true]}],
    ].each { |x|
      doc = p.parse(x[0])
      assert_equal(x[1], doc)
    }
  end

  def test_symbol_keys
    p = Oj::Parser.new(:usual)
    assert_equal(false, p.symbol_keys)
    p.symbol_keys = true
    doc = p.parse('{"a": true, "b": false}')
    assert_equal({a: true, b: false}, doc)
  end

  def test_capacity
    p = Oj::Parser.new(:usual)
    p.capacity = 1000
    assert_equal(4096, p.capacity)
    p.capacity = 5000
    assert_equal(5000, p.capacity)
  end

  def test_decimal
    p = Oj::Parser.new(:usual)
    assert_equal(:auto, p.decimal)
    doc = p.parse('1.234567890123456789')
    assert_equal(BigDecimal, doc.class)
    assert_equal('0.1234567890123456789e1', doc.to_s)
    doc = p.parse('1.25')
    assert_equal(Float, doc.class)

    p.decimal = :float
    assert_equal(:float, p.decimal)
    doc = p.parse('1.234567890123456789')
    assert_equal(Float, doc.class)

    p.decimal = :bigdecimal
    assert_equal(:bigdecimal, p.decimal)
    doc = p.parse('1.234567890123456789')
    assert_equal(BigDecimal, doc.class)
    doc = p.parse('1.25')
    assert_equal(BigDecimal, doc.class)
    assert_equal('0.125e1', doc.to_s)

    p.decimal = :ruby
    assert_equal(:ruby, p.decimal)
    doc = p.parse('1.234567890123456789')
    assert_equal(Float, doc.class)
  end

end
