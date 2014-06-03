# Ubuntu does not accept arguments to ruby when called using env. To get warnings to show up the -w options is
# required. That can be set in the RUBYOPT environment variable.
# export RUBYOPT=-w

$VERBOSE = true

$: << File.join(File.dirname(__FILE__), "../lib")
$: << File.join(File.dirname(__FILE__), "../ext")

require 'minitest/autorun'
require 'minitest/around/unit'
require 'stringio'
require 'date'
require 'bigdecimal'
require 'pp'
require 'oj'

$ruby = RUBY_DESCRIPTION.split(' ')[0]
$ruby = 'ree' if 'ruby' == $ruby && RUBY_DESCRIPTION.include?('Ruby Enterprise Edition')

def hash_eql(h1, h2)
  return false if h1.size != h2.size
  h1.keys.each do |k|
    return false unless h1[k] == h2[k]
  end
  true
end
