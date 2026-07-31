#!/usr/bin/env ruby
# frozen_string_literal: true

$LOAD_PATH << __dir__

require 'helper'

class FileJuice < Minitest::Test
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

  end # Jam

  class Jeez < Jam

    def to_json(*_args)
      %{{"json_class":"#{self.class}","x":#{@x},"y":#{@y}}}
    end

    def self.json_create(h)
      new(h['x'], h['y'])
    end
  end # Jeez

  class Orange < Jam

    def as_json
      { :json_class => self.class,
        :x => @x,
        :y => @y }
    end

    def self.json_create(h)
      new(h['x'], h['y'])
    end
  end

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
    dump_and_load(12_345, false)
    dump_and_load(-54_321, false)
    dump_and_load(1, false)
  end

  def test_float
    mode = Oj.default_options()[:mode]
    Oj.default_options = {:mode => :object}
    dump_and_load(0.0, false)
    dump_and_load(12_345.6789, false)
    dump_and_load(70.35, false)
    dump_and_load(-54_321.012, false)
    dump_and_load(1.7775, false)
    dump_and_load(2.5024, false)
    dump_and_load(2.48e16, false)
    dump_and_load(2.48e100 * 1.0e10, false)
    dump_and_load(-2.48e100 * 1.0e10, false)
    dump_and_load(1/0.0, false)
    Oj.default_options = {:mode => mode}
  end

  def test_string
    dump_and_load('', false)
    dump_and_load('abc', false)
    dump_and_load("abc\ndef", false)
    dump_and_load("a\u0041", false)
    assert_equal("a\u0000a", dump_and_load("a\u0000a", false))
  end

  def test_string_object
    dump_and_load('abc', false)
    dump_and_load(':abc', false)
  end

  def test_array
    dump_and_load([], false)
    dump_and_load([true, false], false)
    dump_and_load(['a', 1, nil], false)
    dump_and_load([[nil]], false)
    dump_and_load([[nil], 58], false)
  end

  # Symbol
  def test_symbol_object
    Oj.default_options = { :mode => :object }
    # dump_and_load(''.to_sym, false)
    dump_and_load(:abc, false)
    dump_and_load(:":xyz", false)
  end

  # Time
  def test_time_object
    t = Time.now()
    Oj.default_options = { :mode => :object, :time_format => :unix_zone }
    dump_and_load(t, false)
  end

  def test_time_object_early
    t = Time.xmlschema('1954-01-05T00:00:00.123456')
    Oj.default_options = { :mode => :object, :time_format => :unix_zone }
    dump_and_load(t, false)
  end

  # Hash
  def test_hash
    Oj.default_options = { :mode => :strict }
    dump_and_load({}, false)
    dump_and_load({ 'true' => true, 'false' => false}, false)
    dump_and_load({ 'true' => true, 'array' => [], 'hash' => { }}, false)
  end

  # Object with to_json()
  def test_json_object_compat
    Oj.default_options = { :mode => :compat, :use_to_json => true, :create_additions => true }
    obj = Jeez.new(true, 58)
    json = Oj.dump(obj, :indent => 2)
    assert(%{{"json_class":"FileJuice::Jeez","x":true,"y":58}
} == json ||
           %{{"json_class":"FileJuice::Jeez","y":58,"x":true}
} == json)
    dump_and_load(obj, false)
    Oj.default_options = { :mode => :compat, :use_to_json => false }
  end

  # Range
  def test_range_object
    Oj.default_options = { :mode => :object }
    json = Oj.dump(1..7, :mode => :object, :indent => 0)
    if $ruby == 'ruby'
      assert_equal(%{{"^u":["Range",1,7,false]}}, json)
    else
      assert_equal(%{{"^O":"Range","begin":1,"end":7,"exclude_end?":false}}, json)
    end
    dump_and_load(1..7, false)
    dump_and_load(1..1, false)
    dump_and_load(1...7, false)
  end

  # BigNum
  def test_bignum_object
    Oj.default_options = { :mode => :compat }
    dump_and_load(7 ** 55, false)
  end

  # BigDecimal
  def test_bigdecimal_strict
    mode = Oj.default_options[:mode]
    Oj.default_options = {:mode => :strict}
    dump_and_load(BigDecimal('3.14159265358979323846'), false)
    Oj.default_options = {:mode => mode}
  end

  def test_bigdecimal_null
    mode = Oj.default_options[:mode]
    Oj.default_options = {:mode => :null}
    dump_and_load(BigDecimal('3.14159265358979323846'), false)
    Oj.default_options = {:mode => mode}
  end

  def test_bigdecimal_object
    Oj.default_options = {:mode => :object}
    dump_and_load(BigDecimal('3.14159265358979323846'), false)
  end

  # Date
  def test_date_object
    Oj.default_options = { :mode => :object }
    dump_and_load(Date.new(2012, 6, 19), false)
  end

  # DateTime
  def test_datetime_object
    Oj.default_options = { :mode => :object }
    dump_and_load(DateTime.new(2012, 6, 19), false)
  end

  def test_load_unicode_path
    json =<<~JSON
      {
        "x":true,
        "y":58,
        "z": [1,2,3]
      }
    JSON

    Tempfile.create('file_test_conceição1.json') do |f|
      f.write(json)
      f.close

      objects = Oj.load_file(f.path)
      assert_equal(Oj.load(json), objects)
    end
  end

  # An escaped key is decoded into a buffer of its own rather than being used
  # in place, so the copy it is kept in has to be freed with the Val.
  def test_escaped_keys_in_a_file
    count = 200
    json = '{' + (0...count).map { |i| %("k\\u0041ey#{i}":#{i}) }.join(',') + '}'

    Tempfile.create('file_test_escaped.json') do |f|
      f.write(json)
      f.close

      loaded = Oj.load_file(f.path, :mode => :object)
      assert_equal(count, loaded.size)
      assert_equal('kAey0', loaded.keys.first)
      assert_equal(count - 1, loaded["kAey#{count - 1}"])
    end
  end

  # A token that is being protected from the head of the reader buffer gave
  # oj_reader_read() a shift of -1, and the shift is unsigned, so instead of
  # growing the buffer it pushed tail past end.
  def test_long_number_at_the_start_of_a_file
    [4093, 100_000].each do |digits|
      json = '1' * digits

      Tempfile.create('file_test_bignum.json') do |f|
        f.write(json)
        f.close

        assert_equal(json.to_i, Oj.load_file(f.path), digits)
        assert_equal(json.to_i, File.open(f.path) { |io| Oj.load(io) }, digits)
      end
    end
  end

  # A key short enough to be kept in the Val itself points into the value
  # stack, which is moved when stack_push grows it.
  def test_deep_keys_survive_the_value_stack_growing
    depth = 10_000
    json = +''
    depth.times { |i| json << %({"key_at_level_#{i}":) }
    json << '1' << ('}' * depth)

    Tempfile.create('file_test_deep.json') do |f|
      f.write(json)
      f.close

      obj = Oj.load_file(f.path, :mode => :object)
      keys = []
      while obj.is_a?(Hash)
        keys << obj.keys.first
        obj = obj.values.first
      end
      assert_equal(depth, keys.size)
      assert_equal((0...depth).map { |i| "key_at_level_#{i}" }, keys)
      assert_equal(1, obj)
    end
  end

  # read() returning -1 was stored in a size_t, so the error looked like a
  # successful read of SIZE_MAX bytes and the uninitialized stack buffer was
  # parsed over and over.
  def test_parser_file_on_a_directory
    assert_raises(IOError) { Oj::Parser.new(:usual).file(__dir__) }
  end

  # Without validate_document_end() the Qundef the delegate uses as a place
  # holder was handed back to Ruby.
  def test_parser_file_on_a_truncated_document
    ['{', '{"a":', '{"a":1', '[', '[1,', '[1,2', 'nul', 'tru', 'fals'].each do |json|
      Tempfile.create('file_test_truncated.json') do |f|
        f.write(json)
        f.close

        assert_raises(EncodingError, json) { Oj::Parser.new(:usual).file(f.path) }
      end
    end
  end

  def dump_and_load(obj, trace=false)
    filename = File.join(__dir__, 'file_test.json')
    File.open(filename, 'w') { |f|
      Oj.to_stream(f, obj, :indent => 2)
    }
    puts "\n*** file: '#{File.read(filename)}'" if trace
    loaded = Oj.load_file(filename)
    if obj.is_a?(Time) && loaded.is_a?(Time)
      assert_equal(obj.tv_sec, loaded.tv_sec)
      if obj.respond_to?(:tv_nsec)
        assert_equal(obj.tv_nsec, loaded.tv_nsec)
      else
        assert_equal(obj.tv_usec, loaded.tv_usec)
      end
      assert_equal(obj.utc?, loaded.utc?)
      assert_equal(obj.utc_offset, loaded.utc_offset)
    elsif obj.nil?
      assert_nil(loaded)
    else
      assert_equal(obj, loaded)
    end
    loaded
  end

end
