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
require 'oj'

$ruby = RUBY_DESCRIPTION.split(' ')[0]
$ruby = 'ree' if 'ruby' == $ruby && RUBY_DESCRIPTION.include?('Ruby Enterprise Edition')

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
    Oj.mimic_JSON # TBD
    children = []
    json = %{{"a":1,"b":[true,false]}}
    if 'rubinius' == $ruby || 'jruby' == $ruby || '1.8.7' == RUBY_VERSION
      obj = JSON.load(json) {|x| children << x }
    else
      p = Proc.new {|x| children << x }
      obj = JSON.load(json, p)
    end
    assert_equal({ 'a' => 1, 'b' => [true, false]}, obj)
    # JRuby 1.7.0 rb_yield() is broken and converts the [true, falser] array into true
    unless 'jruby' == $ruby && '1.9.3' == RUBY_VERSION
      assert([1, true, false, [true, false], { 'a' => 1, 'b' => [true, false]}] == children ||
             [true, false, [true, false], 1, { 'a' => 1, 'b' => [true, false]}] == children,
             "children don't match")
    end
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
    assert(%{{"a":1,"b":[true,false]}} == json ||
           %{{"b":[true,false],"a":1}} == json)
  end
  def test_generate_options
    json = JSON.generate({ 'a' => 1, 'b' => [true, false]},
                         :indent => "--",
                         :array_nl => "\n",
                         :object_nl => "#\n",
                         :space => "*",
                         :space_before => "~")
    assert(%{{#
--"a"~:*1,#
--"b"~:*[
----true,
----false
--]#
}} == json ||
%{{#
--"b"~:*[
----true,
----false
--],#
--"a"~:*1#
}} == json)

  end

# fast_generate
  def test_fast_generate
    json = JSON.generate({ 'a' => 1, 'b' => [true, false]})
    assert(%{{"a":1,"b":[true,false]}} == json ||
           %{{"b":[true,false],"a":1}} == json)
  end

# pretty_generate
  def test_pretty_generate
    json = JSON.pretty_generate({ 'a' => 1, 'b' => [true, false]})
    assert(%{{
  "a" : 1,
  "b" : [
    true,
    false
  ]
}} == json ||
%{{
  "b" : [
    true,
    false
  ],
  "a" : 1
}} == json)
  end

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
    obj = JSON.parse(json)
    assert_equal(jam, obj)
    obj = JSON.parse(json, :create_additions => true)
    assert_equal(jam, obj)
    obj = JSON.parse(json, :create_additions => false)
    assert_equal({'json_class' => 'Jam', 'x' => true, 'y' => 58}, obj)
    json.gsub!('json_class', 'kson_class')
    JSON.create_id = 'kson_class'
    obj = JSON.parse(json, :create_additions => true)
    JSON.create_id = 'json_class'
    assert_equal(jam, obj)
  end
  def test_parse_bang
    json = %{{"a":1,"b":[true,false]}}
    obj = JSON.parse!(json)
    assert_equal({ 'a' => 1, 'b' => [true, false]}, obj)
  end

# recurse_proc
  def test_recurse_proc
    children = []
    JSON.recurse_proc({ 'a' => 1, 'b' => [true, false]}) { |x| children << x }
    # JRuby 1.7.0 rb_yield() is broken and converts the [true, falser] array into true
    unless 'jruby' == $ruby && '1.9.3' == RUBY_VERSION
      assert([1, true, false, [true, false], { 'a' => 1, 'b' => [true, false]}] == children ||
             [true, false, [true, false], 1, { 'b' => [true, false], 'a' => 1}] == children)
    end
  end

end # Mimic
