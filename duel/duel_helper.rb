# frozen_string_literal: true

require 'bigdecimal'

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
# whole in a few milliseconds.)
def polygon_json
  canada = Oj.load(json_file('canada'), mode: :compat)
  points = canada['features'][0]['geometry']['coordinates'].flatten(1).first(2000)
  Oj.dump({'type' => 'Polygon', 'coordinates' => [points]}, mode: :compat)
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

# A plain Ruby object, as object mode dumps it with its instance variables
# and its class.
class Account
  attr_reader :id, :owner, :balance, :opened, :tags, :limits

  def initialize(i)
    @id = i
    @owner = "Owner #{i}"
    @balance = (i * 10.5) + 0.25
    @opened = Time.utc(2026, 3, 1, 9, 0, i % 60)
    @tags = ['checking', "branch-#{i % 7}"]
    @limits = {'daily' => 500, 'monthly' => 10_000}
  end
end

# A model as Rails dumps one: as_json returns its attributes in a Hash.
class Post
  def initialize(i)
    @attributes = {
      'id' => i,
      'title' => "Post number #{i}",
      'body' => 'Lorem ipsum dolor sit amet, <consectetur> adipiscing & elit. ' * 3,
      'published' => i.odd?,
      'rating' => (i % 5) + 0.5,
      'price' => BigDecimal("#{i}.99"),
      'created_at' => Time.utc(2026, 5, 1, 10, 0, i % 60, 250_000),
      'author' => {'id' => i % 9, 'name' => "Author #{i % 9}"},
    }
  end

  def as_json(_options=nil)
    @attributes
  end
end

# Callbacks for Oj::Parser.new(:saj) and Oj.saj_parse, which do nothing, so
# the parser's own work is what's measured.
class NullSaj
  def hash_start(key)
  end

  def hash_end(key)
  end

  def array_start(key)
  end

  def array_end(key)
  end

  def add_value(value, key)
  end
end

# What Oj.optimize_rails sets up for a Rails app: Oj's own encoding of
# Hash, Array, Float, Time, BigDecimal and the rest in rails mode. (Its
# other steps replace ActiveSupport's encoder and the json gem's methods,
# which needs Rails.) It changes only how rails mode dumps.
Oj::Rails.optimize
