#!/usr/bin/env ruby
# encoding: UTF-8

$: << File.dirname(__FILE__)

require 'helper'

require "active_support" # v5.0.0
require "oj" #v2.17.1

puts "ActiveSupport + OJ.dump(DateTime.now)"
puts
puts Oj.dump(DateTime.now)
puts

require "oj/active_support_helper"
#require 'active_support/time'

puts "ActiveSupport + OJ + active_support_helper dump(DateTime.now)"
puts
puts Oj.dump(DateTime.now)
puts

