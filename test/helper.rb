# frozen_string_literal: true
#
# Ubuntu does not accept arguments to ruby when called using env. To get warnings to show up the -w options is
# required. That can be set in the RUBYOPT environment variable.
# export RUBYOPT=-w

$VERBOSE = true

%w(lib ext test).each do |dir|
  $LOAD_PATH.unshift File.expand_path("../../#{dir}", __FILE__)
end

require 'minitest'
require 'minitest/autorun'
require 'stringio'
require 'date'
require 'bigdecimal'
require 'pp'
require 'oj'


if defined?(GC.verify_compaction_references) == 'method'
  # This method was added in Ruby 3.0.0. Calling it this way asks the GC to
  # move objects around, helping to find object movement bugs.
  GC.verify_compaction_references(double_heap: true, toward: :empty)
end


$ruby = RUBY_DESCRIPTION.split(' ')[0]
$ruby = 'ree' if 'ruby' == $ruby && RUBY_DESCRIPTION.include?('Ruby Enterprise Edition')

class Range
  def to_hash()
    { 'begin' => self.begin, 'end' => self.end, 'exclude_end' => self.exclude_end? }
  end
end
