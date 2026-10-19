#!/usr/bin/env ruby
# frozen_string_literal: true

$LOAD_PATH << __dir__

require 'helper'
require 'open3'
require 'rbconfig'

# Each test runs in a new process since whether ostruct has been loaded is
# global and this process may already have loaded it.
class OstructTest < Minitest::Test
  # Only stdout is compared. Under ruby_memcheck the child inherits
  # RUBY_FREE_AT_EXIT and warns about it on stderr.
  def run_ruby(code)
    lib = File.expand_path('../lib', __dir__)
    ext = File.expand_path('../ext', __dir__)
    out, err, status = Open3.capture3(RbConfig.ruby, '-I', lib, '-I', ext, '-e', code)
    assert_predicate(status, :success?, "exit status #{status.exitstatus}\n#{err}")
    out
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

  def test_openstruct_loaded_after_first_rails_encode
    out = run_ruby(<<~RUBY)
      require 'oj'
      Oj.add_to_json
      encoder = Oj::Rails::Encoder.new
      encoder.encode(Object.new)
      Oj.dump(Object.new, mode: :rails)
      require 'ostruct'
      print encoder.encode(OpenStruct.new(a: 1)), ' ', Oj.dump(OpenStruct.new(a: 1), mode: :rails)
    RUBY
    assert_equal('{"json_class":"OpenStruct","t":{"a":1}} {"json_class":"OpenStruct","t":{"a":1}}', out)
  end

  def test_openstruct_loaded_after_first_writer_push
    out = run_ruby(<<~RUBY)
      require 'oj'
      require 'stringio'
      Oj.add_to_json
      class Plain; def to_s; 'plain'; end; end
      writer = Oj::StringWriter.new(mode: :compat)
      writer.push_array
      writer.push_value(Plain.new)
      require 'ostruct'
      writer.push_value(OpenStruct.new(a: 1))
      writer.pop
      io = StringIO.new
      stream = Oj::StreamWriter.new(io, mode: :compat)
      stream.push_array
      stream.push_value(OpenStruct.new(a: 1))
      stream.pop
      stream.flush
      print writer.to_s.strip, ' ', io.string.strip
    RUBY
    assert_equal('["plain",{"json_class":"OpenStruct","t":{"a":1}}] [{"json_class":"OpenStruct","t":{"a":1}}]', out)
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
