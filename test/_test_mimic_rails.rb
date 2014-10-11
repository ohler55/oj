#!/usr/bin/env ruby
# encoding: UTF-8

$: << File.dirname(__FILE__)

require 'helper'
Oj.mimic_JSON
require 'rails/all'

require 'active_model'
require 'active_model_serializers'
require 'active_support/json'

class Category
  include ActiveModel::Model
  include ActiveModel::SerializerSupport

  attr_accessor :id, :name

  def initialize(id, name)
    @id   = id
    @name = name
  end
end

class CategorySerializer < ActiveModel::Serializer
  attributes :id, :name
end

class MimicRails < Minitest::Test

  def test_mimic_exception
    begin
      ActiveSupport::JSON.decode("{")
      puts "Failed"
    rescue ActiveSupport::JSON.parse_error
      assert(true)
    rescue Exception
      assert(false, 'Expected a JSON::ParserError')
    end
  end

  def test_dump_string
    Oj.default_options= {:indent => 2} # JSON this will not change anything
    json = ActiveSupport::JSON.encode([1, true, nil])
    assert_equal(%{[
  1,
  true,
  null
]
}, json)
  end

  def test_dump_rational
    Oj.default_options= {:indent => 2} # JSON this will not change anything
    Oj.default_options= {:use_to_json => true}
    json = ActiveSupport::JSON.encode([1, true, Rational(1)])
    assert_equal(%{[
  1,
  true,
  "1/1"
]
}, json)
  end

  def test_dump_object
    Oj.default_options= {:indent => 2}
    Oj.default_options= {:use_to_json => true}
    category = Category.new(1, 'test')
    serializer = CategorySerializer.new(category)

    json = serializer.to_json()
    puts "*** to_json (#{json.class})#{json}"
    json = serializer.as_json()
    puts "*** as_json (#{json.class})#{json}"
    json = JSON.dump(serializer)
    puts "*** dump (#{json.class})#{json}"
  end


end # MimicRails
