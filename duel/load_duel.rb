# frozen_string_literal: true

require_relative 'duel_helper'

# Oj.load, in the modes that APIs parse with: compat (the json gem's
# behavior, and what MultiJson asks for), strict (JSON types only) and
# rails. Oj caches keys and short strings across parses, so each benchmark
# warms the cache in `once`.

duel 'load: a small API response in compat mode' do
  once do
    @json = small_json
    Oj.load(@json, mode: :compat)
  end
  Oj.load(@json, mode: :compat)
end

duel 'load: ohai.json in compat mode' do
  once do
    @json = json_file('ohai')
    Oj.load(@json, mode: :compat)
  end
  Oj.load(@json, mode: :compat)
end

duel 'load: twitter.json in compat mode' do
  once do
    @json = json_file('twitter')
    Oj.load(@json, mode: :compat)
  end
  Oj.load(@json, mode: :compat)
end

duel 'load: citm_catalog.json in compat mode' do
  once do
    @json = json_file('citm_catalog')
    Oj.load(@json, mode: :compat)
  end
  Oj.load(@json, mode: :compat)
end

duel 'load: a small API response in strict mode' do
  once do
    @json = small_json
    Oj.load(@json, mode: :strict)
  end
  Oj.load(@json, mode: :strict)
end

duel 'load: twitter.json in strict mode' do
  once do
    @json = json_file('twitter')
    Oj.load(@json, mode: :strict)
  end
  Oj.load(@json, mode: :strict)
end

duel 'load: twitter.json in strict mode with symbol keys' do
  once do
    @json = json_file('twitter')
    Oj.load(@json, mode: :strict, symbol_keys: true)
  end
  Oj.load(@json, mode: :strict, symbol_keys: true)
end

duel 'load: activitypub.json in rails mode' do
  once do
    @json = json_file('activitypub')
    Oj.load(@json, mode: :rails)
  end
  Oj.load(@json, mode: :rails)
end

duel 'load: 2,000 GeoJSON points in compat mode' do
  once do
    @json = polygon_json
    Oj.load(@json, mode: :compat)
  end
  Oj.load(@json, mode: :compat)
end

# Strict mode loads a float with more digits than a Float holds as a
# BigDecimal (bigdecimal_load: :auto), as about one in twelve of these is.
duel 'load: 2,000 GeoJSON points in strict mode' do
  once do
    @json = polygon_json
    Oj.load(@json, mode: :strict)
  end
  Oj.load(@json, mode: :strict)
end

duel 'load: Ruby objects in object mode' do
  once do
    @json = accounts_json
    Oj.load(@json, mode: :object)
  end
  Oj.load(@json, mode: :object)
end

# The older callback parsers.
duel 'load: twitter.json through Oj.saj_parse' do
  once do
    @json = json_file('twitter')
    @handler = NullSaj.new
    Oj.saj_parse(@handler, @json)
  end
  Oj.saj_parse(@handler, @json)
end
