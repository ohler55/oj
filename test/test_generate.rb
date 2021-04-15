#!/usr/bin/env ruby
# encoding: utf-8

$: << File.dirname(__FILE__)
$oj_dir = File.dirname(File.expand_path(File.dirname(__FILE__)))
%w(lib ext).each do |dir|
  $: << File.join($oj_dir, dir)
end

require 'minitest'
require 'minitest/autorun'
require 'oj'

class Generator < Minitest::Test

  def test_before
    json = Oj.generate({})
    assert_equal("{}", json)
  end

end
