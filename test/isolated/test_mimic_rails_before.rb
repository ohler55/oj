#!/usr/bin/env ruby
# encoding: UTF-8

$: << File.join(File.dirname(__FILE__), '..')

require 'helper'

Oj.mimic_JSON
begin
  require 'rails/all'
rescue Exception
  puts "*** require of rails failed"
  Process.exit!(true)
end

require 'isolated/shared'

class MimicRailsBefore < SharedMimicRailsTest
end # MimicRailsBefore

