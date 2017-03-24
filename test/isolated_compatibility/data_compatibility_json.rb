# encoding: UTF-8

require 'bigdecimal'

# test data
class Colors
  include Enumerable
  def each
    yield 'red'
    yield 'green'
    yield 'blue'
  end
end

Struct.new('Customer', :name, :address)

fork { exit 99 }
Process.wait

class JsonRenderable
  # This method should not be called by to_json so Oj should also ignore it in
  # compat mode.
  def as_json(options = {})
    hash = { a: :b, c: :d, e: :f }
    hash.except!(*options[:except]) if options[:except]
    hash
  end

  def to_json(options = {})
    super except: [:c, :e]
  end
end

# commented out failing examples
TEST_DATA_TO_JSON = {
  regexp: /test/,
  false_class: false,
  nil_class: nil,
  object: Object.new,
  true_class: true,
  string_1: '',
  string_2: 0x1.chr,
  string_3: 0x1f.chr,
  string_4: ' ',
  string_5: 0x7f.chr,
  String_Chinese: '二胡',
  string_special_1: "\u2028\u2029><&",
  string_special_2: "\/",
  string_special_3: "\\\b\f\n\r\t",
  numeric: 1,
  symbol: :sym,
  time: Time.new(2012, 1, 5, 23, 58, 7.99996, 32400),
  array: [],
  hash: {},
  hash_not_empty: {a: 1},
  date: Date.new,
  date_time: DateTime.new,
  enumerable: Colors.new,
  big_decimal: BigDecimal.new('1')/3,
  big_decimal_infinity: BigDecimal.new('0.5')/0,
  struct: Struct::Customer.new('Dave', '123 Main'),
  float: 1.0/3,
  float_infinity: 0.5/0,
  range: (1..10),
  complex: Complex('0.3-0.5i'),
  exception: Exception.new,
  open_struct: OpenStruct.new(country: "Australia", population: 20_000_000),
  rational: Rational(0.3),
  process_status: $?,
  json_renderable: JsonRenderable.new, #TODO, except: [:c, :e]
}

ary = []; ary << ary
too_deep = '[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[["Too deep"]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]'
too_deep_ary = eval too_deep
s = Class.new(String) do
  def to_s; self; end
  undef to_json
end

utf_8      = '"© ≠ €!"'
ascii_8bit = utf_8.dup.force_encoding('ascii-8bit')
if String.method_defined?(:encode)
  utf_16be = utf_8.encode('utf-16be', 'utf-8')
  utf_16le = utf_8.encode('utf-16le', 'utf-8')
  utf_32be = utf_8.encode('utf-32be', 'utf-8')
  utf_32le = utf_8.encode('utf-32le', 'utf-8')
else
  require 'iconv'
  utf_16be, = Iconv.iconv('utf-16be', 'utf-8', utf_8)
  utf_16le, = Iconv.iconv('utf-16le', 'utf-8', utf_8)
  utf_32be, = Iconv.iconv('utf-32be', 'utf-8', utf_8)
  utf_32le, = Iconv.iconv('utf-32le', 'utf-8', utf_8)
end

TEST_DATA_GENERATE = {
  nan: [JSON::NaN],
  nan_array: [[JSON::NaN]],
  nan_allow: [[JSON::NaN], { allow_nan: true }],
  infinity: [JSON::MinusInfinity],
  infinity_array: [[JSON::MinusInfinity]],
  infinity_allow: [[JSON::MinusInfinity], { allow_nan: true }],
  nested_array: [ary],
  too_deep_array: [too_deep_ary],
  too_deep_array_100: [too_deep_ary, { max_nesting: 100 }],
  toodeep_array_101: [too_deep_ary, { max_nesting: 101 }],
  too_deep_array_nil: [too_deep_ary, { max_nesting: nil }],
  too_deep_array_false: [too_deep_ary, { max_nesting: false }],
  too_deep_array_0: [too_deep_ary, { max_nesting: 0 }],
  system_stack_error: [[s]],
  ascii_8bit_true:  [[ascii_8bit], { ascii_only: true }],
  utf_16be_true:    [[utf_16be], { ascii_only: true }],
  utf_16le_true:    [[utf_16le], { ascii_only: true }],
  utf_32be_true:    [[utf_32be], { ascii_only: true }],
  utf_32le_true:    [[utf_32le], { ascii_only: true }],
  ascii_8bit_false: [[ascii_8bit], { ascii_only: false }],
  utf_16be_false:   [[utf_16be], { ascii_only: false }],
  utf_16le_false:   [[utf_16le], { ascii_only: false }],
  utf_32be_false:   [[utf_32be], { ascii_only: false }],
  utf_32le_false:   [[utf_32le], { ascii_only: false }],
  string_1:  [["© ≠ €! \01"], { ascii_only: true }],
  string_2:  [["© ≠ €! \01"], { ascii_only: false }],
  string_3:  [["\343\201\202\343\201\204\343\201\206\343\201\210\343\201\212"], { ascii_only: true }],
  string_4:  [["\343\201\202\343\201\204\343\201\206\343\201\210\343\201\212"], { ascii_only: false }],
  string_5: [["საქართველო"], { ascii_only: true }],
  string_6: [["საქართველო"], { ascii_only: false }],
  string_7: [["Ã"], { ascii_only: true }],
  string_8: [["Ã"], { ascii_only: false }],
  string_9: [["\xf0\xa0\x80\x81"], { ascii_only: true }],
  string_10: [["\xf0\xa0\x80\x81"], { ascii_only: false }],
  string_11: [["\xea"], { ascii_only: true }],
  string_12: [["\xea"], { ascii_only: false }],
  string_13: [["\x80"], { ascii_only: true }],
}
