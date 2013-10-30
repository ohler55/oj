#!/usr/bin/env ruby
# encoding: UTF-8

# Ubuntu does not accept arguments to ruby when called using env. To get warnings to show up the -w options is
# required. That can be set in the RUBYOPT environment variable.
# export RUBYOPT=-w

$VERBOSE = true

$: << File.join(File.dirname(__FILE__), "../lib")
$: << File.join(File.dirname(__FILE__), "../ext")

require 'test/unit'
require 'stringio'
require 'date'
require 'bigdecimal'
require 'oj'

$ruby = RUBY_DESCRIPTION.split(' ')[0]
$ruby = 'ree' if 'ruby' == $ruby && RUBY_DESCRIPTION.include?('Ruby Enterprise Edition')

class TestEscapingEntities < ::Test::Unit::TestCase
  def test_does_not_escape_entities_by_default
    hash = {'key' => "I <3 this"}
    out = Oj.dump hash
    assert_equal "{\"key\":\"I <3 this\"}", out
  end

  def test_escapes_entities_by_default_when_configured_to_do_so
    hash = {'key' => "I <3 this"}
    Oj.default_options = {:escape_entities => true}
    out = Oj.dump hash
    assert_equal "{\"key\":\"I \\u003c3 this\"}", out
  end

  def test_escapes_entities_when_asked_to
    hash = {'key' => "I <3 this"}
    out = Oj.dump hash, escape_entities: true
    assert_equal "{\"key\":\"I \\u003c3 this\"}", out
  end

  def test_does_not_escape_entities_when_not_asked_to
    hash = {'key' => "I <3 this"}
    out = Oj.dump hash, escape_entities: false
    assert_equal "{\"key\":\"I <3 this\"}", out
  end

  def test_escapes_common_xss_vectors
    hash = {'key' => "<script>alert(123) && formatHD()</script>"}
    out = Oj.dump hash, escape_entities: true
    assert_equal "{\"key\":\"\\u003cscript\\u003ealert(123) \\u0026\\u0026 formatHD()\\u003c/script\\u003e\"}", out
  end
end
