#!/usr/bin/env ruby
# encoding: UTF-8

#frozen_string_literal: false

require 'json_gem/test_helper'

class JSONExtParserTest < Test::Unit::TestCase
  include Test::Unit::TestCaseOmissionSupport

  if defined?(JSON::Ext::Parser)
    if REAL_JSON_GEM && Gem::Version.new(RUBY_VERSION) >= Gem::Version.new('3.5.0')
      def test_allocate
        parser = JSON::Ext::Parser.new("{}")
        parser.__send__(:initialize, "{}")
        assert_equal "{}", parser.source

        parser = JSON::Ext::Parser.allocate
        assert_nil parser.source
      end
    else
      # Ruby 3.4.x or before
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
