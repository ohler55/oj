#!/usr/bin/env ruby
# frozen_string_literal: true

$LOAD_PATH << __dir__

require 'helper'

class GCTest < Minitest::Test
  class Goo
    attr_accessor :x, :child

    def initialize(x, child)
      @x = x
      @child = child
    end

    def to_hash
      { 'json_class' => self.class.to_s, 'x' => x, 'child' => child }
    end

    def self.json_create(h)
      GC.start
      new(h['x'], h['child'])
    end
  end # Goo

  def setup
    @default_options = Oj.default_options
    GC.stress = true
  end

  def teardown
    Oj.default_options = @default_options
    GC.stress = false
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

  def test_parse_gc
    json = '{"a":"Alpha","b":true,"c":12345,"d":[true,[false,[-123456789,null],3.9676,["Something else.",false],null]],"e":{"zero":null,"one":1,"two":2,"three":[3],"four":[0,1,2,3,4]},"f":null,"h":{"a":{"b":{"c":{"d":{"e":{"f":{"g":null}}}}}}},"i":[[[[[[[null]]]]]]]}'

    50.times do
      data = Oj.load(json)
      assert_equal(json, Oj.dump(data))
    end
  end

  def test_load_non_utf8_input_gc
    [['x', Encoding::ASCII_8BIT, [30, 60, 90]], ['é', Encoding::ISO_8859_1, [1000]]].each do |c, enc, sizes|
      sizes.each do |n|
        s = c * n
        json = %({"a":"#{s}","b":#{n}}).encode(enc)
        assert_equal({ 'a' => s, 'b' => n }, Oj.load(json, mode: :compat))
        assert_equal({ 'a' => s, 'b' => n }, Oj.safe_load(StringIO.new(json)))

        bad = %({"a":"#{s}","b":#{n},})
        err = assert_raises(EncodingError) { Oj.load(bad.encode(enc), mode: :compat) }
        assert_includes(err.message, "in '#{bad}")
      end
    end
  end

  def test_load_to_s_input_gc
    values = (1..80).map { |i| "value#{i}" }
    json = Oj.dump({ 'a' => values }, mode: :compat)
    src = Object.new
    src.define_singleton_method(:to_s) { json + '' }
    assert_equal({ 'a' => values }, Oj.load(src, mode: :compat))
  end

  def test_dump_non_utf8_string_gc
    s = 'é' * 3000
    expected = Oj.dump({ 'k' => s }, mode: :compat)
    h = { 'k' => s.encode(Encoding::ISO_8859_1) }
    5.times { assert_equal(expected, Oj.dump(h, mode: :compat)) }
  end
end
