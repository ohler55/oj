#!/usr/bin/env ruby
# encoding: UTF-8

$: << File.join(File.dirname(__FILE__), '..')

require 'helper'

begin
  require 'rails/all'
rescue Exception => e
  puts "*** #{e.class}: #{e.message}"
  Process.exit!(false)
end

Oj.mimic_JSON

require 'isolated/shared'

class MimicRailsAfter < SharedMimicRailsTest
end # MimicRailsAfter
