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
].each do |s|
  x = Oj.load(s)
  puts ">>> #{x}(#{x.class})"
end

iter = 1000000
s = %{
[ true, [false, [true, null], null, [true, false], null]]
}

start = Time.now
iter.times do
  Oj.load(s)
end
dt = Time.now - start
puts "#{iter} Oj.load()s in #{dt} seconds or #{iter/dt} loads/second"

start = Time.now
iter.times do
  Yajl::Parser.parse(s)
end
dt = Time.now - start
puts "#{iter} Yajl::Parser.parse()s in #{dt} seconds or #{iter/dt} parses/second"
