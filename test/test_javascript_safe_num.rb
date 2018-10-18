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

class JavascriptSafeNumberTest < Minitest::Test
  def setup
    @default_options = Oj.default_options
    # in null mode other options other than the number formats are not used.
    Oj.default_options = { :mode => :null }
  end

  def teardown
    Oj.default_options = @default_options
  end

  def test_big_num_as_string
    assert_equal('{"safe":9007199254740992,"unsafe":"9007199254740993"}', Oj.dump({safe: 9007199254740992, unsafe: 9007199254740993}, javascript_safe_numbers: true))
  end

  def test_valid_modes
    [:strict, :null, :compat, :rails, :custom].each do |mode|
      assert_equal('{"safe":9007199254740992,"unsafe":"9007199254740993"}', Oj.dump({safe: 9007199254740992, unsafe: 9007199254740993}, mode: mode, javascript_safe_numbers: true), "Invalid mode #{mode}")
    end

    [:object].each do |mode|
      assert_equal('{":safe":9007199254740992,":unsafe":"9007199254740993"}', Oj.dump({safe: 9007199254740992, unsafe: 9007199254740993}, mode: mode, javascript_safe_numbers: true), "Invalid mode #{mode}")
    end
  end

  def test_negative_big_num
    assert_equal('{"safe":-9007199254740992,"unsafe":"-9007199254740993"}', Oj.dump({safe: -9007199254740992, unsafe: -9007199254740993}, javascript_safe_numbers: true))
  end

  def test_without_opt
    assert_equal('{"safe":9007199254740992,"unsafe":9007199254740993}', Oj.dump({safe: 9007199254740992, unsafe: 9007199254740993}, javascript_safe_numbers: false))
    assert_equal('{"safe":9007199254740992,"unsafe":9007199254740993}', Oj.dump({safe: 9007199254740992, unsafe: 9007199254740993}))
    assert_equal('{"safe":-9007199254740992,"unsafe":-9007199254740993}', Oj.dump({safe: -9007199254740992, unsafe: -9007199254740993}, javascript_safe_numbers: false))
    assert_equal('{"safe":-9007199254740992,"unsafe":-9007199254740993}', Oj.dump({safe: -9007199254740992, unsafe: -9007199254740993}))
  end
end

