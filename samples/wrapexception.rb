#!/usr/bin/env ruby
# encoding: UTF-8

# Ubuntu does not accept arguments to ruby when called using env. To get warnings to show up the -w options is
# required. That can be set in the RUBYOPT environment variable.
# export RUBYOPT=-w

$VERBOSE = true

#$: << File.join(File.dirname(__FILE__), "../lib")
#$: << File.join(File.dirname(__FILE__), "../ext")

require 'oj'

# Oj is not able to automatically deserialize all classes that are a subclass of
# a Ruby Exception. Only exception that take one required string argument in the
# initialize() method are supported. This is an example of how to write an
# Exception subclass that supports both a single string initializer and an
# Exception as an argument. Additional optional arguments can be added as well.
#
# The reason for this restriction has to do with a design decision on the part
# of the Ruby developers. Exceptions are special Objects. They do not follow the
# rules of other Objects. Exceptions have 'mesg' and a 'bt' attribute. Note that
# these are not '@mesg' and '@bt'. They can not be set using the normal C or
# Ruby calls. The only way I have found to set the 'mesg' attribute is through
# the initializer. Unfortunately that means any subclass that provides a
# different initializer can not be automatically decoded. A way around this is
# to use a create function but this example shows an alternative.

class WrapException < StandardError
  attr_reader :original

  def initialize(msg_or_err)
    if msg_or_err.is_a?(Exception)
      super(msg_or_err.message)
      @original = msg_or_err
      set_backtrace(msg_or_err.backtrace)
    else
      super(message)
      @original = nil
    end
  end
end

e = WrapException.new(RuntimeError.new("Something broke."))

json = Oj.dump(e, :mode => :object)
puts "original:\n#{json}"
# outputs:
# original:
# {"^o":"WrapException","original":{"^o":"RuntimeError","~mesg":"Something broke.","~bt":null},"~mesg":"Something broke.","~bt":null}

e2 = Oj.load(json, :mode => :object)
puts "dumped, loaded, and dumped again:\n#{Oj.dump(e2, :mode => :object)}"
# outputs:
# original: {"^o":"WrapException","original":{"^o":"RuntimeError","~mesg":"Something broke.","~bt":null},"~mesg":"Something broke.","~bt":null}
# dumped, loaded, and dumped again:
# {"^o":"WrapException","original":{"^o":"RuntimeError","~mesg":"Something broke.","~bt":null},"~mesg":"Something broke.","~bt":null}

