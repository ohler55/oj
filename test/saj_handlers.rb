#!/usr/bin/env ruby
# frozen_string_literal: true

# The handler test_saj.rb and test_parser_saj.rb both use. Not named test_*.rb
# so that it is not collected as a suite of its own.

require 'oj'

class AllSaj < Oj::Saj
  attr_accessor :calls

  def initialize
    @calls = []

    super
  end

  def hash_start(key)
    @calls << [:hash_start, key]
  end

  def hash_end(key)
    @calls << [:hash_end, key]
  end

  def array_start(key)
    @calls << [:array_start, key]
  end

  def array_end(key)
    @calls << [:array_end, key]
  end

  def add_value(value, key)
    @calls << [:add_value, value, key]
  end

  def error(message, line, column)
    @calls << [:error, message, line, column]
  end

end # AllSaj
