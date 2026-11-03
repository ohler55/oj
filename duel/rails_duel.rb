# frozen_string_literal: true

require_relative 'duel_helper'

# What a Rails app's to_json and render json: call once Oj.optimize_rails
# has replaced ActiveSupport's encoder: Oj::Rails.encode, with Oj's own
# encoding of Hash, Array, Float, Time, BigDecimal and the rest turned
# on. (Oj.optimize_rails's other steps replace ActiveSupport's encoder
# and the json gem's methods, which needs Rails.) Rails mode escapes HTML
# entities in strings, and calls as_json on models.
#
# Oj::Rails.optimize changes the whole process, compat mode's Floats
# included, so it's on only for these benchmarks: every other one
# measures Oj as it is without it.
setup { Oj::Rails.optimize }
teardown { Oj::Rails.deoptimize }

duel 'rails: encode a small API response' do
  once { @data = parsed(small_json) }
  Oj::Rails.encode(@data)
end

duel 'rails: encode twitter.json' do
  once { @data = parsed(json_file('twitter')) }
  Oj::Rails.encode(@data)
end

duel 'rails: encode 50 models through as_json' do
  once { @posts = Array.new(50) { |i| Post.new(i) } }
  Oj::Rails.encode(@posts)
end

duel 'rails: encode 50 Ruby records' do
  once { @data = ruby_records(50) }
  Oj::Rails.encode(@data)
end

# ActiveSupport makes a new encoder for every call to encode.
duel 'rails: a new Encoder for each small response' do
  once { @data = parsed(small_json) }
  Oj::Rails::Encoder.new(nil).encode(@data)
end
