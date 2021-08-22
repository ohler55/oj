#!/usr/bin/env ruby
# encoding: UTF-8

$: << '.'
$: << File.join(File.dirname(__FILE__), "../lib")
$: << File.join(File.dirname(__FILE__), "../ext")

require 'oj'

filename = 'tmp.json'
File.open(filename, "w") { |f|
  cnt = 0
  f.puts('{')
  ('a'..'z').each { |a|
    ('a'..'z').each { |b|
      ('a'..'z').each { |c|
	('a'..'z').each { |d|
	  f.puts(%|"#{a}#{b}#{c}#{d}":#{cnt},|)
	  cnt += 1
	}
      }
    }
  }
  f.puts('"_last":0}')
}

Oj.default_options = { mode: :strict, cache_keys: false, cache_str: -1 }
#Oj.load_file('tmp.json')
start = Time.now
Oj.load_file('tmp.json')
dur = Time.now - start
puts "duration: #{dur}"
