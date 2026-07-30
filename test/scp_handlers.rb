#!/usr/bin/env ruby
# frozen_string_literal: true

# The handlers test_scp.rb and test_scp_fork.rb both use. Not named test_*.rb
# so that it is not collected as a suite of its own.

require 'oj'

class NoHandler < Oj::ScHandler
end

class AllHandler < Oj::ScHandler
  attr_accessor :calls

  def initialize
    super
    @calls = []
  end

  def hash_start
    @calls << [:hash_start]
    {}
  end

  def hash_end
    @calls << [:hash_end]
  end

  def hash_key(key)
    @calls << [:hash_key, key]
    return 'too' if 'two' == key
    return :symbol if 'symbol' == key

    key
  end

  def array_start
    @calls << [:array_start]
    []
  end

  def array_end
    @calls << [:array_end]
  end

  def add_value(value)
    @calls << [:add_value, value]
  end

  def hash_set(_h, key, value)
    @calls << [:hash_set, key, value]
  end

  def array_append(_a, value)
    @calls << [:array_append, value]
  end

end # AllHandler

class Closer < AllHandler
  attr_accessor :io

  def initialize(io)
    super()
    @io = io
  end

  def hash_start
    @calls << [:hash_start]
    @io.close
    {}
  end

  def hash_set(_h, key, value)
    @calls << [:hash_set, key, value]
    @io.close
  end

end # Closer
