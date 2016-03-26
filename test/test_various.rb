#!/usr/bin/env ruby
# encoding: UTF-8

$: << File.dirname(__FILE__)

require 'helper'

class Juice < Minitest::Test

  module TestModule
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

  class Jeez < Jam
    def initialize(x, y)
      super
    end

    def to_json()
      %{{"json_class":"#{self.class}","x":#{@x},"y":#{@y}}}
    end

    def self.json_create(h)
      self.new(h['x'], h['y'])
    end
  end# Jeez

  # contributed by sauliusg to fix as_json
  class Orange < Jam
    def initialize(x, y)
      super
    end

    def as_json()
      { :json_class => self.class,
        :x => @x,
        :y => @y }
    end

    def self.json_create(h)
      self.new(h['x'], h['y'])
    end
  end

  class Melon < Jam
    def initialize(x, y)
      super
    end

    def as_json()
      "#{x} #{y}"
    end

    def self.json_create(h)
      self.new(h['x'], h['y'])
    end
  end

  class Jazz < Jam
    def initialize(x, y)
      super
    end
    def to_hash()
      { 'json_class' => self.class.to_s, 'x' => @x, 'y' => @y }
    end
    def self.json_create(h)
      self.new(h['x'], h['y'])
    end
  end# Jazz

  def setup
    @default_options = Oj.default_options
  end

  def teardown
    Oj.default_options = @default_options
  end

=begin
  # Depending on the order the values may have changed. The set_options sets
  # should cover the function itself.
  def test_get_options
    opts = Oj.default_options()
    assert_equal({ :indent=>0,
                   :second_precision=>9,
                   :circular=>false,
                   :class_cache=>true,
                   :auto_define=>false,
                   :symbol_keys=>false,
                   :bigdecimal_as_decimal=>true,
                   :use_to_json=>true,
                   :nilnil=>false,
                   :allow_gc=>true,
                   :quirks_mode=>true,
                   :float_precision=>15,
                   :mode=>:object,
                   :escape_mode=>:json,
                   :time_format=>:unix_zone,
                   :bigdecimal_load=>:auto,
                   :create_id=>'json_class'}, opts)
  end
=end
  def test_set_options
    orig = Oj.default_options()
    alt ={
      :indent=>" - ",
      :second_precision=>5,
      :circular=>true,
      :class_cache=>false,
      :auto_define=>true,
      :symbol_keys=>true,
      :bigdecimal_as_decimal=>false,
      :use_to_json=>false,
      :nilnil=>true,
      :allow_gc=>false,
      :quirks_mode=>false,
      :float_precision=>13,
      :mode=>:strict,
      :escape_mode=>:ascii,
      :time_format=>:unix_zone,
      :bigdecimal_load=>:float,
      :create_id=>'classy',
      :space=>'z',
      :array_nl=>'a',
      :object_nl=>'o',
      :space_before=>'b',
      :nan=>:huge,
    }
    Oj.default_options = alt
    opts = Oj.default_options()
    assert_equal(alt, opts);

    Oj.default_options = orig # return to original
    # verify back to original
    opts = Oj.default_options()
    assert_equal(orig, opts);
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

  def test_float_parse
    Oj.default_options = { :float_precision => 16 }
    n = Oj.load('0.00001234567890123456')
    assert_equal(Float, n.class)
    assert_equal('1.234567890123456e-05', "%0.15e" % [n])

    n = Oj.load('-0.00001234567890123456')
    assert_equal(Float, n.class)
    assert_equal('-1.234567890123456e-05', "%0.15e" % [n])

    n = Oj.load('1000.0000123456789')
    assert_equal(BigDecimal, n.class)
    assert_equal('0.10000000123456789E4', n.to_s)

    n = Oj.load('-0.000012345678901234567')
    assert_equal(BigDecimal, n.class)
    assert_equal('-0.12345678901234567E-4', n.to_s)
  end

  def test_float_dump
    Oj.default_options = { :float_precision => 16 }
    assert_equal('1405460727.723866', Oj.dump(1405460727.723866))
    Oj.default_options = { :float_precision => 5 }
    assert_equal('1.4055', Oj.dump(1.405460727))
    Oj.default_options = { :float_precision => 0 }
    if RUBY_VERSION.start_with?('1.8')
      assert_equal('1405460727.72387', Oj.dump(1405460727.723866))
    else
      assert_equal('1405460727.723866', Oj.dump(1405460727.723866))
    end
    Oj.default_options = { :float_precision => 15 }
    assert_equal('0.56', Oj.dump(0.56))
    assert_equal('0.5773', Oj.dump(0.5773))
    assert_equal('0.6768', Oj.dump(0.6768))
    assert_equal('0.685', Oj.dump(0.685))
    assert_equal('0.7032', Oj.dump(0.7032))
    assert_equal('0.7051', Oj.dump(0.7051))
    assert_equal('0.8274', Oj.dump(0.8274))
    assert_equal('0.9149', Oj.dump(0.9149))
    assert_equal('64.4', Oj.dump(64.4))
    assert_equal('71.6', Oj.dump(71.6))
    assert_equal('73.4', Oj.dump(73.4))
    assert_equal('80.6', Oj.dump(80.6))
    assert_equal('-95.640172', Oj.dump(-95.640172))
  end

  def test_nan_dump
    assert_equal('null', Oj.dump(0/0.0, :mode => :strict, :nan => :null))
    assert_equal('NaN', Oj.dump(0/0.0, :mode => :strict, :nan => :word))
    assert_equal('3.3e14159265358979323846', Oj.dump(0/0.0, :mode => :strict, :nan => :huge))
  end

  def test_infinity_dump
    assert_equal('null', Oj.dump(1/0.0, :mode => :strict, :nan => :null))
    assert_equal('Infinity', Oj.dump(1/0.0, :mode => :strict, :nan => :word))
    assert_equal('3.0e14159265358979323846', Oj.dump(1/0.0, :mode => :strict, :nan => :huge))
  end

  def test_neg_infinity_dump
    assert_equal('null', Oj.dump(-1/0.0, :mode => :strict, :nan => :null))
    assert_equal('-Infinity', Oj.dump(-1/0.0, :mode => :strict, :nan => :word))
    assert_equal('-3.0e14159265358979323846', Oj.dump(-1/0.0, :mode => :strict, :nan => :huge))
  end

  def test_float
    mode = Oj.default_options()[:mode]
    Oj.default_options = {:mode => :object}
    dump_and_load(0.0, false)
    dump_and_load(12345.6789, false)
    dump_and_load(70.35, false)
    dump_and_load(-54321.012, false)
    dump_and_load(1.7775, false)
    dump_and_load(2.5024, false)
    dump_and_load(2.48e16, false)
    dump_and_load(2.48e100 * 1.0e10, false)
    dump_and_load(-2.48e100 * 1.0e10, false)
    dump_and_load(1/0.0, false)
    # NaN does not always == NaN
    json = Oj.dump(0/0.0)
    assert_equal('3.3e14159265358979323846', json)
    loaded = Oj.load(json);
    assert_equal(true, loaded.nan?)
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
    Oj.default_options = {:mode => :object}
    dump_and_load('abc', false)
    dump_and_load(':abc', false)
  end

  def test_encode
    opts = Oj.default_options
    Oj.default_options = { :ascii_only => false }
    dump_and_load("ぴーたー", false)

    Oj.default_options = { :ascii_only => true }
    json = Oj.dump("ぴーたー")
    assert_equal(%{"\\u3074\\u30fc\\u305f\\u30fc"}, json)
    dump_and_load("ぴーたー", false)
    Oj.default_options = opts
  end

  def test_unicode
    # hits the 3 normal ranges and one extended surrogate pair
    json = %{"\\u019f\\u05e9\\u3074\\ud834\\udd1e"}
    obj = Oj.load(json)
    json2 = Oj.dump(obj, :ascii_only => true)
    assert_equal(json, json2)
  end

  def test_dump_options
    json = Oj.dump({ 'a' => 1, 'b' => [true, false]},
                   :mode => :compat,
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

  def test_null_char
    assert_raises(Oj::ParseError) { Oj.load("\"\0\"") }
    assert_raises(Oj::ParseError) { Oj.load("\"\\\0\"") }
  end

  def test_array
    dump_and_load([], false)
    dump_and_load([true, false], false)
    dump_and_load(['a', 1, nil], false)
    dump_and_load([[nil]], false)
    dump_and_load([[nil], 58], false)
  end
  def test_array_not_closed
    begin
      Oj.load('[')
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end

  # multiple JSON in one string
  def test_multiple_json_callback
    json = %{{"a":1}
[1,2][3,4]
{"b":2}
}
    results = []
    Oj.load(json) { |x| results << x }
    assert_equal([{"a"=>1}, [1,2], [3,4], {"b"=>2}], results)
  end

  def test_multiple_json_no_callback
    json = %{{"a":1}
[1,2][3,4]
{"b":2}
}
    assert_raises(Oj::ParseError) { Oj.load(json) }
  end

  # encoding tests
  def test_does_not_escape_entities_by_default
    Oj.default_options = { :escape_mode => :ascii } # set in mimic mode
    # use Oj to create the hash since some Rubies don't deal nicely with unicode.
    json = %{{"key":"I <3 this\\u2028space"}}
    hash = Oj.load(json)
    out = Oj.dump(hash)
    assert_equal(json, out)
  end
  def test_escapes_entities_by_default_when_configured_to_do_so
    hash = {'key' => "I <3 this"}
    Oj.default_options = {:escape_mode => :xss_safe}
    out = Oj.dump hash
    assert_equal(%{{"key":"I \\u003c3 this"}}, out)
  end
  def test_escapes_entities_when_asked_to
    hash = {'key' => "I <3 this"}
    out = Oj.dump(hash, :escape_mode => :xss_safe)
    assert_equal(%{{"key":"I \\u003c3 this"}}, out)
  end
  def test_does_not_escape_entities_when_not_asked_to
    hash = {'key' => "I <3 this"}
    out = Oj.dump(hash, :escape_mode => :json)
    assert_equal(%{{"key":"I <3 this"}}, out)
  end
  def test_escapes_common_xss_vectors
    hash = {'key' => "<script>alert(123) && formatHD()</script>"}
    out = Oj.dump(hash, :escape_mode => :xss_safe)
    assert_equal(%{{"key":"\\u003cscript\\u003ealert(123) \\u0026\\u0026 formatHD()\\u003c\\/script\\u003e"}}, out)
  end
  def test_escape_newline_by_default
    Oj.default_options = { :escape_mode => :json }
    json = %{["one","two\\n2"]}
    x = Oj.load(json)
    out = Oj.dump(x)
    assert_equal(json, out)
  end
  def test_does_not_escape_newline
    Oj.default_options = { :escape_mode => :newline }
    json = %{["one","two\n2"]}
    x = Oj.load(json)
    out = Oj.dump(x)
    assert_equal(json, out)
  end

  # Symbol
  def test_symbol_strict
    begin
      Oj.dump(:abc, :mode => :strict)
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end
  def test_symbol_null
    json = Oj.dump(:abc, :mode => :null)
    assert_equal('null', json)
  end
  def test_symbol_compat
    json = Oj.dump(:abc, :mode => :compat)
    assert_equal('"abc"', json)
  end
  def test_symbol_object
    Oj.default_options = { :mode => :object }
    #dump_and_load(''.to_sym, false)
    dump_and_load(:abc, false)
    dump_and_load(':xyz'.to_sym, false)
  end

  # Time
  def test_time_strict
    t = Time.local(2012, 1, 5, 23, 58, 7)
    begin
      Oj.dump(t, :mode => :strict)
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end
  def test_time_null
    t = Time.local(2012, 1, 5, 23, 58, 7)
    json = Oj.dump(t, :mode => :null)
    assert_equal('null', json)
  end
  def test_unix_time_compat
    t = Time.xmlschema("2012-01-05T23:58:07.123456000+09:00")
    #t = Time.local(2012, 1, 5, 23, 58, 7, 123456)
    json = Oj.dump(t, :mode => :compat, :time_format => :unix)
    assert_equal(%{1325775487.123456000}, json)
  end
  def test_unix_time_compat_precision
    t = Time.xmlschema("2012-01-05T23:58:07.123456789+09:00")
    #t = Time.local(2012, 1, 5, 23, 58, 7, 123456)
    json = Oj.dump(t, :mode => :compat, :second_precision => 5, :time_format => :unix)
    assert_equal(%{1325775487.12346}, json)
    t = Time.xmlschema("2012-01-05T23:58:07.999600+09:00")
    json = Oj.dump(t, :mode => :compat, :second_precision => 3, :time_format => :unix)
    assert_equal(%{1325775488.000}, json)
  end
  def test_unix_time_compat_early
    t = Time.xmlschema("1954-01-05T00:00:00.123456789+00:00")
    json = Oj.dump(t, :mode => :compat, :second_precision => 5, :time_format => :unix)
    assert_equal(%{-504575999.87654}, json)
  end
  def test_unix_time_compat_1970
    t = Time.xmlschema("1970-01-01T00:00:00.123456789+00:00")
    json = Oj.dump(t, :mode => :compat, :second_precision => 5, :time_format => :unix)
    assert_equal(%{0.12346}, json)
  end
  def test_ruby_time_compat
    t = Time.xmlschema("2012-01-05T23:58:07.123456000+09:00")
    json = Oj.dump(t, :mode => :compat, :time_format => :ruby)
    #assert_equal(%{"2012-01-05 23:58:07 +0900"}, json)
    assert_equal(%{"#{t.to_s}"}, json)
  end
  def test_xml_time_compat
    begin
      t = Time.new(2012, 1, 5, 23, 58, 7.123456000, 34200)
      json = Oj.dump(t, :mode => :compat, :time_format => :xmlschema)
      assert_equal(%{"2012-01-05T23:58:07.123456000+09:30"}, json)
    rescue Exception
      # some Rubies (1.8.7) do not allow the timezome to be set
      t = Time.local(2012, 1, 5, 23, 58, 7, 123456)
      json = Oj.dump(t, :mode => :compat, :time_format => :xmlschema)
      tz = t.utc_offset
      # Ruby does not handle a %+02d properly so...
      sign = '+'
      if 0 > tz
        sign = '-'
        tz = -tz
      end
      assert_equal(%{"2012-01-05T23:58:07.123456000%s%02d:%02d"} % [sign, tz / 3600, tz / 60 % 60], json)
    end
  end
  def test_xml_time_compat_no_secs
    begin
      t = Time.new(2012, 1, 5, 23, 58, 7.0, 34200)
      json = Oj.dump(t, :mode => :compat, :time_format => :xmlschema)
      assert_equal(%{"2012-01-05T23:58:07+09:30"}, json)
    rescue Exception
      # some Rubies (1.8.7) do not allow the timezome to be set
      t = Time.local(2012, 1, 5, 23, 58, 7, 0)
      json = Oj.dump(t, :mode => :compat, :time_format => :xmlschema)
      tz = t.utc_offset
      # Ruby does not handle a %+02d properly so...
      sign = '+'
      if 0 > tz
        sign = '-'
        tz = -tz
      end
      assert_equal(%{"2012-01-05T23:58:07%s%02d:%02d"} % [sign, tz / 3600, tz / 60 % 60], json)
    end
  end
  def test_xml_time_compat_precision
    begin
      t = Time.new(2012, 1, 5, 23, 58, 7.123456789, 32400)
      json = Oj.dump(t, :mode => :compat, :time_format => :xmlschema, :second_precision => 5)
      assert_equal(%{"2012-01-05T23:58:07.12346+09:00"}, json)
    rescue Exception
      # some Rubies (1.8.7) do not allow the timezome to be set
      t = Time.local(2012, 1, 5, 23, 58, 7, 123456)
      json = Oj.dump(t, :mode => :compat, :time_format => :xmlschema, :second_precision => 5)
      tz = t.utc_offset
      # Ruby does not handle a %+02d properly so...
      sign = '+'
      if 0 > tz
        sign = '-'
        tz = -tz
      end
      assert_equal(%{"2012-01-05T23:58:07.12346%s%02d:%02d"} % [sign, tz / 3600, tz / 60 % 60], json)
    end
  end
  def test_xml_time_compat_precision_round
    begin
      t = Time.new(2012, 1, 5, 23, 58, 7.9996, 32400)
      json = Oj.dump(t, :mode => :compat, :time_format => :xmlschema, :second_precision => 3)
      assert_equal(%{"2012-01-05T23:58:08+09:00"}, json)
    rescue Exception
      # some Rubies (1.8.7) do not allow the timezome to be set
      t = Time.local(2012, 1, 5, 23, 58, 7, 999600)
      json = Oj.dump(t, :mode => :compat, :time_format => :xmlschema, :second_precision => 3)
      tz = t.utc_offset
      # Ruby does not handle a %+02d properly so...
      sign = '+'
      if 0 > tz
        sign = '-'
        tz = -tz
      end
      assert_equal(%{"2012-01-05T23:58:08%s%02d:%02d"} % [sign, tz / 3600, tz / 60 % 60], json)
    end
  end
  def test_xml_time_compat_zulu
    begin
      t = Time.new(2012, 1, 5, 23, 58, 7.0, 0)
      json = Oj.dump(t, :mode => :compat, :time_format => :xmlschema)
      assert_equal(%{"2012-01-05T23:58:07Z"}, json)
    rescue Exception
      # some Rubies (1.8.7) do not allow the timezome to be set
      t = Time.utc(2012, 1, 5, 23, 58, 7, 0)
      json = Oj.dump(t, :mode => :compat, :time_format => :xmlschema)
      #tz = t.utc_offset
      assert_equal(%{"2012-01-05T23:58:07Z"}, json)
    end
  end

  # Class
  def test_class_strict
    begin
      Oj.dump(Juice, :mode => :strict)
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end
  def test_class_null
    json = Oj.dump(Juice, :mode => :null)
    assert_equal('null', json)
  end
  def test_class_compat
    json = Oj.dump(Juice, :mode => :compat)
    assert_equal(%{"Juice"}, json)
  end
  def test_class_object
    Oj.default_options = { :mode => :object }
    dump_and_load(Juice, false)
  end

  # Module
  def test_module_strict
    begin
      Oj.dump(TestModule, :mode => :strict)
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end
  def test_module_null
    json = Oj.dump(TestModule, :mode => :null)
    assert_equal('null', json)
  end
  def test_module_compat
    json = Oj.dump(TestModule, :mode => :compat)
    assert_equal(%{"Juice::TestModule"}, json)
  end
  def test_module_object
    Oj.default_options = { :mode => :object }
    dump_and_load(TestModule, false)
  end

  # Hash
  def test_hash
    Oj.default_options = { :mode => :strict }
    dump_and_load({}, false)
    dump_and_load({ 'true' => true, 'false' => false}, false)
    dump_and_load({ 'true' => true, 'array' => [], 'hash' => { }}, false)
  end
  def test_non_str_hash_strict
    begin
      Oj.dump({ 1 => true, 0 => false }, :mode => :strict)
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end
  def test_non_str_hash_null
    begin
      Oj.dump({ 1 => true, 0 => false }, :mode => :null)
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end
  def test_non_str_hash_compat
    json = Oj.dump({ 1 => true, 0 => false }, :mode => :compat)
    h = Oj.load(json, :mode => :strict)
    assert_equal({ "1" => true, "0" => false }, h)
  end
  def test_non_str_hash_object
    Oj.default_options = { :mode => :object }
    json = Oj.dump({ 1 => true, :sim => nil })
    h = Oj.load(json, :mode => :strict)
    assert_equal({"^#1" => [1, true], ":sim" => nil}, h)
    h = Oj.load(json)
    assert_equal({ 1 => true, :sim => nil }, h)
  end
  def test_mixed_hash_object
    Oj.default_options = { :mode => :object }
    json = Oj.dump({ 1 => true, 'nil' => nil, :sim => 4 })
    h = Oj.load(json, :mode => :strict)
    assert_equal({"^#1" => [1, true], "nil" => nil, ":sim" => 4}, h)
    h = Oj.load(json)
    assert_equal({ 1 => true, 'nil' => nil, :sim => 4 }, h)
  end
  def test_hash_not_closed
    begin
      Oj.load('{')
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end

  # Object with to_json()
  def test_json_object_strict
    obj = Jeez.new(true, 58)
    begin
      Oj.dump(obj, :mode => :strict)
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end
  def test_json_object_null
    obj = Jeez.new(true, 58)
    json = Oj.dump(obj, :mode => :null)
    assert_equal('null', json)
  end
  def test_json_object_compat
    Oj.default_options = { :mode => :compat, :use_to_json => true }
    obj = Jeez.new(true, 58)
    json = Oj.dump(obj, :indent => 2)
    assert(%{{"json_class":"Juice::Jeez","x":true,"y":58}
} == json ||
           %{{"json_class":"Juice::Jeez","y":58,"x":true}
} == json)
    dump_and_load(obj, false)
    Oj.default_options = { :mode => :compat, :use_to_json => false }
  end
  def test_json_object_create_id
    Oj.default_options = { :mode => :compat, :create_id => 'kson_class' }
    expected = Jeez.new(true, 58)
    json = %{{"kson_class":"Juice::Jeez","x":true,"y":58}}
    obj = Oj.load(json)
    assert_equal(expected, obj)
    Oj.default_options = { :create_id => 'json_class' }
  end
  def test_json_object_object
    obj = Jeez.new(true, 58)
    json = Oj.dump(obj, :mode => :object, :indent => 2)
    assert(%{{
  "^o":"Juice::Jeez",
  "x":true,
  "y":58
}
} == json ||
%{{
  "^o":"Juice::Jeez",
  "y":58,
  "x":true
}
} == json)
    obj2 = Oj.load(json, :mode => :object)
    assert_equal(obj, obj2)
  end

# Object with to_hash()
  def test_to_hash_object_strict
    obj = Jazz.new(true, 58)
    begin
      Oj.dump(obj, :mode => :strict)
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end
  def test_to_hash_object_null
    obj = Jazz.new(true, 58)
    json = Oj.dump(obj, :mode => :null)
    assert_equal('null', json)
  end
  def test_to_hash_object_compat
    obj = Jazz.new(true, 58)
    json = Oj.dump(obj, :mode => :compat, :indent => 2)
    h = Oj.load(json, :mode => :strict)
    assert_equal(obj.to_hash, h)
  end
  def test_to_hash_object_object
    obj = Jazz.new(true, 58)
    json = Oj.dump(obj, :mode => :object, :indent => 2)
    assert(%{{
  "^o":"Juice::Jazz",
  "x":true,
  "y":58
}
} == json ||
%{{
  "^o":"Juice::Jazz",
  "y":58,
  "x":true
}
} == json)
    obj2 = Oj.load(json, :mode => :object)
    assert_equal(obj, obj2)
  end

  # Object with as_json() # contributed by sauliusg
  def test_as_json_object_strict
    obj = Orange.new(true, 58)
    begin
      Oj.dump(obj, :mode => :strict)
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end

  def test_as_json_object_null
    obj = Orange.new(true, 58)
    json = Oj.dump(obj, :mode => :null)
    assert_equal('null', json)
  end

  def test_as_json_object_compat_hash
    Oj.default_options = { :mode => :compat, :use_to_json => true }
    obj = Orange.new(true, 58)
    json = Oj.dump(obj, :indent => 2)
    assert(!json.nil?)
    dump_and_load(obj, false)
  end

  def test_as_json_object_compat_non_hash
    Oj.default_options = { :mode => :compat, :use_to_json => true }
    obj = Melon.new(true, 58)
    json = Oj.dump(obj, :indent => 2)
    assert_equal(%{"true 58"}, json)
  end

  def test_as_json_object_object
    obj = Orange.new(true, 58)
    json = Oj.dump(obj, :mode => :object, :indent => 2)
    assert(%{{
  "^o":"Juice::Orange",
  "x":true,
  "y":58
}
} == json ||
%{{
  "^o":"Juice::Orange",
  "y":58,
  "x":true
}
} == json)
    obj2 = Oj.load(json, :mode => :object)
    assert_equal(obj, obj2)
  end

  # Object without to_json() or to_hash()
  def test_object_strict
    obj = Jam.new(true, 58)
    begin
      Oj.dump(obj, :mode => :strict)
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end
  def test_object_null
    obj = Jam.new(true, 58)
    json = Oj.dump(obj, :mode => :null)
    assert_equal('null', json)
  end
  def test_object_compat
    obj = Jam.new(true, 58)
    json = Oj.dump(obj, :mode => :compat, :indent => 2)
    assert(%{{
  "x":true,
  "y":58
}
} == json ||
%{{
  "y":58,
  "x":true
}
} == json)
  end
  def test_object_object
    obj = Jam.new(true, 58)
    json = Oj.dump(obj, :mode => :object, :indent => 2)
    assert(%{{
  "^o":"Juice::Jam",
  "x":true,
  "y":58
}
} == json ||
%{{
  "^o":"Juice::Jam",
  "y":58,
  "x":true
}
} == json)
    obj2 = Oj.load(json, :mode => :object)
    assert_equal(obj, obj2)
  end

  def test_object_object_no_cache
    obj = Jam.new(true, 58)
    json = Oj.dump(obj, :mode => :object, :indent => 2)
    assert(%{{
  "^o":"Juice::Jam",
  "x":true,
  "y":58
}
} == json ||
%{{
  "^o":"Juice::Jam",
  "y":58,
  "x":true
}
} == json)
    obj2 = Oj.load(json, :mode => :object, :class_cache => false)
    assert_equal(obj, obj2)
  end

  # Exception
  def test_exception
    err = nil
    begin
      raise StandardError.new('A Message')
    rescue Exception => e
      err = e
    end
    json = Oj.dump(err, :mode => :object, :indent => 2)
    #puts "*** #{json}"
    e2 = Oj.load(json, :mode => :strict)
    assert_equal(err.class.to_s, e2['^o'])
    assert_equal(err.message, e2['~mesg'])
    assert_equal(err.backtrace, e2['~bt'])
    e2 = Oj.load(json, :mode => :object)
    if RUBY_VERSION.start_with?('1.8') || 'rubinius' == $ruby
      assert_equal(e.class, e2.class);
      assert_equal(e.message, e2.message);
      assert_equal(e.backtrace, e2.backtrace);
    else
      assert_equal(e, e2);
    end
  end

  # Range
  def test_range_strict
    begin
      Oj.dump(1..7, :mode => :strict)
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end
  def test_range_null
    json = Oj.dump(1..7, :mode => :null)
    assert_equal('null', json)
  end
  def test_range_compat
    json = Oj.dump(1..7, :mode => :compat)
    h = Oj.load(json, :mode => :strict)
    assert_equal({'begin' => 1, 'end' => 7, 'exclude_end' => false}, h)
    json = Oj.dump(1...7, :mode => :compat)
    h = Oj.load(json, :mode => :strict)
    assert_equal({'begin' => 1, 'end' => 7, 'exclude_end' => true}, h)
  end
  def test_range_object
    unless RUBY_VERSION.start_with?('1.8')
      Oj.default_options = { :mode => :object }
      json = Oj.dump(1..7, :mode => :object, :indent => 0)
      if 'rubinius' == $ruby
        assert(%{{"^O":"Range","begin":1,"end":7,"exclude_end?":false}} == json)
      else
        assert_equal(%{{"^u":["Range",1,7,false]}}, json)
      end
      dump_and_load(1..7, false)
      dump_and_load(1..1, false)
      dump_and_load(1...7, false)
    end
  end

  # BigNum
  def test_bignum_strict
    json = Oj.dump(7 ** 55, :mode => :strict)
    assert_equal('30226801971775055948247051683954096612865741943', json)
  end
  def test_bignum_null
    json = Oj.dump(7 ** 55, :mode => :null)
    assert_equal('30226801971775055948247051683954096612865741943', json)
  end
  def test_bignum_compat
    json = Oj.dump(7 ** 55, :mode => :compat)
    b = Oj.load(json, :mode => :strict)
    assert_equal(30226801971775055948247051683954096612865741943, b)
  end

  def test_bignum_object
    dump_and_load(7 ** 55, false)
    dump_and_load(10 ** 19, false)
  end

  # BigDecimal
  def test_bigdecimal_strict
    mode = Oj.default_options[:mode]
    Oj.default_options = {:mode => :strict}
    dump_and_load(BigDecimal.new('3.14159265358979323846'), false)
    Oj.default_options = {:mode => mode}
  end
  def test_bigdecimal_null
    mode = Oj.default_options[:mode]
    Oj.default_options = {:mode => :null}
    dump_and_load(BigDecimal.new('3.14159265358979323846'), false)
    Oj.default_options = {:mode => mode}
  end
  def test_bigdecimal_compat
    orig = BigDecimal.new('80.51')
    json = Oj.dump(orig, :mode => :compat, :bigdecimal_as_decimal => false)
    bg = Oj.load(json, :mode => :compat)
    assert_equal(orig.to_s, bg)
    orig = BigDecimal.new('3.14159265358979323846')
    json = Oj.dump(orig, :mode => :compat, :bigdecimal_as_decimal => false)
    bg = Oj.load(json, :mode => :compat)
    assert_equal(orig.to_s, bg)
  end
  def test_bigdecimal_load
    orig = BigDecimal.new('80.51')
    json = Oj.dump(orig, :mode => :compat, :bigdecimal_as_decimal => true)
    bg = Oj.load(json, :mode => :compat, :bigdecimal_load => true)
    assert_equal(BigDecimal, bg.class)
    assert_equal(orig, bg)
  end
  def test_float_load
    orig = BigDecimal.new('80.51')
    json = Oj.dump(orig, :mode => :compat, :bigdecimal_as_decimal => true)
    bg = Oj.load(json, :mode => :compat, :bigdecimal_load => :float)
    assert_equal(Float, bg.class)
    assert_equal(orig.to_f, bg)
  end
  def test_bigdecimal_compat_as_json
    orig = BigDecimal.new('80.51')
    BigDecimal.send(:define_method, :as_json) do
      %{this is big}
    end
    json = Oj.dump(orig, :mode => :compat, :bigdecimal_as_decimal => false)
    bg = Oj.load(json, :mode => :compat)
    assert_equal("this is big", bg)
    BigDecimal.send(:remove_method, :as_json) # cleanup
  end
  def test_bigdecimal_object
    mode = Oj.default_options[:mode]
    Oj.default_options = {:mode => :object}
    dump_and_load(BigDecimal.new('3.14159265358979323846'), false)
    Oj.default_options = {:mode => mode}
    # Infinity is the same for Float and BigDecimal
    json = Oj.dump(BigDecimal.new('Infinity'), :mode => :object)
    assert_equal('Infinity', json)
    json = Oj.dump(BigDecimal.new('-Infinity'), :mode => :object)
    assert_equal('-Infinity', json)
  end

  def test_infinity
    n = Oj.load('Infinity', :mode => :object)
    assert_equal(BigDecimal.new('Infinity').to_f, n);
    begin
      Oj.load('Infinity', :mode => :strict)
      fail()
    rescue Oj::ParseError
      assert(true)
    end
    x = Oj.load('Infinity', :mode => :compat)
    assert_equal('Infinity', x.to_s)

  end

  # Date
  def test_date_strict
    begin
      Oj.dump(Date.new(2012, 6, 19), :mode => :strict)
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end
  def test_date_null
    json = Oj.dump(Date.new(2012, 6, 19), :mode => :null)
    assert_equal('null', json)
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
      assert_equal({"year" => orig.year, "month" => orig.month, "day" => orig.day, "start" => orig.start}, x)
    end
  end
  def test_date_object
    Oj.default_options = {:mode => :object}
    dump_and_load(Date.new(2012, 6, 19), false)
  end

  # DateTime
  def test_datetime_strict
    begin
      Oj.dump(DateTime.new(2012, 6, 19, 20, 19, 27), :mode => :strict)
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end
  def test_datetime_null
    json = Oj.dump(DateTime.new(2012, 6, 19, 20, 19, 27), :mode => :null)
    assert_equal('null', json)
  end
  def test_datetime_compat
    orig = DateTime.new(2012, 6, 19, 20, 19, 27)
    json = Oj.dump(orig, :mode => :compat)
    x = Oj.load(json, :mode => :compat)
    # Some Rubies implement Date as data and some as a real Object. Either are
    # okay for the test.
    assert_equal(orig.to_s, x)
  end
  def test_datetime_object
    Oj.default_options = {:mode => :object}
    dump_and_load(DateTime.new(2012, 6, 19), false)
  end

  # autodefine Oj::Bag
  def test_bag
    json = %{{
  "^o":"Juice::Jem",
  "x":true,
  "y":58 }}
    obj = Oj.load(json, :mode => :object, :auto_define => true)
    assert_equal('Juice::Jem', obj.class.name)
    assert_equal(true, obj.x)
    assert_equal(58, obj.y)
  end

  # Circular
  def test_circular_object
    obj = Jam.new(nil, 58)
    obj.x = obj
    json = Oj.dump(obj, :mode => :object, :indent => 2, :circular => true)
    assert(%{{
  "^o":"Juice::Jam",
  "^i":1,
  "x":"^r1",
  "y":58
}
} == json ||
%{{
  "^o":"Juice::Jam",
  "^i":1,
  "y":58,
  "x":"^r1"
}
} == json)
    obj2 = Oj.load(json, :mode => :object, :circular => true)
    assert_equal(obj2.x.__id__, obj2.__id__)
  end

  def test_circular_hash
    h = { 'a' => 7 }
    h['b'] = h
    json = Oj.dump(h, :mode => :object, :indent => 2, :circular => true)
    ha = Oj.load(json, :mode => :strict)
    assert_equal({'^i' => 1, 'a' => 7, 'b' => '^r1'}, ha)
    Oj.load(json, :mode => :object, :circular => true)
    assert_equal(h['b'].__id__, h.__id__)
  end

  def test_circular_array
    a = [7]
    a << a
    json = Oj.dump(a, :mode => :object, :indent => 2, :circular => true)
    assert_equal(%{[
  "^i1",
  7,
  "^r1"
]
}, json)
    a2 = Oj.load(json, :mode => :object, :circular => true)
    assert_equal(a2[1].__id__, a2.__id__)
  end

  def test_circular
    h = { 'a' => 7 }
    obj = Jam.new(h, 58)
    obj.x['b'] = obj
    json = Oj.dump(obj, :mode => :object, :indent => 2, :circular => true)
    ha = Oj.load(json, :mode => :strict)
    assert_equal({'^o' => 'Juice::Jam', '^i' => 1, 'x' => { '^i' => 2, 'a' => 7, 'b' => '^r1' }, 'y' => 58 }, ha)
    Oj.load(json, :mode => :object, :circular => true)
    assert_equal(obj.x.__id__, h.__id__)
    assert_equal(h['b'].__id__, obj.__id__)
  end

# Stream Deeply Nested
  def test_deep_nest
    begin
      n = 10000
      Oj.strict_load('[' * n + ']' * n)
    rescue Exception => e
      assert(false, e.message)
    end
  end

  def test_deep_nest_dump
    begin
      a = []
      10000.times { a << [a] }
      Oj.dump(a)
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end

# Stream IO
  def test_io_string
    src = { 'x' => true, 'y' => 58, 'z' => [1, 2, 3]}
    output = StringIO.open("", "w+")
    Oj.to_stream(output, src)

    input = StringIO.new(output.string())
    obj = Oj.load(input, :mode => :strict)
    assert_equal(src, obj)
  end

  def test_io_file
    src = { 'x' => true, 'y' => 58, 'z' => [1, 2, 3]}
    filename = File.join(File.dirname(__FILE__), 'open_file_test.json')
    File.open(filename, "w") { |f|
      Oj.to_stream(f, src)
    }
    f = File.new(filename)
    obj = Oj.load(f, :mode => :strict)
    f.close()
    assert_equal(src, obj)
  end

# symbol_keys option
  def test_symbol_keys
    json = %{{
  "x":true,
  "y":58,
  "z": [1,2,3]
}
}
    obj = Oj.load(json, :mode => :strict, :symbol_keys => true)
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
    obj = Oj.load(json, :mode => :strict)
    assert_equal({ 'x' => true, 'y' => 58, 'z' => [1, 2, 3]}, obj)
  end

  def test_comment_c
    json = %{/*before*/
{
  "x"/*one*/:/*two*/true,
  "y":58,
  "z": [1,2,3]}
}
    obj = Oj.load(json, :mode => :strict)
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
    obj = Oj.load(json, :mode => :strict)
    assert_equal({ 'x' => true, 'y' => 58, 'z' => [1, 2, 3]}, obj)
  end

  def test_nilnil_false
    begin
      Oj.load(nil)
    rescue Exception
      assert(true)
      return
    end
    assert(false, "*** expected an exception")
  end

  def test_nilnil_true
    obj = Oj.load(nil, :nilnil => true)
    assert_equal(nil, obj)
  end

  def test_quirks_null_mode
    assert_raises(Oj::ParseError) { Oj.load("null", :quirks_mode => false) }
    assert_equal(nil, Oj.load("null", :quirks_mode => true))
  end

  def test_quirks_bool_mode
    assert_raises(Oj::ParseError) { Oj.load("true", :quirks_mode => false) }
    assert_equal(true, Oj.load("true", :quirks_mode => true))
  end

  def test_quirks_number_mode
    assert_raises(Oj::ParseError) { Oj.load("123", :quirks_mode => false) }
    assert_equal(123, Oj.load("123", :quirks_mode => true))
  end

  def test_quirks_decimal_mode
    assert_raises(Oj::ParseError) { Oj.load("123.45", :quirks_mode => false) }
    assert_equal(123.45, Oj.load("123.45", :quirks_mode => true))
  end

  def test_quirks_string_mode
    assert_raises(Oj::ParseError) { Oj.load('"string"', :quirks_mode => false) }
    assert_equal('string', Oj.load('"string"', :quirks_mode => true))
  end

  def test_quirks_array_mode
    assert_equal([], Oj.load("[]", :quirks_mode => false))
    assert_equal([], Oj.load("[]", :quirks_mode => true))
  end

  def test_quirks_object_mode
    assert_equal({}, Oj.load("{}", :quirks_mode => false))
    assert_equal({}, Oj.load("{}", :quirks_mode => true))
  end

  def dump_and_load(obj, trace=false)
    json = Oj.dump(obj, :indent => 2)
    puts json if trace
    loaded = Oj.load(json)
    assert_equal(obj, loaded)
    loaded
  end

end
