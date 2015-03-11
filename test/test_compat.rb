#!/usr/bin/env ruby
# encoding: UTF-8

$: << File.dirname(__FILE__)

require 'helper'

class CompatJuice < Minitest::Test

  class Jeez
    attr_accessor :x, :y

    def initialize(x, y)
      @x = x
      @y = y
    end

    def eql?(o)
      self.class == o.class && @x == o.x && @y == o.y
    end
    alias == eql?

    def as_json()
      {"json_class" => self.class.to_s,"x" => @x,"y" => @y}
    end

    def self.json_create(h)
      self.new(h['x'], h['y'])
    end
  end # Jeez

  module One
    module Two
      module Three
        class Deep

          def initialize()
          end

          def eql?(o)
            self.class == o.class
          end
          alias == eql?

          def to_hash()
            {'json_class' => "#{self.class.name}"}
          end

          def to_json(*a)
            %{{"json_class":"#{self.class.name}"}}
          end

          def self.json_create(h)
            self.new()
          end
        end # Deep
      end # Three
    end # Two
  end # One

  def setup
    @default_options = Oj.default_options
  end

  def teardown
    Oj.default_options = @default_options
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
    dump_and_load(70.35, false)
    dump_and_load(-54321.012, false)
    dump_and_load(1.7775, false)
    dump_and_load(2.5024, false)
    dump_and_load(2.48e16, false)
    dump_and_load(2.48e100 * 1.0e10, false)
    dump_and_load(-2.48e100 * 1.0e10, false)
  end

  def test_string
    dump_and_load('', false)
    dump_and_load('abc', false)
    dump_and_load("abc\ndef", false)
    dump_and_load("a\u0041", false)
  end

  def test_encode
    opts = Oj.default_options
    Oj.default_options = { :ascii_only => false }
    unless 'jruby' == $ruby
      dump_and_load("ぴーたー", false)
    end
    Oj.default_options = { :ascii_only => true }
    json = Oj.dump("ぴーたー")
    assert_equal(%{"\\u3074\\u30fc\\u305f\\u30fc"}, json)
    unless 'jruby' == $ruby
      dump_and_load("ぴーたー", false)
    end
    Oj.default_options = opts
  end

  def test_unicode
    # hits the 3 normal ranges and one extended surrogate pair
    json = %{"\\u019f\\u05e9\\u3074\\ud834\\udd1e"}
    obj = Oj.load(json)
    json2 = Oj.dump(obj, :ascii_only => true)
    assert_equal(json, json2)
  end

  def test_array
    dump_and_load([], false)
    dump_and_load([true, false], false)
    dump_and_load(['a', 1, nil], false)
    dump_and_load([[nil]], false)
    dump_and_load([[nil], 58], false)
  end

  def test_array_deep
    dump_and_load([1,[2,[3,[4,[5,[6,[7,[8,[9,[10,[11,[12,[13,[14,[15,[16,[17,[18,[19,[20]]]]]]]]]]]]]]]]]]]], false)
  end

  # Hash
  def test_hash
    dump_and_load({}, false)
    dump_and_load({ 'true' => true, 'false' => false}, false)
    dump_and_load({ 'true' => true, 'array' => [], 'hash' => { }}, false)
  end

  def test_hash_deep
    dump_and_load({'1' => {
                      '2' => {
                        '3' => {
                          '4' => {
                            '5' => {
                              '6' => {
                                '7' => {
                                  '8' => {
                                    '9' => {
                                      '10' => {
                                        '11' => {
                                          '12' => {
                                            '13' => {
                                              '14' => {
                                                '15' => {
                                                  '16' => {
                                                    '17' => {
                                                      '18' => {
                                                        '19' => {
                                                          '20' => {}}}}}}}}}}}}}}}}}}}}}, false)
  end

  def test_hash_escaped_key
    json = %{{"a\nb":true,"c\td":false}}
    obj = Oj.compat_load(json)
    assert_equal({"a\nb" => true, "c\td" => false}, obj)
  end

  def test_bignum_object
    dump_and_load(7 ** 55, false)
  end

  # BigDecimal
  def test_bigdecimal_compat
    dump_and_load(BigDecimal.new('3.14159265358979323846'), false)
  end

  def test_bigdecimal_load
    orig = BigDecimal.new('80.51')
    json = Oj.dump(orig, :mode => :compat, :bigdecimal_as_decimal => true)
    bg = Oj.load(json, :mode => :compat, :bigdecimal_load => true)
    assert_equal(BigDecimal, bg.class)
    assert_equal(orig, bg)
  end

  # Time
  def test_time_ruby
    if RUBY_VERSION.start_with?('1.8')
      t = Time.parse('2015-01-05T21:37:07.123456-08:00')
    else
      t = Time.new(2015, 1, 5, 21, 37, 7.123456, -8 * 3600)
    end
    expect = '"' + t.to_s + '"'
    json = Oj.dump(t, :mode => :compat, :time_format => :ruby)
    assert_equal(expect, json)
  end
  def test_time_xml
    if RUBY_VERSION.start_with?('1.8')
      t = Time.parse('2015-01-05T21:37:07.123456-08:00')
    else
      t = Time.new(2015, 1, 5, 21, 37, 7.123456, -8 * 3600)
    end
    json = Oj.dump(t, :mode => :compat, :time_format => :xmlschema, :second_precision => 6)
    assert_equal('"2015-01-05T21:37:07.123456-08:00"', json)
  end
  unless RUBY_VERSION.start_with?('1.8')
    def test_time_xml_12345
      t = Time.new(2015, 1, 5, 21, 37, 7.123456, 12345)
      json = Oj.dump(t, :mode => :compat, :time_format => :xmlschema, :second_precision => 6)
      assert_equal('"2015-01-05T21:37:07.123456+03:25"', json)
    end
  end
  def test_time_unix
    if RUBY_VERSION.start_with?('1.8')
      t = Time.parse('2015-01-05T21:37:07.123456-08:00')
    else
      t = Time.new(2015, 1, 5, 21, 37, 7.123456, -8 * 3600)
    end
    json = Oj.dump(t, :mode => :compat, :time_format => :unix, :second_precision => 6)
    assert_equal('1420522627.123456', json)
  end
  def test_time_unix_zone
    if RUBY_VERSION.start_with?('1.8')
      t = Time.parse('2015-01-05T21:37:07.123456-08:00')
    else
      t = Time.new(2015, 1, 5, 21, 37, 7.123456, -8 * 3600)
    end
    json = Oj.dump(t, :mode => :compat, :time_format => :unix_zone, :second_precision => 6)
    assert_equal('1420522627.123456e-28800', json)
  end
  unless RUBY_VERSION.start_with?('1.8')
    def test_time_unix_zone_12345
      t = Time.new(2015, 1, 5, 21, 37, 7.123456, 12345)
      json = Oj.dump(t, :mode => :compat, :time_format => :unix_zone, :second_precision => 6)
      assert_equal('1420481482.123456e12345', json)
    end
  end
  def test_time_unix_zone_early
    if RUBY_VERSION.start_with?('1.8')
      t = Time.parse('1954-01-05T21:37:07.123456-08:00')
    else
      t = Time.new(1954, 1, 5, 21, 37, 7.123456, -8 * 3600)
    end
    json = Oj.dump(t, :mode => :compat, :time_format => :unix_zone, :second_precision => 6)
    assert_equal('-504469372.876544e-28800', json)
  end

  # Stream IO
  def test_io_string
    json = %{{
  "x":true,
  "y":58,
  "z": [1,2,3]
}
}
    input = StringIO.new(json)
    obj = Oj.compat_load(input)
    assert_equal({ 'x' => true, 'y' => 58, 'z' => [1, 2, 3]}, obj)
  end

  def test_io_file
    filename = File.join(File.dirname(__FILE__), 'open_file_test.json')
    File.open(filename, 'w') { |f| f.write(%{{
  "x":true,
  "y":58,
  "z": [1,2,3]
}
}) }
    f = File.new(filename)
    obj = Oj.compat_load(f)
    f.close()
    assert_equal({ 'x' => true, 'y' => 58, 'z' => [1, 2, 3]}, obj)
  end

  # symbol_keys option
  def test_symbol_keys
    json = %{{
  "x":true,
  "y":58,
  "z": [1,2,3]
}
}
    obj = Oj.compat_load(json, :symbol_keys => true)
    assert_equal({ :x => true, :y => 58, :z => [1, 2, 3]}, obj)
  end

  # comments
  def test_comment_slash
    json = %{{
  "x":true,//three
  "y":58,
  "z": [1,2,
3 // six
]}
}
    obj = Oj.compat_load(json)
    assert_equal({ 'x' => true, 'y' => 58, 'z' => [1, 2, 3]}, obj)
  end

  def test_comment_c
    json = %{{
  "x"/*one*/:/*two*/true,
  "y":58,
  "z": [1,2,3]}
}
    obj = Oj.compat_load(json)
    assert_equal({ 'x' => true, 'y' => 58, 'z' => [1, 2, 3]}, obj)
  end

  def test_comment
    json = %{{
  "x"/*one*/:/*two*/true,//three
  "y":58/*four*/,
  "z": [1,2/*five*/,
3 // six
]
}
}
    obj = Oj.compat_load(json)
    assert_equal({ 'x' => true, 'y' => 58, 'z' => [1, 2, 3]}, obj)
  end

  def test_json_object_compat
    obj = Jeez.new(true, 58)
    Oj.default_options = { :mode => :compat, :use_to_json => true }
    dump_and_load(obj, false)
  end

  def test_json_module_object
    obj = One::Two::Three::Deep.new()
    dump_and_load(obj, false)
  end

  def test_json_object_create_id
    expected = Jeez.new(true, 58)
    json = Oj.dump(expected, :indent => 2, :mode => :compat, :use_to_json => true)
    obj = Oj.compat_load(json)
    assert_equal(expected, obj)
  end

  def test_json_object_bad
    json = %{{"json_class":"CompatJuice::Junk","x":true}}
    begin
      Oj.compat_load(json)
    rescue Exception => e
      assert_equal("Oj::ParseError", e.class().name)
      return
    end
    assert(false, "*** expected an exception")
  end

  def test_json_object_create_cache
    expected = Jeez.new(true, 58)
    json = Oj.dump(expected, :indent => 2, :mode => :compat, :use_to_json => true)
    obj = Oj.compat_load(json, :class_cache => true)
    assert_equal(expected, obj)
    obj = Oj.compat_load(json, :class_cache => false)
    assert_equal(expected, obj)
  end

  def test_json_object_create_id_other
    expected = Jeez.new(true, 58)
    json = Oj.dump(expected, :indent => 2, :mode => :compat, :use_to_json => true)
    json.gsub!('json_class', '_class_')
    obj = Oj.compat_load(json, :create_id => "_class_")
    assert_equal(expected, obj)
  end

  def test_json_object_create_deep
    expected = One::Two::Three::Deep.new()
    json = Oj.dump(expected, :indent => 2, :mode => :compat)
    obj = Oj.compat_load(json)
    assert_equal(expected, obj)
  end

  def dump_and_load(obj, trace=false)
    json = Oj.dump(obj, :indent => 2, :mode => :compat)
    puts json if trace
    loaded = Oj.compat_load(json);
    assert_equal(obj, loaded)
    loaded
  end

end
