#!/usr/bin/env ruby
# frozen_string_literal: true

$LOAD_PATH << __dir__
@oj_dir = File.dirname(File.expand_path(__dir__))
%w(lib ext).each do |dir|
  $LOAD_PATH << File.join(@oj_dir, dir)
end

require 'minitest'
require 'minitest/autorun'
require 'stringio'
require 'date'
require 'bigdecimal'
require 'weakref'
require 'oj'

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

    def to_json(*_a)
      %|{"json_class":"#{self.class}","x":#{@x},"y":#{@y}}|
    end

    def self.json_create(h)
      new(h['x'], h['y'])
    end
  end # Jeez

  class Argy

    def to_json(*a)
      %|{"args":"#{a}"}|
    end
  end # Argy

  class Rex
    attr_accessor :s

    def self.json_create(s)
      new(s)
    end

    def initialize(s)
      @s = s
    end
  end # Rex

  class Stringy

    def to_s
      %|[1,2]|
    end
  end # Stringy

  module One
    module Two
      module Three
        class Deep

          def eql?(o)
            self.class == o.class
          end
          alias == eql?

          def to_json(*_a)
            %|{"json_class":"#{self.class.name}"}|
          end

          def self.json_create(_h)
            new()
          end
        end # Deep
      end # Three
    end # Two
  end # One

  def setup
    @default_options = Oj.default_options
    # in compat mode other options other than the JSON gem globals and options
    # are not used.
    Oj.default_options = { :mode => :compat }
  end

  def teardown
    Oj.default_options = @default_options
    # Oj.default_options does not report :match_string so restoring the saved
    # options leaves any regexps a test set in place. Clear them explicitly.
    Oj.default_options = {:match_string => {}}
  end

  # Overwrite the machine stack before collecting. Without this a stale
  # reference left on the stack can keep an object alive and hide a missing
  # mark.
  def scrub_stack(depth = 128)
    return 0 if 0 == depth

    pad = 'x' * 64
    scrub_stack(depth - 1) + pad.size
  end

  def collect_garbage
    scrub_stack
    4.times { GC.start(full_mark: true, immediate_sweep: true) }
  end

  # A WeakRef reports whether the C level mark kept the object alive on CRuby.
  # Other engines manage the objects a C extension holds differently so the
  # tests below check that the option still works instead.
  def assert_marked(ref, msg)
    assert(ref.weakref_alive?, msg) if 'ruby' == RUBY_ENGINE
  end

  # The default options hold the classes of the :hash_class, :array_class and
  # :ignore options and the regexps of the :match_string option. Nothing else
  # refers to them so they are collected unless the defaults are marked.

  def test_default_options_marks_hash_class
    klass = Class.new(Hash)
    Oj.default_options = {:hash_class => klass}
    ref = WeakRef.new(klass)
    klass = nil
    collect_garbage

    assert_marked(ref, 'the hash_class of the defaults was collected')
    loaded = Oj.load('{"a":1}')
    assert_equal(Oj.default_options[:hash_class], loaded.class)
    assert_equal({'a' => 1}, loaded.to_h)
  end

  def test_default_options_marks_ignore
    klass = Class.new
    Oj.default_options = {:ignore => [klass]}
    ref = WeakRef.new(klass)
    klass = nil
    collect_garbage

    assert_marked(ref, 'a class of the ignore option of the defaults was collected')
    ignore = Oj.default_options[:ignore]
    assert_equal(1, ignore.size)
    assert_kind_of(Class, ignore[0])
  end

  def test_default_options_marks_match_string
    # The regexp is not kept in a local variable so the regexps of the defaults
    # are the only reference to it. Matching calls it, so if it is collected the
    # load below reads a freed object.
    Oj.default_options = {:create_additions => true, :match_string => {Regexp.new('^z') => Rex}}
    collect_garbage

    obj = Oj.load('"zzz"')
    assert_equal(Rex, obj.class, 'the regexp of the match_string option of the defaults was collected')
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
    dump_and_load(12_345, false)
    dump_and_load(-54_321, false)
    dump_and_load(1, false)
  end

  def test_fixnum_array
    data = (1..1000).to_a
    json = Oj.dump(data, mode: :compat)
    assert_equal("[#{data.join(',')}]", json)
  end

  def test_float
    dump_and_load(0.0, false)
    dump_and_load(0.56, false)
    dump_and_load(3.0, false)
    dump_and_load(12_345.6789, false)
    dump_and_load(70.35, false)
    dump_and_load(-54_321.012, false)
    dump_and_load(1.7775, false)
    dump_and_load(2.5024, false)
    dump_and_load(2.48e16, false)
    dump_and_load(2.48e100 * 1.0e10, false)
    dump_and_load(-2.48e100 * 1.0e10, false)
    dump_and_load(1_405_460_727.723866, false)
    dump_and_load(0.5773, false)
    dump_and_load(0.6768, false)
    dump_and_load(0.685, false)
    dump_and_load(0.7032, false)
    dump_and_load(0.7051, false)
    dump_and_load(0.8274, false)
    dump_and_load(0.9149, false)
    dump_and_load(64.4, false)
    dump_and_load(71.6, false)
    dump_and_load(73.4, false)
    dump_and_load(80.6, false)
    dump_and_load(-95.640172, false)
  end

  def test_string
    dump_and_load('', false)
    dump_and_load('abc', false)
    dump_and_load("abc\ndef", false)
    dump_and_load("a\u0041", false)
  end

  def test_encode
    opts = Oj.default_options
    Oj.default_options = { :ascii_only => true }
    json = Oj.dump('ぴーたー')
    assert_equal(%{"\\u3074\\u30fc\\u305f\\u30fc"}, json)
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
    dump_and_load([1, [2, [3, [4, [5, [6, [7, [8, [9, [10, [11, [12, [13, [14, [15, [16, [17, [18, [19, [20]]]]]]]]]]]]]]]]]]]], false)
  end

  # A deeply nested array dumped in compat mode with an integer :indent used to
  # overflow the output buffer because the integer-indent branch of dump_array
  # called fill_indent() without a preceding assure_size(). Any nesting deep
  # enough that depth*indent exceeds the buffer slack corrupted the heap.
  def test_array_deep_indent
    [1, 2, 16].each do |indent|
      [50, 120, 250].each do |depth|
        nested = eval('[[' * depth + '1' + ']]' * depth)
        json   = Oj.dump(nested, :mode => :compat, :indent => indent)
        # round-trips back to the same structure and does not crash
        assert_equal(nested, Oj.compat_load(json))
      end
    end
  end

  def test_symbol
    json = Oj.dump(:abc, :mode => :compat)
    assert_equal('"abc"', json)
  end

  def test_time_xml_schema
    t = Time.xmlschema('2012-01-05T23:58:07.123456000+09:00')
    # t = Time.local(2012, 1, 5, 23, 58, 7, 123456)
    json = Oj.dump(t, :mode => :compat)
    assert_equal(%{"2012-01-05 23:58:07 +0900"}, json)
  end

  def test_class
    json = Oj.dump(CompatJuice, :mode => :compat)
    assert_equal(%{"CompatJuice"}, json)
  end

  def test_module
    json = Oj.dump(One::Two, :mode => :compat)
    assert_equal(%{"CompatJuice::One::Two"}, json)
  end

  # Hash
  def test_non_str_hash
    json = Oj.dump({ 1 => true, 0 => false }, :mode => :compat)
    h = Oj.load(json, :mode => :strict)
    assert_equal({ '1' => true, '0' => false }, h)
  end

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

  def test_invalid_escapes_handled
    json = '{"subtext":"\"404er\” \w \k \3 \a"}'
    obj = Oj.compat_load(json)
    assert_equal({'subtext' => '"404er” w k 3 a'}, obj)
  end

  def test_hash_escaping
    json = Oj.to_json({'<>' => '<>'}, mode: :compat)
    assert_equal('{"<>":"<>"}', json)
  end

  def test_bignum_object
    dump_and_load(7 ** 55, false)
  end

  def test_json_object
    obj = Jeez.new(true, 58)
    json = Oj.to_json(obj)
    assert(%|{"json_class":"CompatJuice::Jeez","x":true,"y":58}| == json ||
          %|{"json_class":"CompatJuice::Jeez","y":58,"x":true}| == json)
    dump_to_json_and_load(obj, false)
  end

  def test_json_object_create_id
    Oj.default_options = { :create_id => 'kson_class', :create_additions => true}
    expected = Jeez.new(true, 58)
    json = %{{"kson_class":"CompatJuice::Jeez","x":true,"y":58}}
    obj = Oj.load(json)
    assert_equal(expected, obj)
    Oj.default_options = { :create_id => 'json_class' }
  end

  def test_bignum_compat
    json = Oj.dump(7 ** 55, :mode => :compat)
    b = Oj.load(json, :mode => :strict)
    assert_equal(30_226_801_971_775_055_948_247_051_683_954_096_612_865_741_943, b)
  end

  # BigDecimal
  def test_bigdecimal
    # BigDecimals are dumped as strings and can not be restored to the
    # original value without using an undocumented feature of the JSON gem.
    json = Oj.dump(BigDecimal('3.14159265358979323846'))
    # 2.4.0 changes the exponent to lowercase
    assert_equal('"0.314159265358979323846e1"', json.downcase)
  end

  def test_decimal_class
    big = BigDecimal('3.14159265358979323846')
    # :decimal_class is the undocumented feature.
    json = Oj.load('3.14159265358979323846', mode: :compat, decimal_class: BigDecimal)
    assert_equal(big, json)
  end

  def test_infinity
    assert_raises(Oj::ParseError) { Oj.load('Infinity', :mode => :strict) }
    x = Oj.load('Infinity', :mode => :compat)
    assert_equal('Infinity', x.to_s)
  end

  # Time
  def test_time_from_time_object
    t = Time.new(2015, 1, 5, 21, 37, 7.123456, -8 * 3600)
    expect = '"' + t.to_s + '"'
    json = Oj.dump(t)
    assert_equal(expect, json)
  end

  def test_date_compat
    orig = Date.new(2012, 6, 19)
    json = Oj.dump(orig, :mode => :compat)
    x = Oj.load(json, :mode => :compat)
    # Some Rubies implement Date as data and some as a real Object. Either are
    # okay for the test.
    if x.is_a?(String)
      assert_equal(orig.to_s, x)
    else # better be a Hash
      assert_equal({'year' => orig.year, 'month' => orig.month, 'day' => orig.day, 'start' => orig.start}, x)
    end
  end

  def test_datetime_compat
    orig = DateTime.new(2012, 6, 19, 20, 19, 27)
    json = Oj.dump(orig, :mode => :compat)
    x = Oj.load(json, :mode => :compat)
    # Some Rubies implement Date as data and some as a real Object. Either are
    # okay for the test.
    assert_equal(orig.to_s, x)
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
    filename = File.join(__dir__, 'open_file_test.json')
    File.write(filename, %{{
  "x":true,
  "y":58,
  "z": [1,2,3]
}
})
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

  # If mimic_JSON has not been called then Oj.dump will call to_json on the
  # top level object only.
  def test_json_object_top
    obj = Jeez.new(true, 58)
    dump_to_json_and_load(obj, false)
  end

  # A child to_json should not be called.
  def test_json_object_child
    obj = { 'child' => Jeez.new(true, 58) }
    assert_equal('{"child":{"json_class":"CompatJuice::Jeez","x":true,"y":58}}', Oj.dump(obj))
  end

  def test_json_module_object
    obj = One::Two::Three::Deep.new()
    dump_to_json_and_load(obj, false)
  end

  def test_json_object_dump_create_id
    expected = Jeez.new(true, 58)
    json = Oj.to_json(expected)
    obj = Oj.compat_load(json, :create_additions => true)
    assert_equal(expected, obj)
  end

  def test_json_object_bad
    json = %{{"json_class":"CompatJuice::Junk","x":true}}
    begin
      Oj.compat_load(json, :create_additions => true)
    rescue Exception => e
      assert_equal('ArgumentError', e.class().name)
      return
    end
    assert(false, '*** expected an exception')
  end

  def test_json_object_create_cache
    expected = Jeez.new(true, 58)
    json = Oj.to_json(expected)
    obj = Oj.compat_load(json, :class_cache => true, :create_additions => true)
    assert_equal(expected, obj)
    obj = Oj.compat_load(json, :class_cache => false, :create_additions => true)
    assert_equal(expected, obj)
  end

  def test_json_object_create_id_other
    expected = Jeez.new(true, 58)
    json = Oj.to_json(expected)
    json.gsub!('json_class', '_class_')
    obj = Oj.compat_load(json, :create_id => '_class_', :create_additions => true)
    assert_equal(expected, obj)
  end

  def test_json_object_create_deep
    expected = One::Two::Three::Deep.new()
    json = Oj.to_json(expected)
    obj = Oj.compat_load(json, :create_additions => true)
    assert_equal(expected, obj)
  end

  def test_range
    json = Oj.dump(1..7)
    assert_equal('"1..7"', json)
  end

  def test_arg_passing
    json = Oj.to_json(Argy.new(), :max_nesting => 40)
    assert_match(/.*max_nesting.*40.*/, json)
  end

  def test_max_nesting
    assert_raises() { Oj.to_json([[[[[]]]]], :max_nesting => 3) }
    assert_raises() { Oj.dump([[[[[]]]]], :max_nesting => 3, :mode=>:compat) }

    assert_raises() { Oj.to_json([[]], :max_nesting => 1) }
    assert_equal('[[]]', Oj.to_json([[]], :max_nesting => 2))

    assert_raises() { Oj.dump([[]], :max_nesting => 1, :mode=>:compat) }
    assert_equal('[[]]', Oj.dump([[]], :max_nesting => 2, :mode=>:compat))

    assert_raises() { Oj.to_json([[3]], :max_nesting => 1) }
    assert_equal('[[3]]', Oj.to_json([[3]], :max_nesting => 2))

    assert_raises() { Oj.dump([[3]], :max_nesting => 1, :mode=>:compat) }
    assert_equal('[[3]]', Oj.dump([[3]], :max_nesting => 2, :mode=>:compat))

  end

  def test_bad_unicode
    assert_raises() { Oj.to_json("\xE4xy") }
  end

  def test_bad_unicode_e2
    assert_raises() { Oj.to_json("L\xE2m ") }
  end

  def test_bad_unicode_start
    assert_raises() { Oj.to_json("\x8abc") }
  end

  def test_parse_to_s
    s = Stringy.new
    assert_equal([1, 2], Oj.load(s, :mode => :compat))
  end

  def test_parse_large_string
    error = assert_raises() { Oj.load(%|{"a":"aaaaaaaaaa\0aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}|) }
    assert_includes(error.message, 'NULL byte in string')

    error = assert_raises() { Oj.load(%|{"a":"aaaaaaaaaaaaaaaaaaaa                       }|) }
    assert_includes(error.message, 'quoted string not terminated')

    json =<<~JSON
      {
        "a": "\\u3074\\u30fc\\u305f\\u30fc",
        "b": "aaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      }
    JSON
    assert_equal('ぴーたー', Oj.load(json)['a'])
  end

  def test_parse_large_escaped_string
    invalid_json = %|{"a":"aaaa\\nbbbb\\rcccc\\tddd\\feee\\bf/\\\\\\u3074\\u30fc\\u305f\\u30fc                             }|
    error = assert_raises() { Oj.load(invalid_json) }
    assert_includes(error.message, 'quoted string not terminated')

    json = '"aaaa\\nbbbb\\rcccc\\tddd\\feee\\bf/\\\\\\u3074\\u30fc\\u305f\\u30fc             "'
    assert_equal("aaaa\nbbbb\rcccc\tddd\feee\bf/\\ぴーたー             ", Oj.load(json))
  end

  def test_invalid_to_s
    obj = Object.new
    def obj.to_s
      nil
    end

    assert_raises(TypeError) { Oj.dump(obj, mode: :compat) }
  end

  def dump_and_load(obj, trace=false)
    json = Oj.dump(obj)
    puts json if trace
    loaded = Oj.compat_load(json, :create_additions => true)
    if obj.nil?
      assert_nil(loaded)
    else
      assert_equal(obj, loaded)
    end
    loaded
  end

  def dump_to_json_and_load(obj, trace=false)
    json = Oj.to_json(obj, :indent => '  ')
    puts json if trace
    loaded = Oj.compat_load(json, :create_additions => true)
    if obj.nil?
      assert_nil(loaded)
    else
      assert_equal(obj, loaded)
    end
    loaded
  end

end
