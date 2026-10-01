#!/usr/bin/env ruby
# frozen_string_literal: true

$LOAD_PATH << __dir__

require 'helper'
require 'rbconfig'

# Each test runs in a new process since whether ostruct has been loaded is
# global and this process may already have loaded it.
class OstructTest < Minitest::Test
  def run_ruby(code)
    lib = File.expand_path('../lib', __dir__)
    ext = File.expand_path('../ext', __dir__)
    IO.popen([RbConfig.ruby, '-I', lib, '-I', ext, '-e', code], err: [:child, :out], &:read)
  end

  def test_oj_does_not_load_ostruct
    out = run_ruby(<<~RUBY)
      require 'oj'
      Oj.dump(Object.new, mode: :custom)
      Oj.mimic_JSON
      Oj.add_to_json
      Oj.add_to_json(Rational)
      JSON.generate(Rational(1, 3))
      print defined?(OpenStruct).inspect
    RUBY
    assert_equal('nil', out)
  end

  def test_openstruct_loaded_after_first_use
    out = run_ruby(<<~RUBY)
      require 'oj'
      opts = { mode: :custom, create_additions: true, create_id: '^o' }
      Oj.load(Oj.dump(Object.new, opts), opts)
      require 'ostruct'
      json = Oj.dump(OpenStruct.new(a: 1), opts)
      print json, ' ', Oj.load(json, opts).a
    RUBY
    assert_equal('{"^o":"OpenStruct","table":{"a":1}} 1', out)
  end

  def test_generic_object_requires_ostruct
    out = run_ruby(<<~RUBY)
      require 'oj'
      Oj.mimic_JSON
      print JSON::GenericObject[a: 1].a
    RUBY
    assert_equal('1', out)
  end
end
