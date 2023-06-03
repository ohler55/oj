#!/usr/bin/env ruby
$: << File.join(File.dirname(__FILE__), '..')

require 'helper'
require 'isolated/shared'

require 'json'
Oj.mimic_JSON

class MimicAfter < SharedMimicTest
end # MimicAfter
