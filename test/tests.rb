#!/usr/bin/env ruby -wW1
# encoding: UTF-8

$: << File.join(File.dirname(__FILE__), "../lib")
$: << File.join(File.dirname(__FILE__), "../ext")

require 'test/unit'
require 'stringio'
require 'oj'

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

end # Jam

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
end # Jeez

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
end # Jazz

class Range
  def to_hash()
    { 'begin' => self.begin, 'end' => self.end, 'exclude_end' => self.exclude_end? }
  end
end # Range

class Juice < ::Test::Unit::TestCase

  def test0_get_options
    opts = Oj.default_options()
    assert_equal({ :encoding=>nil,
                   :indent=>0,
                   :circular=>false,
                   :auto_define=>true,
                   :symbol_keys=>false,
                   :ascii_only=>false,
                   :mode=>:object}, opts)
  end

  def test0_set_options
    orig = {
      :encoding=>nil,
      :indent=>0,
      :circular=>false,
      :auto_define=>true,
      :symbol_keys=>false,
      :ascii_only=>false,
      :mode=>:object}
    o2 = {
      :encoding=>"UTF-8",
      :indent=>4,
      :circular=>true,
      :auto_define=>false,
      :symbol_keys=>true,
      :ascii_only=>true,
      :mode=>:compat}
    o3 = { :indent => 4 }
    Oj.default_options = o2
    opts = Oj.default_options()
    assert_equal(opts, o2);
    Oj.default_options = o3 # see if it throws an exception
    Oj.default_options = orig # return to original
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

  def test_bignum
    dump_and_load(12345678901234567890123456789, false)
  end

  def test_float
    dump_and_load(0.0, false)
    dump_and_load(12345.6789, false)
    dump_and_load(-54321.012, false)
    dump_and_load(2.48e16, false)
    dump_and_load(2.48e1000, false)
    dump_and_load(-2.48e1000, false)
  end

  def test_string
    dump_and_load('', false)
    dump_and_load('abc', false)
    dump_and_load("abc\ndef", false)
    dump_and_load("a\u0041", false)
  end

  def test_string_object
    dump_and_load('abc', false)
    dump_and_load(':abc', false)
  end

  def test_encode
    Oj.default_options = { :encoding => 'UTF-8' }
    dump_and_load("ぴーたー", false)
    Oj.default_options = { :ascii_only => true }
    dump_and_load("ぴーたー", false)
    Oj.default_options = { :encoding => nil, :ascii_only => false }
  end

  def test_array
    dump_and_load([], false)
    dump_and_load([true, false], false)
    dump_and_load(['a', 1, nil], false)
    dump_and_load([[nil]], false)
    dump_and_load([[nil], 58], false)
  end

  # Symbol
  def test_symbol_strict
    begin
      json = Oj.dump(:abc, :mode => :strict)
    rescue Exception => e
      assert(true)
    end
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
      json = Oj.dump(t, :mode => :strict)
    rescue Exception => e
      assert(true)
    end
  end
  def test_time_null
    t = Time.local(2012, 1, 5, 23, 58, 7)
    json = Oj.dump(t, :mode => :null)
    assert_equal('null', json)
  end
  def test_time_compat
    t = Time.local(2012, 1, 5, 23, 58, 7)
    json = Oj.dump(t, :mode => :compat)
    assert_equal(%{1325775487.000000}, json)
  end    
  def test_time_object
    t = Time.now()
    Oj.default_options = { :mode => :object }
    dump_and_load(t, false)
  end

# Class
  def test_class_strict
    begin
      json = Oj.dump(Juice, :mode => :strict)
    rescue Exception => e
      assert(true)
    end
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

# Hash
  def test_hash
    Oj.default_options = { :mode => :strict }
    dump_and_load({}, false)
    dump_and_load({ 'true' => true, 'false' => false}, false)
    dump_and_load({ 'true' => true, 'array' => [], 'hash' => { }}, false)
  end
  def test_non_str_hash_strict
    begin
      json = Oj.dump({ 1 => true, 0 => false }, :mode => :strict)
    rescue Exception => e
      assert(true)
    end
  end
  def test_non_str_hash_null
    begin
      json = Oj.dump({ 1 => true, 0 => false }, :mode => :null)
    rescue Exception => e
      assert(true)
    end
  end
  def test_non_str_hash_compat
    begin
      json = Oj.dump({ 1 => true, 0 => false }, :mode => :compat)
    rescue Exception => e
      assert(true)
    end
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

# Object with to_json()
  def test_json_object_strict
    obj = Jeez.new(true, 58)
    begin
      json = Oj.dump(obj, :mode => :strict)
    rescue Exception => e
      assert(true)
    end
  end
  def test_json_object_null
    obj = Jeez.new(true, 58)
    json = Oj.dump(obj, :mode => :null)
    assert_equal('null', json)
  end
  def test_json_object_compat
    Oj.default_options = { :mode => :compat }
    obj = Jeez.new(true, 58)
    json = Oj.dump(obj, :indent => 2)
    assert_equal(%{{"json_class":"Jeez","x":true,"y":58}}, json)
    dump_and_load(obj, false)
  end
  def test_json_object_object
    obj = Jeez.new(true, 58)
    json = Oj.dump(obj, :mode => :object, :indent => 2)
    assert_equal(%{{
  "^o":"Jeez",
  "x":true,
  "y":58}}, json)
    obj2 = Oj.load(json, :mode => :object)
    assert_equal(obj, obj2)
  end

# Object with to_hash()
  def test_to_hash_object_strict
    obj = Jazz.new(true, 58)
    begin
      json = Oj.dump(obj, :mode => :strict)
    rescue Exception => e
      assert(true)
    end
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
    assert_equal(%{{
  "^o":"Jazz",
  "x":true,
  "y":58}}, json)
    obj2 = Oj.load(json, :mode => :object)
    assert_equal(obj, obj2)
  end

  # Object without to_json() or to_hash()
  def test_object_strict
    obj = Jam.new(true, 58)
    begin
      json = Oj.dump(obj, :mode => :strict)
    rescue Exception => e
      assert(true)
    end
  end
  def test_object_null
    obj = Jam.new(true, 58)
    json = Oj.dump(obj, :mode => :null)
    assert_equal('null', json)
  end
  def test_object_compat
    obj = Jam.new(true, 58)
    json = Oj.dump(obj, :mode => :compat, :indent => 2)
    assert_equal(%{{
  "x":true,
  "y":58}}, json)
  end
  def test_object_object
    obj = Jam.new(true, 58)
    json = Oj.dump(obj, :mode => :object, :indent => 2)
    assert_equal(%{{
  "^o":"Jam",
  "x":true,
  "y":58}}, json)
    obj2 = Oj.load(json, :mode => :object)
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
    unless RUBY_VERSION.start_with?('1.8')
      assert_equal(err.message, e2['~mesg'])
      assert_equal(err.backtrace, e2['~bt'])
      e2 = Oj.load(json, :mode => :object)
      assert_equal(e, e2);
    end
  end

  # Range
  def test_range_strict
    begin
      json = Oj.dump(1..7, :mode => :strict)
    rescue Exception => e
      assert(true)
    end
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
    # TBD get Range working with 1.8.7
    unless RUBY_VERSION.start_with?('1.8')
      Oj.default_options = { :mode => :object }
      json = Oj.dump(1..7, :mode => :object, :indent => 0)
      assert_equal(%{{"^u":["Range",1,7,false]}}, json)
      dump_and_load(1..7, false)
      dump_and_load(1..1, false)
      dump_and_load(1...7, false)
    end
  end

  # autodefine Oj::Bag
  def test_bag
    json = %{{
  "^o":"Jem",
  "x":true,
  "y":58 }}
    obj = Oj.load(json, :mode => :object)
    assert_equal('Jem', obj.class.name)
    assert_equal(true, obj.x)
    assert_equal(58, obj.y)
  end

  # Circular
  def test_circular_object
    obj = Jam.new(nil, 58)
    obj.x = obj
    json = Oj.dump(obj, :mode => :object, :indent => 2, :circular => true)
    assert_equal(%{{
  "^o":"Jam",
  "^i":1,
  "x":"^r1",
  "y":58}}, json)
    obj2 = Oj.load(json, :mode => :object, :circular => true)
    assert_equal(obj2.x.__id__, obj2.__id__)
  end

  def test_circular_hash
    h = { 'a' => 7 }
    h['b'] = h
    json = Oj.dump(h, :mode => :object, :indent => 2, :circular => true)
    ha = Oj.load(json, :mode => :strict)
    assert_equal({'^i' => 1, 'a' => 7, 'b' => '^r1'}, ha)
    h2 = Oj.load(json, :mode => :object, :circular => true)
    assert_equal(h['b'].__id__, h.__id__)
  end

  def test_circular_array
    a = [7]
    a << a
    json = Oj.dump(a, :mode => :object, :indent => 2, :circular => true)
    assert_equal(%{[
  "^i1",
  7,
  "^r1"]}, json)
    a2 = Oj.load(json, :mode => :object, :circular => true)
    assert_equal(a2[1].__id__, a2.__id__)
  end

  def test_circular
    h = { 'a' => 7 }
    obj = Jam.new(h, 58)
    obj.x['b'] = obj
    json = Oj.dump(obj, :mode => :object, :indent => 2, :circular => true)
    ha = Oj.load(json, :mode => :strict)
    assert_equal({'^o' => 'Jam', '^i' => 1, 'x' => { '^i' => 2, 'a' => 7, 'b' => '^r1' }, 'y' => 58 }, ha)
    obj2 = Oj.load(json, :mode => :object, :circular => true)
    assert_equal(obj.x.__id__, h.__id__)
    assert_equal(h['b'].__id__, obj.__id__)
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
    obj = Oj.load(input, :mode => :strict)
    assert_equal({ 'x' => true, 'y' => 58, 'z' => [1, 2, 3]}, obj)
  end

  def test_io_file
    filename = 'open_file_test.json'
    File.open(filename, 'w') { |f| f.write(%{{
  "x":true,
  "y":58,
  "z": [1,2,3]
}
}) }
    f = File.new(filename)
    obj = Oj.load(f, :mode => :strict)
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
    json = %{{
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

  def dump_and_load(obj, trace=false)
    json = Oj.dump(obj, :indent => 2)
    puts json if trace
    loaded = Oj.load(json);
    assert_equal(obj, loaded)
    loaded
  end

end
