#!/usr/bin/env ruby -wW1
# encoding: UTF-8

$: << File.join(File.dirname(__FILE__), "../lib")
$: << File.join(File.dirname(__FILE__), "../ext")

require 'test/unit'
require 'stringio'
require 'oj'

class Jam
  attr_accessor :x, :y

  def initialize(x, y)
    @x = x
    @y = y
  end

  def eql?(o)
    self.class == o.class && @x == o.x && @y == o.y
  end
  alias == eql?

  def to_json()
    %{{"json_class":"#{self.class}","x":#{@x},"y":#{@y}}}
  end

  def self.json_create(h)
    self.new(h['x'], h['y'])
  end

end # Jam

class Mimic < ::Test::Unit::TestCase

  def test0_mimic_json
    assert(defined?(JSON).nil?)
    Oj.mimic_JSON
    assert(!defined?(JSON).nil?)
  end

# dump
  def test_dump_string
    json = JSON.dump([1, true, nil])
    assert_equal(%{[1,true,null]}, json)
  end

  def test_dump_io
    s = StringIO.new()
    json = JSON.dump([1, true, nil], s)
    assert_equal(s, json)
    assert_equal(%{[1,true,null]}, s.string)
  end
  # TBD options

# load
  def test_load_string
    json = %{{"a":1,"b":[true,false]}}
    obj = JSON.load(json)
    assert_equal({ 'a' => 1, 'b' => [true, false]}, obj)
  end

  def test_load_io
    json = %{{"a":1,"b":[true,false]}}
    obj = JSON.load(StringIO.new(json))
    assert_equal({ 'a' => 1, 'b' => [true, false]}, obj)
  end

  def test_load_proc
    children = []
    json = %{{"a":1,"b":[true,false]}}
    p = Proc.new {|x| children << x }
    obj = JSON.load(json, p)
    assert_equal({ 'a' => 1, 'b' => [true, false]}, obj)
    assert_equal([1, true, false, [true, false], { 'a' => 1, 'b' => [true, false]}], children)
  end

# []
  def test_bracket_load
    json = %{{"a":1,"b":[true,false]}}
    obj = JSON[json]
    assert_equal({ 'a' => 1, 'b' => [true, false]}, obj)
  end

  def test_bracket_dump
    json = JSON[[1, true, nil]]
    assert_equal(%{[1,true,null]}, json)
  end

# generate
  def test_generate
    json = JSON.generate({ 'a' => 1, 'b' => [true, false]})
    assert_equal(%{{"a":1,"b":[true,false]}}, json)
  end
  # TBD with options

# fast_generate
  def test_fast_generate
    json = JSON.generate({ 'a' => 1, 'b' => [true, false]})
    assert_equal(%{{"a":1,"b":[true,false]}}, json)
  end

# pretty_generate
  def test_fast_generate
    json = JSON.pretty_generate({ 'a' => 1, 'b' => [true, false]})
    assert_equal(%{{
  "a":1,
  "b":[
    true,
    false
  ]}}, json)
  end
  # TBD with options

# parse
  def test_parse
    json = %{{"a":1,"b":[true,false]}}
    obj = JSON.parse(json)
    assert_equal({ 'a' => 1, 'b' => [true, false]}, obj)
  end
  def test_parse_sym_names
    json = %{{"a":1,"b":[true,false]}}
    obj = JSON.parse(json, :symbolize_names => true)
    assert_equal({ :a => 1, :b => [true, false]}, obj)
  end
  def test_parse_additions
    jam = Jam.new(true, 58)
    json = Oj.dump(jam, :mode => :compat)
    puts json
    obj = JSON.parse(json)
    assert_equal(jam, obj)
    obj = JSON.parse(json, :create_additions => true)
    assert_equal(jam, obj)
    obj = JSON.parse(json, :create_additions => false)
    assert_equal({'json_class' => 'Jam', 'x' => true, 'y' => 58}, obj)
  end
  def test_parse_bang
    json = %{{"a":1,"b":[true,false]}}
    obj = JSON.parse!(json)
    assert_equal({ 'a' => 1, 'b' => [true, false]}, obj)
  end

# recurse_proc
  def test_recurse_proc
    children = []
    obj = JSON.recurse_proc({ 'a' => 1, 'b' => [true, false]}) { |x| children << x }
    assert_equal([1, true, false, [true, false], { 'a' => 1, 'b' => [true, false]}], children)
  end

end # Mimic
