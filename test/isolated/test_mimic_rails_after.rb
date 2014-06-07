#!/usr/bin/env ruby
# encoding: UTF-8

$: << File.join(File.dirname(__FILE__), '..')

require 'helper'

begin
  require 'rails/all'
rescue Exception
  Process.exit!(true)
end

Oj.mimic_JSON

require 'isolated/shared'

class MimicRailsAfter < SharedMimicRailsTest
end # MimicRailsAfter
