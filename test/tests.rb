#!/usr/bin/env ruby
# encoding: UTF-8

$: << File.dirname(__FILE__)
$oj_dir = File.dirname(File.expand_path(File.dirname(__FILE__)))
%w(lib ext).each do |dir|
  $: << File.join($oj_dir, dir)
end

require 'test_compat'
require 'test_custom'
require 'test_fast'
require 'test_file'
require 'test_gc'
require 'test_hash'
require 'test_null'
require 'test_object'
require 'test_saj'
require 'test_scp'
require 'test_strict'
require 'test_rails'
require 'test_wab'
require 'test_writer'
require 'test_integer_range'

at_exit do
  require 'helper'
  if '3.1.0' <= RUBY_VERSION && !(RbConfig::CONFIG['host_os'] =~ /(mingw|mswin)/)
    #Oj::debug_odd("teardown before GC.verify_compaction_references")
    verify_gc_compaction
    #Oj::debug_odd("teardown after GC.verify_compaction_references")
  end
end
