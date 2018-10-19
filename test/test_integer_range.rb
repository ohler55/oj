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

class IntegerRangeTest < Minitest::Test
  def setup
    @default_options = Oj.default_options
    # in null mode other options other than the number formats are not used.
    Oj.default_options = { :mode => :null }
  end

  def teardown
    Oj.default_options = @default_options
  end

  def test_range
    test = {s: 0, s2: -1, s3: 1, u: -2, u2: 2, u3: 9007199254740993}
    exp = '{"s":0,"s2":-1,"s3":1,"u":"-2","u2":"2","u3":"9007199254740993"}'
    assert_equal(exp, Oj.dump(test, integer_range: (-1..1)))
  end

  def test_max_safe
    test = {s: 9007199254740993, s2: -9007199254740993, u: -9007199254740994, u2: 9007199254740994}
    exp = '{"s":"9007199254740993","s2":"-9007199254740993","u":"-9007199254740994","u2":"9007199254740994"}'
    assert_equal(exp, Oj.dump(test, integer_range: :max_safe))
  end

  def test_bignum
    test = {u: -10000000000000000000, u2: 10000000000000000000}
    exp = '{"u":"-10000000000000000000","u2":"10000000000000000000"}'
    assert_equal(exp, Oj.dump(test, integer_range: (-1..1)))
  end

  def test_valid_modes
    test = {safe: 0, unsafe: 9007199254740993}
    exp  = '{"safe":0,"unsafe":"9007199254740993"}'

    [:strict, :null, :compat, :rails, :custom].each do |mode|
      assert_equal(exp, Oj.dump(test, mode: mode, integer_range: (-1..1)), "Invalid mode #{mode}")
    end

    exp = '{":safe":0,":unsafe":"9007199254740993"}'
    [:object].each do |mode|
      assert_equal(exp, Oj.dump(test, mode: mode, integer_range: (-1..1)), "Invalid mode #{mode}")
    end
  end

  def test_modes_without_opt
    test = {safe:0, unsafe: 10000000000000000000}
    exp = '{"safe":0,"unsafe":10000000000000000000}'

    [:strict, :null, :compat, :rails, :custom].each do |mode|
      assert_equal(exp, Oj.dump(test, mode: mode), "Invalid mode #{mode}")
    end

    exp = '{":safe":0,":unsafe":"10000000000000000000"}'
    [:object].each do |mode|
      assert_equal(exp, Oj.dump(test, mode: mode, integer_range: (-1..1)), "Invalid mode #{mode}")
    end
  end
end

