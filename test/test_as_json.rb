#!/usr/bin/env ruby
# encoding: UTF-8

$: << File.dirname(__FILE__)

require 'helper'
require 'active_record'
require 'sqlite3'
require 'oj'
ActiveRecord::Base.establish_connection(
    :adapter => "sqlite3",
    :database => ":memory:"
)

ActiveRecord::Schema.define do
  create_table :raccoons do |table|
    table.column :name, :string
    table.column :occupation, :string
  end
end

class ObjectFolder < Minitest::Test
  class Raccoon < ActiveRecord::Base

    def serializable_hash(opts = {})
      super(opts)
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
    raccoon = Raccoon.create(name: 'Rocket', occupation: 'Bounty Hunter')
    obj = Oj.dump(raccoon.to_json)
    assert_equal(obj, '"{\"id\":1,\"name\":\"Rocket\",\"occupation\":\"Bounty Hunter\"}"')
    obj = Oj.dump(raccoon.as_json(include:{}, only:[], methods:[]))
    assert_equal(obj, '{}')
    obj = Oj.dump(raccoon.to_json(include:{}, only:[], methods:[]))
    assert_equal(obj, '"{}"')
  end

end
