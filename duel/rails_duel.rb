# frozen_string_literal: true

require_relative 'duel_helper'

# What a Rails app's to_json and render json: call once Oj.optimize_rails
# has replaced ActiveSupport's encoder: Oj::Rails.encode, with the
# optimizations duel_helper.rb turns on. Rails mode escapes HTML entities
# in strings, and calls as_json on models.

duel 'rails: encode a small API response' do
  once { @data = Oj.load(small_json, mode: :strict) }
  Oj::Rails.encode(@data)
end

duel 'rails: encode twitter.json' do
  once { @data = Oj.load(json_file('twitter'), mode: :strict) }
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
  once { @data = Oj.load(small_json, mode: :strict) }
  Oj::Rails::Encoder.new(nil).encode(@data)
end
