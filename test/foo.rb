#!/usr/bin/env ruby -wW1
# encoding: UTF-8

$: << File.join(File.dirname(__FILE__), "../lib")
$: << File.join(File.dirname(__FILE__), "../ext")

#require 'test/unit'
require 'optparse'
require 'yajl'
require 'oj'

$indent = 2

opts = OptionParser.new
opts.on("-h", "--help", "Show this display")                { puts opts; Process.exit!(0) }
files = opts.parse(ARGV)

[ 'true',
  'false',
  'null',
  '[true, false, null]',
  '[true, [true, false], null]',
  '"string"',
  '["foo", true]',
  '12345',
  '12345.6789',
  '12345.6789e-30',
  '{ "x":-33}',
].each do |s|
  x = Oj.load(s)
  puts ">>> #{x}(#{x.class})"
end

iter = 100000
s = %{
{ "class": "Foo::Bar",
  "attr1": [ true, [false, [12345, null], 3.967, ["something", false], null]],
  "attr2": { "one": 1 }
}
}

start = Time.now
iter.times do
  Oj.load(s)
end
oj_dt = Time.now - start
puts "%d Oj.load()s in %0.3f seconds or %0.1f loads/msec" % [iter, oj_dt, iter/oj_dt/1000.0]

start = Time.now
iter.times do
  Yajl::Parser.parse(s)
end
yajl_dt = Time.now - start
puts "%d Yajl::Parser.parse()s in %0.3f seconds or %0.1f parses/msec" % [iter, yajl_dt, iter/yajl_dt/1000.0]

puts "Oj is %0.1f times faster than YAJL" % [yajl_dt / oj_dt]


obj = Oj.load(s)

start = Time.now
iter.times do
  Oj.dump(obj)
end
oj_dt = Time.now - start
puts "%d Oj.dump()s in %0.3f seconds or %0.1f dumps/msec" % [iter, oj_dt, iter/oj_dt/1000.0]

start = Time.now
iter.times do
  Yajl::Encoder.encode(obj)
end
yajl_dt = Time.now - start
puts "%d Yajl::Encoder.encode()s in %0.3f seconds or %0.1f encodes/msec" % [iter, yajl_dt, iter/yajl_dt/1000.0]

puts "Oj is %0.1f times faster than YAJL" % [yajl_dt / oj_dt]

