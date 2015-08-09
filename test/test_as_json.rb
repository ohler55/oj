#!/usr/bin/env ruby
# encoding: UTF-8

$: << File.dirname(__FILE__)

require 'helper'
require 'oj'

class ObjectFolder < Minitest::Test
  class Raccoon

    attr_accessor :name

    def initialize(name)
      @name = name
    end

    def as_json(options={})
      {name: @name}.merge(options)
    end
  end

  def setup
    @default_options = Oj.default_options
  end

  def teardown
    Oj.default_options = @default_options
  end

  def test_as_json_options
    Oj.mimic_JSON()
    raccoon = Raccoon.new('Rocket')
    obj = Oj.dump(raccoon.to_json)
    assert_equal(obj, '"{\"name\":\"Rocket\"}"')
    obj = Oj.dump(raccoon.to_json(occupation: 'bounty hunter'))
    assert_equal(obj, '"{\"name\":\"Rocket\",\"occupation\":\"bounty hunter\"}"')
  end

end
