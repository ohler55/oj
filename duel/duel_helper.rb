# frozen_string_literal: true

require 'bigdecimal'
require 'json'
require_relative 'account'
require_relative 'null_saj'
require_relative 'post'

# The documents in test/data are the usual JSON benchmark corpus (the json
# gem's too): twitter.json (631 KB of tweets, much of it Unicode),
# citm_catalog.json (1.7 MB of nested objects, keys repeated throughout),
# activitypub.json (58 KB) and ohai.json (32 KB).
def json_file(name)
  File.read(File.join(__dir__, '..', 'test', 'data', "#{name}.json"))
end

# A typical API response of one record, about 600 bytes.
def small_json
  <<~JSON
    {"id":1042,"type":"user","attributes":{"name":"Jane Doe","email":"jane@example.com",
    "active":true,"admin":false,"score":98.6,"balance":-12.25,"login_count":1723,
    "created_at":"2026-01-15T08:30:00.000Z","updated_at":"2026-10-24T12:00:00.123Z",
    "tags":["ruby","json","c"],"bio":"Likes \\"quotes\\", tabs\\tand caf\\u00e9.",
    "address":{"street":"42 Main Street","city":"Springfield","state":null,"zip":"01234"}},
    "relationships":{"teams":{"data":[{"id":7,"type":"team"},{"id":9,"type":"team"}]}},
    "links":{"self":"https://api.example.com/v1/users/1042"}}
  JSON
end

# The first 2,000 points of canada.json's border, a GeoJSON polygon: almost
# all of it floats of 15 to 17 digits. (canada.json is too large to parse
# whole in a few milliseconds.) Its numbers are cut from the file's text
# as they're written there, so every version gets the same document,
# whatever the Oj being measured would make of them.
def polygon_json
  text = json_file('canada')
  points = text[text.index('[[[')..].scan(/\[(-?[\d.eE+-]+),(-?[\d.eE+-]+)\]/).first(2000)
  %({"type":"Polygon","coordinates":[[#{points.map { |x, y| "[#{x},#{y}]" }.join(',')}]]})
end

# A document as Ruby data, for a benchmark that dumps it: parsed with the
# json gem, not with the Oj being measured, so every version dumps the
# same data.
def parsed(json)
  JSON.parse(json)
end

# 50 Accounts as object mode dumps them, written out in
# fixtures/accounts.json, so every version loads the same document.
def accounts_json
  File.read(File.join(__dir__, 'fixtures', 'accounts.json'))
end

# A record as a Ruby app builds one to dump: symbol keys, floats, Times,
# big integers and a BigDecimal, nested.
def ruby_record(i)
  {
    id: 10_000_000_000_000_000_000 + i,
    name: "User #{i}",
    email: "user#{i}@example.com",
    active: i.even?,
    role: :member,
    score: (i * 1.37) + 0.1,
    ratio: 1.0 / (i + 3),
    balance: BigDecimal("#{i}.25"),
    created_at: Time.utc(2026, 1, 15, 8, 30, 0, 123_456),
    updated_at: Time.utc(2026, 10, 24, 12, 0, i % 60, 500_000),
    tags: [:ruby, :json, "tag-#{i}"],
    address: {street: "#{i} Main Street", city: 'Springfield', state: nil, zip: format('%05d', i)},
  }
end

def ruby_records(count)
  Array.new(count) { |i| ruby_record(i) }
end
