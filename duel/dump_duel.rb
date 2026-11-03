# frozen_string_literal: true

require_relative 'duel_helper'

# Oj.dump, of what a parse returns (string keys, JSON types only) and of
# what an app builds itself (symbol keys, Times, BigDecimals, big
# integers), in the modes apps dump with.

duel 'dump: a small API response in compat mode' do
  once { @data = parsed(small_json) }
  Oj.dump(@data, mode: :compat)
end

duel 'dump: twitter.json in compat mode' do
  once { @data = parsed(json_file('twitter')) }
  Oj.dump(@data, mode: :compat)
end

duel 'dump: citm_catalog.json in compat mode' do
  once { @data = parsed(json_file('citm_catalog')) }
  Oj.dump(@data, mode: :compat)
end

duel 'dump: twitter.json in strict mode' do
  once { @data = parsed(json_file('twitter')) }
  Oj.dump(@data, mode: :strict)
end

duel 'dump: twitter.json in compat mode, indented' do
  once { @data = parsed(json_file('twitter')) }
  Oj.dump(@data, mode: :compat, indent: 2)
end

duel 'dump: 2,000 GeoJSON points of floats' do
  once { @data = parsed(polygon_json) }
  Oj.dump(@data, mode: :compat)
end

duel 'dump: 50 Ruby records in compat mode' do
  once { @data = ruby_records(50) }
  Oj.dump(@data, mode: :compat)
end

duel 'dump: 50 Ruby records in custom mode' do
  once { @data = ruby_records(50) }
  Oj.dump(@data, mode: :custom, time_format: :xmlschema)
end

duel 'dump: 50 Ruby records in rails mode' do
  once { @data = ruby_records(50) }
  Oj.dump(@data, mode: :rails)
end

duel 'dump: 50 Ruby records in object mode' do
  once { @data = ruby_records(50) }
  Oj.dump(@data, mode: :object)
end

duel 'dump: 50 Ruby objects in object mode' do
  once { @data = Array.new(50) { |i| Account.new(i) } }
  Oj.dump(@data, mode: :object)
end

# The json gem's generate, which Oj also has. (Given only the object, it
# calls Oj.mimic_JSON first, which would change Oj's default options for
# every benchmark after it, so this passes the options too.)
duel 'generate: twitter.json' do
  once { @data = parsed(json_file('twitter')) }
  Oj.generate(@data, {})
end
