#!/usr/bin/env ruby -wW1
# encoding: UTF-8

$: << File.join(File.dirname(__FILE__), "../lib")
$: << File.join(File.dirname(__FILE__), "../ext")

require 'test/unit'
require 'oj'

class Juice < ::Test::Unit::TestCase

  def test_get_options
    opts = Oj.default_options()
    assert_equal(opts, {
                   :encoding=>nil,
                   :indent=>2,
                   :circular=>false,
                   :mode=>nil,
                   :effort=>:tolerant})
  end

  def test_set_options
    orig = {
      :encoding=>nil,
      :indent=>2,
      :circular=>false,
      :mode=>nil,
      :effort=>:strict}
    o2 = {
      :encoding=>"UTF-8",
      :indent=>4,
      :circular=>true,
      :mode=>:object,
      :effort=>:tolerant }
    o3 = { :indent => 4 }
    Oj.default_options = o2
    opts = Oj.default_options()
    assert_equal(opts, o2);
    Oj.default_options = o3 # see if it throws an exception
    Oj.default_options = orig # return to original
  end

  def test_nil
    dump_and_load(nil, false)
  end

  def test_true
    dump_and_load(true, false)
  end

  def test_false
    dump_and_load(false, false)
  end

  def test_fixnum
    dump_and_load(0, false)
    dump_and_load(12345, false)
    dump_and_load(-54321, false)
    dump_and_load(1, false)
  end

  def test_float
    dump_and_load(0.0, false)
    dump_and_load(12345.6789, false)
    dump_and_load(-54321.012, true)
    dump_and_load(2.48e16, false)
  end

  def test_string
    dump_and_load('', false)
    dump_and_load('abc', false)
    dump_and_load("abc\ndef", false)
    dump_and_load("a\u0041", false)
  end

  def test_array
    dump_and_load([], false)
    dump_and_load([true, false], false)
    dump_and_load(['a', 1, nil], false)
    dump_and_load([[nil]], false)
    dump_and_load([[nil], 58], false)
  end

  def test_hash
    dump_and_load({}, false)
    dump_and_load({ 'true' => true, 'false' => false}, false)
    dump_and_load({ 'true' => true, 'array' => [], 'hash' => { }}, false)
  end

  
  
  def dump_and_load(obj, trace=false)
    json = Oj.dump(obj, :indent => 2)
    puts json if trace
    loaded = Oj.load(json, :mode => :simple);
    assert_equal(obj, loaded)
    loaded
  end

end
