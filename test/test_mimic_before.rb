#!/usr/bin/env ruby
# encoding: UTF-8

# Ubuntu does not accept arguments to ruby when called using env. To get warnings to show up the -w options is
# required. That can be set in the RUBYOPT environment variable.
# export RUBYOPT=-w

$VERBOSE = true

$: << File.join(File.dirname(__FILE__), "../lib")
$: << File.join(File.dirname(__FILE__), "../ext")

require 'test/unit'
require 'oj'
require 'oj_mimic_json'
#Oj.mimic_JSON
require 'json'

class MimicBefore < ::Test::Unit::TestCase

  # correct exception
  def test_mimic_exception
    begin
      JSON.parse("{")
      puts "Failed"
    rescue JSON::ParserError
      assert(true)
    rescue Exception
      assert(false, 'Expected a JSON::ParserError')
    end
  end

end # MimicAfter
