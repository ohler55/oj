#!/usr/bin/env ruby
# encoding: UTF-8

$LOAD_PATH << __dir__
@oj_dir = File.dirname(File.expand_path(__dir__))
%w(lib ext).each do |dir|
  $LOAD_PATH << File.join(@oj_dir, dir)
end

require 'test_parser_usual'
require 'test_parser_saj'
