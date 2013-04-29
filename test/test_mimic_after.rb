#!/usr/bin/env ruby
# encoding: UTF-8

# Ubuntu does not accept arguments to ruby when called using env. To get warnings to show up the -w options is
# required. That can be set in the RUBYOPT environment variable.
# export RUBYOPT=-w

$VERBOSE = true

$: << File.join(File.dirname(__FILE__), "../lib")
$: << File.join(File.dirname(__FILE__), "../ext")

require 'test/unit'
require 'stringio'
require 'oj'
require 'json'

class MimicAfter < ::Test::Unit::TestCase

  def test0_mimic_json
    assert(!defined?(JSON).nil?)
    Oj.mimic_JSON
  end

# dump
  def test_dump_string
    Oj.default_options= {:indent => 2} # JSON this will not change anything
    json = JSON.dump([1, true, nil])
    assert_equal(%{[
  1,
  true,
  null]}, json)
  end

end # MimicAfter
