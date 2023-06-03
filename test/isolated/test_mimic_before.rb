#!/usr/bin/env ruby
$: << File.join(File.dirname(__FILE__), '..')

require 'helper'
require 'isolated/shared'

Oj.mimic_JSON
require 'json'

class MimicBefore < SharedMimicTest
end # MimicBefore
