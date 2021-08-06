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

  def test_omit_null
    p = Oj::Parser.new(:usual)
    p.omit_null = true
    doc = p.parse('{"a":true,"b":null}')
    assert_equal({'a'=>true}, doc)

    p.omit_null = false
    doc = p.parse('{"a":true,"b":null}')
    assert_equal({'a'=>true, 'b'=>nil}, doc)
  end

  class MyArray < Array
  end

  def test_array_class
    p = Oj::Parser.new(:usual)
    p.array_class = MyArray
    assert_equal(MyArray, p.array_class)
    doc = p.parse('[true]')
    assert_equal(MyArray, doc.class)
  end

  class MyHash < Hash
  end

  def test_hash_class
    p = Oj::Parser.new(:usual)
    p.hash_class = MyHash
    assert_equal(MyHash, p.hash_class)
    doc = p.parse('{"a":true}')
    assert_equal(MyHash, doc.class)
  end

  class MyClass
    attr_accessor :a
    attr_accessor :b

    def to_s
      "MyClass{a: #{@a} b: #{b}}"
    end
  end

  def test_create_id
    p = Oj::Parser.new(:usual)
    p.create_id = '^'
    doc = p.parse('{"a":true}')
    assert_equal(Hash, doc.class)
    doc = p.parse('{"a":true,"^":"UsualTest::MyClass","b":false}')

    puts "\n*** #{doc.class} #{doc}"

    return
    p.hash_class = MyHash
    assert_equal(MyHash, p.hash_class)
    doc = p.parse('{"a":true}')
    assert_equal(MyHash, doc.class)

    doc = p.parse('{"a":true,"^":"UsualTest::MyClass","b":false}')

    puts "\n*** #{doc.class} #{doc}"
    # TBD
  end

end
