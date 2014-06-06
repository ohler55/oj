#!/usr/bin/env ruby
# encoding: UTF-8

$: << File.join(File.dirname(__FILE__), '..')

require 'helper'

Oj.mimic_JSON
require 'rails/all'

require 'isolated/shared'

class MimicRailsBefore < SharedMimicRailsTest
end # MimicRailsBefore
