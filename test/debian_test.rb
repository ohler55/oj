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

$ruby = RUBY_DESCRIPTION.split(' ')[0]
$ruby = 'ree' if 'ruby' == $ruby && RUBY_DESCRIPTION.include?('Ruby Enterprise Edition')

def hash_eql(h1, h2)
  return false if h1.size != h2.size
  h1.keys.each do |k|
    return false unless h1[k] == h2[k]
  end
  true
end

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

end# Jam

# contributed by sauliusg to fix as_json
class Orange < Jam
  def initialize(x, y)
    super
  end

  def as_json()
    puts "Orange.as_json called"
    { :json_class => self.class,
      :x => @x,
      :y => @y }
  end

  def self.json_create(h)
    puts "Orange.json_create"
    self.new(h['x'], h['y'])
  end
end

class DebJuice < ::Test::Unit::TestCase

  def test_class_hash
    Oj.hash_test()
  end

  def test_as_json_object_compat_hash_cached
    Oj.default_options = { :mode => :compat, :class_cache => true }
    obj = Orange.new(true, 58)
    puts "dumping compat with cache"
    json = Oj.dump(obj, :indent => 2)
    assert(!json.nil?)
    dump_and_load(obj, true)
  end

  def dump_and_load(obj, trace=false)
    puts "dumping"
    json = Oj.dump(obj, :indent => 2)
    puts json if trace
    puts "loading"
    loaded = Oj.load(json);
    puts "done"
    assert_equal(obj, loaded)
    loaded
  end

end
