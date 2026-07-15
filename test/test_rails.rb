#!/usr/bin/env ruby
# frozen_string_literal: true

$LOAD_PATH << __dir__

require 'helper'

# Stand-ins for what ActiveRecord's as_json returns. as_json has already
# resolved the Rails :only/:except selection at the attribute level, so these
# return the finished hash (an include_root_in_json wrapper, or :methods keys).
# No ActiveSupport or database is needed to exercise the #1008 / #1020 paths.
class OjRootWrappedProbe
  def as_json(_options = nil)
    {'vendor' => {'name' => 'v-1'}}
  end
end

class OjMethodsProbe
  def as_json(_options = nil)
    {'name' => 'v-1', 'company_and_code' => 'v-1/c1'}
  end
end

class OjSecretProbe
  def as_json(_options = nil)
    {'name' => 'v-1', 'secret' => 's-1'}
  end
end

class RailsJuice < Minitest::Test

  def test_bigdecimal_dump
    orig = Oj.default_options
    Oj.default_options = { mode: :rails, bigdecimal_as_decimal: true }
    bd = BigDecimal('123')
    json = Oj.dump(bd)
    Oj.default_options = orig

    assert_equal('0.123e3', json.downcase)

    json = Oj.dump(bd, mode: :rails, bigdecimal_as_decimal: false)
    assert_equal('"0.123e3"', json.downcase)

    json = Oj.dump(bd, mode: :rails, bigdecimal_as_decimal: true)
    assert_equal('0.123e3', json.downcase)
  end

  def test_invalid_encoding
    assert_raises(EncodingError) {
      Oj.dump("\"\xf3j", mode: :rails)
    }
    assert_raises(EncodingError) {
      Oj.dump("\xf3j", mode: :rails)
    }
  end

  # ActiveSupport creates a new encoder for every encode call that is given
  # options, so the buffers the :only and :except options allocate in the
  # encoder must be freed when the encoder is collected. The encoders are not
  # kept alive here so that a leak checker sees the buffers as lost.

  def test_encoder_only
    3.times do
      json = Oj::Rails::Encoder.new(only: [:id]).encode({id: 1, secret: 2})
      assert_equal('{"id":1}', json)
    end
  end

  def test_encoder_except
    3.times do
      json = Oj::Rails::Encoder.new(except: [:secret]).encode({id: 1, secret: 2})
      assert_equal('{"id":1}', json)
    end
  end

  # A :match_string option builds a chain of compiled regexps in the encoder
  # that the encoder must free when it is collected.
  def test_encoder_match_string
    3.times do
      json = Oj::Rails::Encoder.new(match_string: {/^x/ => String}).encode({id: 1})
      assert_equal('{"id":1}', json)
    end
  end

  # #1020: to_json(only:) on a record with include_root_in_json wraps each row
  # in a root key. as_json already applied :only to the inner attributes, so the
  # wrapper key must survive - it must not be dropped by the dump level filter.
  def test_1020_only_keeps_as_json_root_wrapper
    json = Oj::Rails::Encoder.new(only: [:name]).encode(OjRootWrappedProbe.new)
    assert_equal('{"vendor":{"name":"v-1"}}', json)
  end

  # #1008: to_json(only:, methods:) - the key added by :methods is not one of the
  # :only attributes, but as_json added it on purpose, so it must survive.
  def test_1008_only_keeps_as_json_methods_key
    json = Oj::Rails::Encoder.new(only: [:name], methods: [:company_and_code]).encode(OjMethodsProbe.new)
    assert_equal('{"name":"v-1","company_and_code":"v-1/c1"}', json)
  end

  # The suppression is scoped to the as_json result: sibling keys of the object
  # that are not selected are still filtered.
  def test_only_still_filters_siblings_of_as_json_value
    json = Oj.dump({'a' => OjMethodsProbe.new, 'b' => 1, 'c' => 2}, mode: :rails, only: ['a', 'b'])
    assert_equal('{"a":{"name":"v-1","company_and_code":"v-1/c1"},"b":1}', json)
  end

  # A StringWriter owns its options struct. Suppressing the filter for an
  # as_json value must not disturb that struct, so :only keeps working for the
  # value and for every later push_value on the same writer (and nothing leaks).
  def test_string_writer_rails_only_survives_as_json
    w = Oj::StringWriter.new(mode: :rails, only: ['name'])
    w.push_object
    w.push_value(OjSecretProbe.new, 'a')
    w.push_value({'name' => 1, 'secret' => 2}, 'b')
    w.pop
    assert_equal(%|{"a":{"name":"v-1"},"b":{"name":1}}\n|, w.to_s)
  end

end
