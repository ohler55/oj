#!/usr/bin/env ruby -wW1
# encoding: UTF-8

$: << File.join(File.dirname(__FILE__), "../lib")
$: << File.join(File.dirname(__FILE__), "../ext")

require 'test/unit'
require 'oj'

$json1 = %{{
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

class DocTest < ::Test::Unit::TestCase
  def test_nil
    json = %{null}
    Oj::Doc.open(json) do |f|
      assert_equal(NilClass, f.type)
      assert_equal(nil, f.fetch())
    end
  end

  def test_true
    json = %{true}
    Oj::Doc.open(json) do |f|
      assert_equal(TrueClass, f.type)
      assert_equal(true, f.fetch())
    end
  end

  def test_false
    json = %{false}
    Oj::Doc.open(json) do |f|
      assert_equal(FalseClass, f.type)
      assert_equal(false, f.fetch())
    end
  end

  def test_string
    json = %{"a string"}
    Oj::Doc.open(json) do |f|
      assert_equal(String, f.type)
      assert_equal('a string', f.fetch())
    end
  end

  def test_fixnum
    json = %{12345}
    Oj::Doc.open(json) do |f|
      assert_equal(Fixnum, f.type)
      assert_equal(12345, f.fetch())
    end
  end

  def test_float
    json = %{12345.6789}
    Oj::Doc.open(json) do |f|
      assert_equal(Float, f.type)
      assert_equal(12345.6789, f.fetch())
    end
  end

  def test_float_exp
    json = %{12345.6789e7}
    Oj::Doc.open(json) do |f|
      assert_equal(Float, f.type)
      assert_equal(12345.6789e7, f.fetch())
    end
  end

  def test_array_empty
    json = %{[]}
    Oj::Doc.open(json) do |f|
      assert_equal(Array, f.type)
      assert_equal([], f.fetch())
    end
  end

  def test_array
    json = %{[true,false]}
    Oj::Doc.open(json) do |f|
      assert_equal(Array, f.type)
      assert_equal([true, false], f.fetch())
    end
  end

  def test_hash_empty
    json = %{{}}
    Oj::Doc.open(json) do |f|
      assert_equal(Hash, f.type)
      assert_equal({}, f.fetch())
    end
  end

  def test_hash
    json = %{{"one":true,"two":false}}
    Oj::Doc.open(json) do |f|
      assert_equal(Hash, f.type)
      assert_equal({'one' => true, 'two' => false}, f.fetch())
    end
  end

  # move() and where?()
  def test_move_hash
    json = %{{"one":{"two":false}}}
    Oj::Doc.open(json) do |f|
      f.move('/one')
      assert_equal('/one', f.where?)
      f.move('/one/two')
      assert_equal('/one/two', f.where?)
    end
  end

  def test_move_array
    json = %{[1,[2,true]]}
    Oj::Doc.open(json) do |f|
      f.move('/1')
      assert_equal('/1', f.where?)
      f.move('/2/1')
      assert_equal('/2/1', f.where?)
    end
  end

  def test_move
    Oj::Doc.open($json1) do |f|
      [ '/',
        '/array',
        '/boolean',
        '/array/1/hash/h2/a/3',
      ].each do |p|
        f.move(p)
        assert_equal(p, f.where?)
      end
      begin
        f.move('/array/x')
      rescue Exception => e
        assert_equal('/', f.where?)
        assert(true)
      end
    end
  end

  def test_move_relative
    Oj::Doc.open($json1) do |f|
      [['/', 'array', '/array'],
       ['/array', '1/num', '/array/1/num'],
       ['/array/1/hash', 'h2/a', '/array/1/hash/h2/a'],
       ['/array/1', 'hash/h2/a/2', '/array/1/hash/h2/a/2'],
       ['/array/1/hash', '../string', '/array/1/string'],
       ['/array/1/hash', '..', '/array/1'],
      ].each do |start,path,where|
        f.move(start)
        f.move(path)
        assert_equal(where, f.where?)
      end
    end
  end

  def test_type
    Oj::Doc.open($json1) do |f|
      [['/', Hash],
       ['/array', Array],
       ['/array/1', Hash],
       ['/array/1/num', Fixnum],
       ['/array/1/string', String],
       ['/array/1/hash/h2/a', Array],
       ['/array/1/hash/../num', Fixnum],
      ].each do |path,type|
        assert_equal(type, f.type(path))
      end
    end
  end

  # TBD
  # type
  # local_key
  # home
  # fetch
  # each_leaf
  # each_branch
  # each_value
  # dump

end # DocTest
