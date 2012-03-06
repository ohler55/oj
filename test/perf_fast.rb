#!/usr/bin/env ruby -wW1
# encoding: UTF-8

$: << '.'
$: << File.join(File.dirname(__FILE__), "../lib")
$: << File.join(File.dirname(__FILE__), "../ext")

require 'optparse'
require 'yajl'
require 'perf'
require 'json'
require 'json/ext'
require 'oj'

$verbose = false
$indent = 0
$iter = 100000
$with_object = true
$with_bignum = true
$with_nums = true

opts = OptionParser.new
opts.on("-v", "verbose")                                    { $verbose = true }
opts.on("-c", "--count [Int]", Integer, "iterations")       { |i| $iter = i }
opts.on("-i", "--indent [Int]", Integer, "indentation")     { |i| $indent = i }
opts.on("-h", "--help", "Show this display")                { puts opts; Process.exit!(0) }
files = opts.parse(ARGV)

$obj = {
  'a' => 'Alpha',
  'b' => true,
  'c' => 12345,
  'd' => [ true, [false, [12345, nil], 3.967, ['something', false], nil]],
  'e' => { 'one' => 1, 'two' => 2 },
  'f' => nil,
  'g' => 12345678901234567890123456789
}

Oj.default_options = { :indent => $indent, :mode => :compat }

$json = Oj.dump($obj)
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
#capture_error('Oj:fast', $obj, 'load', 'dump') { |o| Oj::Fast.new(Oj.dump(o, :mode => :strict)) }
capture_error('Yajl', $obj, 'encode', 'parse') { |o| Yajl::Parser.parse(Yajl::Encoder.encode(o)) }
capture_error('JSON::Ext', $obj, 'generate', 'parse') { |o| JSON.generator = JSON::Ext::Generator; JSON::Ext::Parser.new(JSON.generate(o)).parse }

if $verbose
  puts "json:\n#{$json}\n"
end

puts '-' * 80
puts "Load/Parse Performance"
perf = Perf.new()
perf.add('Oj:fast', 'load') { Oj::Fast.new($json) } unless $failed.has_key?('Oj:fast')
perf.add('Yajl', 'parse') { Yajl::Parser.parse($json) } unless $failed.has_key?('Yajl')
perf.add('JSON::Ext', 'parse') { JSON::Ext::Parser.new($json).parse } unless $failed.has_key?('JSON::Ext')
perf.run($iter)
puts

unless $failed.empty?
  puts "The following packages were not included for the reason listed"
  $failed.each { |tag,msg| puts "***** #{tag}: #{msg}" }
end
