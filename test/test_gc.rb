# encoding: UTF-8

require 'helper'

class GCTest < Minitest::Test
  class Goo
    attr_accessor :x, :child

    def initialize(x, child)
      @x = x
      @child = child
    end

    def to_hash()
      { 'json_class' => "#{self.class}", 'x' => x, 'child' => child }
    end

    def self.json_create(h)
      GC.start
      self.new(h['x'], h['child'])
    end
  end # Goo

  def around
    opts = Oj.default_options
    yield
    Oj.default_options = opts
  end

  # if no crash then the GC marking is working
  def test_parse_compat_gc
    g = Goo.new(0, nil)
    100.times { |i| g = Goo.new(i, g) }
    json = Oj.dump(g, :mode => :compat)
    Oj.compat_load(json)
  end

  def test_parse_object_gc
    g = Goo.new(0, nil)
    100.times { |i| g = Goo.new(i, g) }
    json = Oj.dump(g, :mode => :object)
    Oj.object_load(json)
  end
end
