#!/usr/bin/env ruby
# frozen_string_literal: true

$LOAD_PATH << __dir__

require 'helper'

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

end
