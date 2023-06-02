#!/usr/bin/env ruby -wW2

if $0 == __FILE__
  $LOAD_PATH << '.'
  $LOAD_PATH << '..'
  $LOAD_PATH << '../lib'
  $LOAD_PATH << '../ext'
end

require 'sample/file'
require 'sample/dir'

def files(dir)
  d = ::Sample::Dir.new(dir)
  Dir.new(dir).each do |fn|
    next if fn.start_with?('.')

    filename = File.join(dir, fn)
    # filename = '.' == dir ? fn : File.join(dir, fn)
    if File.directory?(filename)
      d << files(filename)
    else
      d << ::Sample::File.new(filename)
    end
  end
  # pp d
  d
end
