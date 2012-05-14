#!/usr/bin/env ruby -wW1
# encoding: UTF-8

$: << '.'
$: << File.join(File.dirname(__FILE__), "../lib")
$: << File.join(File.dirname(__FILE__), "../ext")

require 'optparse'
#require 'yajl'
require 'perf'
#require 'json'
#require 'json/pure'
#require 'json/ext'
#require 'msgpack'
require 'oj'
#require 'ox'

class Jazz
  attr_accessor :boolean, :number, :string

  def initialize()
    @boolean = true
    @number = 58
    @string = "A string"
  end

  def eql?(o)
    (self.class == o.class &&
     boolean == o.boolean &&
     number == o.number &&
     string == o.string)
  end
  alias == eql?

  def to_json(*) # Yajl and JSON have different signatures
    %{
{ "json_class":"Jazz",
  "boolean":#{@boolean},
  "number":#{@number},
  "string":"#{@string}"
}}
  end

  def to_hash()
    { 'json_class' => "Jazz",
      'boolean' => @boolean,
      'number' => @number,
      'string' => @string
    }
  end
  alias as_json to_hash

  def to_msgpack(out='')
    to_hash().to_msgpack(out)
  end

  def self.json_create(h)
    j = self.new()
    j.instance_variable_set(:@boolean, h['boolean'])
    j.instance_variable_set(:@number, h['number'])
    j.instance_variable_set(:@string, h['string'])
    j
  end
end

$verbose = false
$indent = 0
$iter = 10000
$with_object = true
$with_bignum = true
$with_nums = true

opts = OptionParser.new
opts.on("-v", "verbose")                                    { $verbose = true }
opts.on("-c", "--count [Int]", Integer, "iterations")       { |i| $iter = i }
opts.on("-i", "--indent [Int]", Integer, "indentation")     { |i| $indent = i }
opts.on("-o", "without objects")                            { $with_object = false }
opts.on("-b", "without bignum")                             { $with_bignum = false }
opts.on("-n", "without numbers")                            { $with_nums = false }
opts.on("-h", "--help", "Show this display")                { puts opts; Process.exit!(0) }
files = opts.parse(ARGV)

if $with_nums
  $obj = {
    'a' => 'Alpha', # string
    'b' => true,    # boolean
    'c' => 12345,   # number
    'd' => [ true, [false, [12345, nil], 3.967, ['something', false], nil]], # mix it up array
    'e' => { 'one' => 1, 'two' => 2 }, # hash
    'f' => nil,     # nil
    #'g' => 12345678901234567890123456789, # big number
    'h' => { 'a' => { 'b' => { 'c' => { 'd' => {'e' => { 'f' => { 'g' => nil }}}}}}}, # deep hash, not that deep
    'i' => [[[[[[[nil]]]]]]]  # deep array, again, not that deep
  }
  $obj['g'] = 12345678901234567890123456789 if $with_bignum
else
  $obj = {
    'a' => 'Alpha',
    'b' => true,
    'c' => '12345',
    'd' => [ true, [false, ['12345', nil], '3.967', ['something', false], nil]],
    'e' => { 'one' => '1', 'two' => '2' },
    'f' => nil,
    'h' => { 'a' => { 'b' => { 'c' => { 'd' => {'e' => { 'f' => { 'g' => nil }}}}}}}, # deep hash, not that deep
    'i' => [[[[[[[nil]]]]]]]  # deep array, again, not that deep
  }
end
$obj['j'] = Jazz.new() if $with_object

Oj.default_options = { :indent => $indent, :mode => :compat }

$json = Oj.dump($obj)
$obj_json = Oj.dump($obj, :mode => :object)
$failed = {} # key is same as String used in tests later

def capture_error(tag, orig, load_key, dump_key, &blk)
  begin
    obj = blk.call(orig)
    raise "#{tag} #{dump_key} and #{load_key} did not return the same object as the original." unless orig == obj
  rescue Exception => e
    $failed[tag] = "#{e.class}: #{e.message}"
  end
end

# Verify that all packages dump and load correctly and return the same Object as the original.
capture_error('Oj:compat', $obj, 'load', 'dump') { |o| Oj.load(Oj.dump(o)) }
capture_error('Oj', $obj, 'load', 'dump') { |o| Oj.load(Oj.dump(o, :mode => :compat), :mode => :compat) }
capture_error('Ox', $obj, 'load', 'dump') { |o|
  require 'ox'
  Ox.default_options = { :indent => $indent, :mode => :object }
  $xml = Ox.dump($obj, :indent => $indent)
  Ox.load(Ox.dump(o, :mode => :object), :mode => :object)
}
capture_error('MessagePack', $obj, 'unpack', 'pack') { |o| require 'msgpack'; MessagePack.unpack(MessagePack.pack($obj)) }
capture_error('Yajl', $obj, 'encode', 'parse') { |o| require 'yajl'; Yajl::Parser.parse(Yajl::Encoder.encode(o)) }
capture_error('JSON::Ext', $obj, 'generate', 'parse') { |o|
  require 'json'
  require 'json/ext'
  JSON.generator = JSON::Ext::Generator
  JSON.parser = JSON::Ext::Parser
  JSON.parse(JSON.generate(o))
}
capture_error('JSON::Pure', $obj, 'generate', 'parse') { |o|
  require 'json/pure'
  JSON.generator = JSON::Pure::Generator
  JSON.parser = JSON::Pure::Parser
  JSON.parse(JSON.generate(o))
}

begin
  $msgpack = MessagePack.pack($obj)
rescue Exception => e
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
unless $failed.has_key?('JSON::Ext')
  perf.add('JSON::Ext', 'parse') { JSON.parse($json) }
  perf.before('JSON::Ext') { JSON.parser = JSON::Ext::Parser }
end
unless $failed.has_key?('JSON::Pure')
  perf.add('JSON::Pure', 'parse') { JSON.parse($json) }
  perf.before('JSON::Pure') { JSON.parser = JSON::Pure::Parser }
end
unless $failed.has_key?('Oj:compat')
  perf.add('Oj:compat', 'load') { Oj.load($json) }
  perf.before('Oj:compat') { Oj.default_options = { :mode => :compat} }
end
unless $failed.has_key?('Oj')
  perf.add('Oj', 'load') { Oj.load($obj_json) }
  perf.before('Oj') { Oj.default_options = { :mode => :object} }
end
perf.add('Yajl', 'parse') { Yajl::Parser.parse($json) } unless $failed.has_key?('Yajl')
perf.add('Ox', 'load') { Ox.load($xml) } unless $failed.has_key?('Ox')
perf.add('MessagePack', 'unpack') { MessagePack.unpack($msgpack) } unless $failed.has_key?('MessagePack')
perf.run($iter)

puts
puts '-' * 80
puts "Dump/Encode/Generate Performance"
perf = Perf.new()
unless $failed.has_key?('JSON::Ext')
  if 0 == $indent
    perf.add('JSON::Ext', 'generate') { JSON.generate($obj) }
  else
    perf.add('JSON::Ext', 'generate') { JSON.pretty_generate($obj) }
  end
  perf.before('JSON::Ext') { JSON.generator = JSON::Ext::Generator }
end
unless $failed.has_key?('JSON::Pure')
  if 0 == $indent
    perf.add('JSON::Pure', 'generate') { JSON.generate($obj) }
  else
    perf.add('JSON::Pure', 'generate') { JSON.pretty_generate($obj) }
  end
  perf.before('JSON::Pure') { JSON.generator = JSON::Pure::Generator }
end
unless $failed.has_key?('Oj')
  perf.add('Oj', 'dump') { Oj.dump($obj) }
  perf.before('Oj') { Oj.default_options = { :mode => :object} }
end
unless $failed.has_key?('Oj:compat')
  perf.add('Oj:compat', 'dump') { Oj.dump($obj) }
  perf.before('Oj:compat') { Oj.default_options = { :mode => :compat} }
end
perf.add('Yajl', 'encode') { Yajl::Encoder.encode($obj) } unless $failed.has_key?('Yajl')
perf.add('Ox', 'dump') { Ox.dump($obj) } unless $failed.has_key?('Ox')
perf.add('MessagePack', 'pack') { MessagePack.pack($obj) } unless $failed.has_key?('MessagePack')
perf.run($iter)

puts

unless $failed.empty?
  puts "The following packages were not included for the reason listed"
  $failed.each { |tag,msg| puts "***** #{tag}: #{msg}" }
end
