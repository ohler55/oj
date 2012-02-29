#!/usr/bin/env ruby -wW1
# encoding: UTF-8

$: << '.'
$: << File.join(File.dirname(__FILE__), "../lib")
$: << File.join(File.dirname(__FILE__), "../ext")

require 'optparse'
require 'yajl'
require 'perf'
require 'json'
require 'json/pure'
require 'json/ext'
require 'msgpack'
require 'oj'
require 'ox'

class Jazz
  def initialize()
    @boolean = true
    @number = 58
    @string = "A string"
    @array = [true, false, nil]
    @hash = { 'one' => 1, 'two' => 3 }
  end
  def to_json(*) # Yajl and JSON have different signatures
    %{
{ "json_class":"Jazz",
  "boolean":#{@boolean},
  "number":#{@number},
  "string":"#{@string}",
  "array":#{@array},
  "hash":#{@hash},
}}
  end
  def to_hash()
    { 'json_class' => "Jazz",
      'boolean' => @boolean,
      'number' => @number,
      'string' => @string,
      'array' => @array,
      'hash' => @hash,
    }
  end
  def to_msgpack(out)
    out << MessagePack.pack(to_hash())
  end
  def self.json_create(h)
    j = self.new()
    j.instance_variable_set(:@boolean, h['boolean'])
    j.instance_variable_set(:@number, h['number'])
    j.instance_variable_set(:@string, h['string'])
    j.instance_variable_set(:@array, h['array'])
    j.instance_variable_set(:@hash, h['hash'])
    j
  end
end

$verbose = false
$indent = 0
$iter = 100000
$with_object = true
$with_bignum = true
$with_nums = true

opts = OptionParser.new
opts.on("-c", "--count [Int]", Integer, "iterations")       { |i| $iter = i }
opts.on("-i", "--indent [Int]", Integer, "indentation")     { |i| $indent = i }
opts.on("-o", "without objects")                            { $with_object = false }
opts.on("-b", "without bignum")                             { $with_bignum = false }
opts.on("-n", "without numbers")                            { $with_nums = false }
opts.on("-h", "--help", "Show this display")                { puts opts; Process.exit!(0) }
files = opts.parse(ARGV)

if $with_nums
  $obj = {
    'a' => 'Alpha',
    'b' => true,
    'c' => 12345,
    'd' => [ true, [false, [12345, nil], 3.967, ['something', false], nil]],
    'e' => { 'one' => 1, 'two' => 2 },
    'f' => nil,
  }
  $obj['g'] = Jazz.new() if $with_object
  $obj['h'] = 12345678901234567890123456789 if $with_bignum
else
  $obj = {
    'a' => 'Alpha',
    'b' => true,
    'c' => '12345',
    'd' => [ true, [false, ['12345', nil], '3.967', ['something', false], nil]],
    'e' => { 'one' => '1', 'two' => '2' },
    'f' => nil,
  }
end

Oj.default_options = { :indent => $indent, :mode => :compat }
Ox.default_options = { :indent => $indent, :mode => :object }

$json = Oj.dump($obj)
$obj_json = Oj.dump($obj, :mode => :object)
$xml = Ox.dump($obj, :indent => $indent)
begin
  $msgpack = MessagePack.pack($obj)
rescue Exception => e
  puts "MessagePack failed to pack! #{e.class}: #{e.message}.\nSkipping."
  $msgpack = nil
end

if $verbose
  puts "json:\n#{$json}\n"
  puts "object json:\n#{$obj_json}\n"
  puts "Oj loaded object:\n#{Oj.load($json)}\n"
  puts "Yajl loaded object:\n#{Yajl::Parser.parse($json)}\n"
  puts "JSON loaded object:\n#{JSON::Ext::Parser.new($json).parse}\n"
end

puts '-' * 80
puts "Load/Parse Performance"
perf = Perf.new()
perf.add('Oj', 'load') { Oj.load($json) }
perf.before('Oj') { Oj.default_options = { :mode => :compat} }
perf.add('Oj:object', 'load') { Oj.load($obj_json) }
perf.before('Oj:object') { Oj.default_options = { :mode => :object} }
perf.add('Yajl', 'parse') { Yajl::Parser.parse($json) }
perf.add('JSON::Ext', 'parse') { JSON::Ext::Parser.new($json).parse }
perf.add('JSON::Pure', 'parse') { JSON::Pure::Parser.new($json).parse }
perf.add('Ox', 'load') { Ox.load($xml) }
perf.add('MessagePack', 'unpack') { MessagePack.unpack($msgpack) } unless $msgpack.nil?
perf.run($iter)

puts
puts '-' * 80
puts "Dump/Encode/Generate Performance"
perf = Perf.new()
perf.add('Oj', 'dump') { Oj.dump($obj) }
perf.before('Oj') { Oj.default_options = { :mode => :compat} }
perf.add('Oj:object', 'dump') { Oj.dump($obj) }
perf.before('Oj:object') { Oj.default_options = { :mode => :object} }
perf.add('Yajl', 'encode') { Yajl::Encoder.encode($obj) }
if 0 == $indent
  perf.add('JSON::Ext', 'generate') { JSON.generate($obj) }
else
  perf.add('JSON::Ext', 'generate') { JSON.pretty_generate($obj) }
end
perf.before('JSON::Ext') { JSON.generator = JSON::Ext::Generator }
if 0 == $indent
  perf.add('JSON::Pure', 'generate') { JSON.generate($obj) }
else
  perf.add('JSON::Pure', 'generate') { JSON.pretty_generate($obj) }
end
perf.before('JSON::Pure') { JSON.generator = JSON::Pure::Generator }
perf.add('Ox', 'dump') { Ox.dump($obj) }
perf.add('MessagePack', 'pack') { MessagePack.pack($obj) } unless $msgpack.nil?
perf.run($iter)

puts
