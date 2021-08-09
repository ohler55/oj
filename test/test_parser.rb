#!/usr/bin/env ruby
# encoding: utf-8

$: << File.dirname(__FILE__)
$oj_dir = File.dirname(File.expand_path(File.dirname(__FILE__)))
%w(lib ext).each do |dir|
  $: << File.join($oj_dir, dir)
end

require 'minitest'
require 'minitest/autorun'
require 'stringio'
require 'date'
require 'bigdecimal'
require 'oj'

class ParserJuice < Minitest::Test

  def test_array
    p = Oj::Parser.new(:debug)
    out = p.parse(%|[true, false, null, 123, -1.23, "abc"]|)
    puts out
    out = p.parse(%|{"abc": []}|)
    puts out
  end

end
