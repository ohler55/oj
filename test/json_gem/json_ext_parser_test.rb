#!/usr/bin/env ruby
# encoding: UTF-8

#frozen_string_literal: false

require 'json_gem/test_helper'

class JSONExtParserTest < Test::Unit::TestCase
  include Test::Unit::TestCaseOmissionSupport

  if defined?(JSON::Ext::Parser)
    # json 2.10.0 reimplemented the C parser and dropped the TypeError that
    # re-initializing a parser, or reading the source of an allocated one, used
    # to raise. Upstream rewrote this test at that version. Oj's mimic still
    # raises, so it keeps the old expectation.
    #
    # This has to key on the json version rather than RUBY_VERSION. The Ruby
    # version was only ever a stand-in for "the json that ships with this Ruby",
    # and it stops being one as soon as a Gemfile pulls a newer json onto an
    # older Ruby - which the rails_8.1 gemfile does, because Rails 8.1 depends
    # on json directly.
    if REAL_JSON_GEM && Gem::Version.new(JSON::VERSION) >= Gem::Version.new('2.10.0')
      def test_allocate
        parser = JSON::Ext::Parser.new("{}")
        parser.__send__(:initialize, "{}")
        assert_equal "{}", parser.source

        parser = JSON::Ext::Parser.allocate
        assert_nil parser.source
      end
    else
      # json 2.9.x or before, or Oj mimicking it
      def test_allocate
        parser = JSON::Ext::Parser.new("{}")
        assert_raise(TypeError, '[ruby-core:35079]') do
          parser.__send__(:initialize, "{}")
        end
        parser = JSON::Ext::Parser.allocate
        assert_raise(TypeError, '[ruby-core:35079]') { parser.source }
      end
    end
  end
end
