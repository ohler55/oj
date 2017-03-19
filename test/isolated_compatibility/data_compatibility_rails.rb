# encoding: UTF-8

require 'data_compatibility_json.rb'
require 'rails/all'
require 'sqlite3'

require 'oj'

if Rails::VERSION::MAJOR == 5 && RUBY_VERSION == "2.2.3"
  # see https://github.com/ohler55/oj/commit/050b4c70836394cffd96b63388ff0dedb8ed3558
  require 'oj/active_support_helper'
end

ActiveRecord::Base.logger = Logger.new(STDERR)
ActiveRecord::Base.logger.level = 1

ActiveRecord::Base.establish_connection(
  adapter: 'sqlite3',
  database:':memory:'
)

ActiveRecord::Migration.verbose = false
ActiveRecord::Schema.define do
  create_table :users do |table|
    table.column :name, :string
  end
end

class User < ActiveRecord::Base
  validates :name, presence: true
end

user = User.new
user.valid?

# commented out failing examples
RAILS_TEST_DATA = {
  Regexp: /test/,
  FalseClass: false,
  NilClass: nil,
  Object: Object.new,
  TrueClass: true,
  String: 'abc',
  StringChinese: '二胡',
  StringSpecial: "\u2028\u2029><&",
  StringSpecial2: "\/",
  StringSpecial3: "\\\b\f\n\r\t",
  Numeric: 1,
  Symbol: :sym,
  Time: Time.new(2012, 1, 5, 23, 58, 7.99996, 32400),
  Array: [],
  Hash: {},
  HashNotEmpty: {a: 1},
  Date: Date.new,
  DateTime: DateTime.new,
  Enumerable: Colors.new,
  BigDecimal: BigDecimal.new('1')/3,
  BigDecimalInfinity: BigDecimal.new('0.5')/0,
  Struct: Struct::Customer.new('Dave', '123 Main'),
  Float: 1.0/3,
  FloatInfinity: 0.5/0,
  Range: (1..10),
  Complex: Complex('0.3-0.5i'),
  # Exception: Exception.new,
  OpenStruct: OpenStruct.new(country: "Australia", population: 20_000_000),
  Rational: Rational(0.3),
  # 'Process::Status' => $?,
  # JsonRenderable: JsonRenderable.new, #TODO, except: [:c, :e]
  # 'ActiveSupport::TimeWithZone' => Time.utc(2005,2,1,15,15,10).in_time_zone('Hawaii'),
  # 'ActiveModel::Errors' => user.errors,
  # 'ActiveSupport::Duration' => 1.month.ago,
  # 'ActiveSupport::Multibyte::Chars' => 'über'.mb_chars,
  # 'ActiveRecord::Relation' => User.where(name: 'aaa'),
  # 'ActiveRecord' => User.find_or_create_by(name: "John"),
  # 'ActiveSupport::OrderedHash': ActiveSupport::OrderedHash[:foo, :bar],
  # 'ActionDispatch::Journey::GTG::TransitionTable' => TODO,
}
