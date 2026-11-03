# frozen_string_literal: true

require_relative 'duel_helper'

# Oj::Parser, the newer parser: a parser is made once and reused for every
# document, the way an app keeps one (Oj::Parser.usual is a shared one).

duel 'parser: usual, a small API response' do
  once do
    @parser = Oj::Parser.new(:usual)
    @json = small_json
    @parser.parse(@json)
  end
  @parser.parse(@json)
end

duel 'parser: usual, activitypub.json' do
  once do
    @parser = Oj::Parser.new(:usual)
    @json = json_file('activitypub')
    @parser.parse(@json)
  end
  @parser.parse(@json)
end

duel 'parser: usual, twitter.json' do
  once do
    @parser = Oj::Parser.new(:usual)
    @json = json_file('twitter')
    @parser.parse(@json)
  end
  @parser.parse(@json)
end

duel 'parser: usual, citm_catalog.json' do
  once do
    @parser = Oj::Parser.new(:usual)
    @json = json_file('citm_catalog')
    @parser.parse(@json)
  end
  @parser.parse(@json)
end

duel 'parser: usual, twitter.json with symbol keys' do
  once do
    @parser = Oj::Parser.new(:usual)
    @parser.symbol_keys = true
    @json = json_file('twitter')
    @parser.parse(@json)
  end
  @parser.parse(@json)
end

duel 'parser: usual, 2,000 GeoJSON points of floats' do
  once do
    @parser = Oj::Parser.new(:usual)
    @json = polygon_json
    @parser.parse(@json)
  end
  @parser.parse(@json)
end

duel 'parser: validate twitter.json' do
  once do
    @parser = Oj::Parser.new(:validate)
    @json = json_file('twitter')
  end
  @parser.parse(@json)
end

duel 'parser: saj callbacks for twitter.json' do
  once do
    @parser = Oj::Parser.new(:saj)
    @parser.handler = NullSaj.new
    @json = json_file('twitter')
    @parser.parse(@json)
  end
  @parser.parse(@json)
end
