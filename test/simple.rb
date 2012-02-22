#!/usr/bin/env ruby -wW1
# encoding: UTF-8

$: << File.join(File.dirname(__FILE__), "../lib")
$: << File.join(File.dirname(__FILE__), "../ext")

require 'test/unit'
require 'oj'

class Jeez
  def initialize(x, y)
    @x = x
    @y = y
  end
  
  def to_json()
    %{{"x":#{@x},"y":#{@y}}}
  end
  
end # Jeez

class Jazz
  def initialize(x, y)
    @x = x
    @y = y
  end
  def to_hash()
    { 'x' => @x, 'y' => @y }
  end
  
end # Jazz

class Juice < ::Test::Unit::TestCase

  def test_get_options
    opts = Oj.default_options()
    assert_equal(opts, {
                   :encoding=>nil,
                   :indent=>0,
                   :circular=>false,
                   :mode=>:object})
  end

  def test_set_options
    orig = {
      :encoding=>nil,
      :indent=>0,
      :circular=>false,
      :mode=>:object}
    o2 = {
      :encoding=>"UTF-8",
      :indent=>4,
      :circular=>true,
      :mode=>:compat}
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
    dump_and_load(-54321.012, false)
    dump_and_load(2.48e16, false)
  end

  def test_string
    dump_and_load('', false)
    dump_and_load('abc', false)
    dump_and_load("abc\ndef", false)
    dump_and_load("a\u0041", false)
  end

  def test_class
    begin
      json = Oj.dump(self.class, :mode => :strict)
    rescue Exception => e
      assert(true)
    end
    json = Oj.dump(self.class, :mode => :object)
    assert_equal('{"*":"Class","-":"Juice"}', json)
    json = Oj.dump(self.class, :mode => :null)
    assert_equal('null', json)
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
  
  def test_encode
    Oj.default_options = { :encoding => 'UTF-8' }
    dump_and_load("ぴーたー", false)
    Oj.default_options = { :encoding => nil }
  end

  def test_time
    t = Time.new(2012, 1, 5, 23, 58, 7)
    begin
      json = Oj.dump(t, :mode => :strict)
    rescue Exception => e
      assert(true)
    end
    json = Oj.dump(t, :mode => :compat)
    assert_equal(%{{"*":"Time","-":1325775487.000000}}, json)
    json = Oj.dump(t, :mode => :null)
    assert_equal('null', json)
  end

  def test_json_object
    begin
      json = Oj.dump(Jeez.new(true, 58), :mode => :strict)
    rescue Exception => e
      assert(true)
    end
    json = Oj.dump(Jeez.new(true, 58), :mode => :compat, :indent => 2)
    assert_equal(%{{"x":true,"y":58}}, json)
    json = Oj.dump(Jeez.new(true, 58), :mode => :null)
    assert_equal('null', json)
  end

  def test_hash_object
    begin
      json = Oj.dump(Jazz.new(true, 58), :mode => :strict)
    rescue Exception => e
      assert(true)
    end
    json = Oj.dump(Jazz.new(true, 58), :mode => :compat, :indent => 2)
    assert_equal(%{{
  "x":true,
  "y":58}}, json)
    json = Oj.dump(Jazz.new(true, 58), :mode => :null)
    assert_equal('null', json)
  end

  def test_object
    begin
      json = Oj.dump(Jazz.new(true, 58), :mode => :strict)
    rescue Exception => e
      assert(true)
    end
    json = Oj.dump(Jazz.new(true, 58), :mode => :object, :indent => 2)
    assert_equal(%{{
  "*":"Jazz",
  "x":true,
  "y":58}}, json)
    json = Oj.dump(Jazz.new(true, 58), :mode => :null)
    assert_equal('null', json)
  end

  def dump_and_load(obj, trace=false)
    json = Oj.dump(obj, :indent => 2)
    puts json if trace
    loaded = Oj.load(json);
    assert_equal(obj, loaded)
    loaded
  end

end
