# encoding: UTF-8

require 'helper'

class DebJuice < Minitest::Test
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
