#!/usr/bin/env ruby
# encoding: UTF-8

$: << File.join(File.dirname(__FILE__), '..')

require 'helper'

require 'rails/all'
Oj.mimic_JSON

require 'isolated/shared'

class MimicRailsAfter < SharedMimicRailsTest
end # MimicRailsAfter
