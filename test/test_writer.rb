#!/usr/bin/env ruby
# encoding: UTF-8

# Ubuntu does not accept arguments to ruby when called using env. To get warnings to show up the -w options is
# required. That can be set in the RUBYOPT environment variable.
# export RUBYOPT=-w

$VERBOSE = true

$: << File.join(File.dirname(__FILE__), "../lib")
$: << File.join(File.dirname(__FILE__), "../ext")

require 'test/unit'
require 'stringio'
require 'date'
require 'bigdecimal'
require 'oj'

class OjWriter < ::Test::Unit::TestCase

  def test_string_writer_empty_array
    w = Oj::StringWriter.new(:indent => 0)
    w.push_array()
    w.pop()
    assert_equal('[]', w.to_s)

  end

  def test_string_writer_nested_array
    w = Oj::StringWriter.new(:indent => 0)
    w.push_array()
    w.push_array()
    w.pop()
    w.push_array()
    w.push_array()
    w.pop()
    w.pop()
    w.push_array()
    w.pop()
    w.pop()
    assert_equal('[[],[[]],[]]', w.to_s)
  end

  def test_string_writer_empty_object
    w = Oj::StringWriter.new(:indent => 0)
    w.push_object()
    w.pop()
    assert_equal('{}', w.to_s)
  end

  def test_string_writer_nested_object
    w = Oj::StringWriter.new(:indent => 0)
    w.push_object()
    w.push_object("a1")
    w.pop()
    w.push_object("a2")
    w.push_object("b")
    w.pop()
    w.pop()
    w.push_object("a3")
    w.pop()
    w.pop()
    assert_equal('{"a1":{},"a2":{"b":{}},"a3":{}}', w.to_s)
  end

  def test_string_writer_value_array
    w = Oj::StringWriter.new(:indent => 0)
    w.push_array()
    w.push_value(7)
    w.push_value(7.3)
    w.push_value(true)
    w.push_value(nil)
    w.push_value("a string")
    w.push_value({'a' => 65})
    w.push_value([1,2])
    w.pop()
    assert_equal('[7,7.3,true,null,"a string",{"a":65},[1,2]]', w.to_s)
  end

  def test_string_writer_pop_excess
    w = Oj::StringWriter.new(:indent => 0)
    begin
      w.pop()
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end

  def test_string_writer_obj_no_key
    w = Oj::StringWriter.new(:indent => 0)
    w.push_object()
    begin
      w.push_value(59)
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end

  def test_string_writer_array_with_key
    w = Oj::StringWriter.new(:indent => 0)
    w.push_array()
    begin
      w.push_value(59, 'x')
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end

  def test_string_writer_deep
    cnt = 1000
    w = Oj::StringWriter.new(:indent => 0)
    cnt.times do
      w.push_array()
    end
    cnt.times do
      w.pop()
    end
    # if no exception then passed
    assert(true)
  end

  def test_string_writer_pop_all
    w = Oj::StringWriter.new(:indent => 0)
    w.push_object()
    w.push_object("a1")
    w.pop()
    w.push_array("a2")
    w.push_value(3)
    w.push_array()
    w.pop_all()
    assert_equal('{"a1":{},"a2":[3,[]]}', w.to_s)
  end

  def test_string_writer_reset
    w = Oj::StringWriter.new(:indent => 0)
    w.push_array()
    w.pop()
    w.reset()
    assert_equal('', w.to_s)
  end

end # OjWriter
