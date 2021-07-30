#!/usr/bin/env ruby
# encoding: UTF-8

$: << '.'
$: << File.join(File.dirname(__FILE__), "../lib")
$: << File.join(File.dirname(__FILE__), "../ext")

require 'optparse'
require 'perf'
require 'oj'

$verbose = false
$indent = 0
$iter = 50_000
$with_bignum = false
$size = 1
$cache_keys = true
$thread_safe = false

opts = OptionParser.new
opts.on("-v", "verbose")                                  { $verbose = true }
opts.on("-c", "--count [Int]", Integer, "iterations")     { |i| $iter = i }
opts.on("-i", "--indent [Int]", Integer, "indentation")   { |i| $indent = i }
opts.on("-s", "--size [Int]", Integer, "size (~Kbytes)")  { |i| $size = i }
opts.on("-b", "with bignum")                              { $with_bignum = true }
opts.on("-k", "no cache")                                 { $cache_keys = false }
opts.on("-r", "ractor safe")                              { $thread_safe = true }
opts.on("-h", "--help", "Show this display")              { puts opts; Process.exit!(0) }
files = opts.parse(ARGV)

$obj = {
  'a' => 'Alpha', # string
  'b' => true,    # boolean
  'c' => 12345,   # number
  'd' => [ true, [false, [-123456789, nil], 3.9676, ['Something else.', false], nil]], # mix it up array
  'e' => { 'zero' => nil, 'one' => 1, 'two' => 2, 'three' => [3], 'four' => [0, 1, 2, 3, 4] }, # hash
  'f' => nil,     # nil
  'h' => { 'a' => { 'b' => { 'c' => { 'd' => {'e' => { 'f' => { 'g' => nil }}}}}}}, # deep hash, not that deep
  'i' => [[[[[[[nil]]]]]]]  # deep array, again, not that deep
}
$obj['g'] = 12345678901234567890123456789 if $with_bignum

if 0 < $size
  o = $obj
  $obj = []
  (4 * $size).times do
    $obj << o
  end
end

$json = Oj.dump($obj)
$failed = {} # key is same as String used in tests later

class AllSaj
  def initialize()
  end

  def hash_start(key)
  end

  def hash_end(key)
  end

  def array_start(key)
  end

  def array_end(key)
  end

  def add_value(value, key)
  end
end # AllSaj

class NoSaj
  def initialize()
  end
end # NoSaj

no_handler = NoSaj.new()
all_handler = AllSaj.new()

if $verbose
  puts "json:\n#{$json}\n"
end

### Validate ######################
p_val = Oj::Parser.new(:validate)

puts '-' * 80
puts "Validate Performance"
perf = Perf.new()
perf.add('Oj::Parser.validate', 'none') { p_val.parse($json) }
perf.add('Oj::Saj.none', 'none') { Oj.saj_parse(no_handler, $json) }
perf.run($iter)

### SAJ ######################
p_all = Oj::Parser.new(:saj)
p_all.handler = all_handler
p_all.cache_keys = $cache_keys
p_all.thread_safe = $thread_safe
p_all.cache_strings = 5

puts '-' * 80
puts "Parse Callback Performance"
perf = Perf.new()
perf.add('Oj::Parser.saj', 'all') { p_all.parse($json) }
perf.add('Oj::Saj.all', 'all') { Oj.saj_parse(all_handler, $json) }
perf.run($iter)

### Usual ######################
p_usual = Oj::Parser.new(:usual)
p_usual.cache_keys = $cache_keys
p_usual.thread_safe = $thread_safe
p_usual.cache_strings = 5

=begin
puts '-' * 80
puts "Parse Usual Performance"
perf = Perf.new()
perf.add('Oj::Parser.usual', '') { p_usual.parse($json) }
perf.add('Oj::strict_load', '') { Oj.strict_load($json) }
perf.run($iter)
=end

unless $failed.empty?
  puts "The following packages were not included for the reason listed"
  $failed.each { |tag,msg| puts "***** #{tag}: #{msg}" }
end
