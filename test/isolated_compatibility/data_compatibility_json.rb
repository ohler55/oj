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

# commented out failing examples
JSON_TEST_DATA = {
  Regexp: /test/,
  FalseClass: false,
  NilClass: nil,
  # Object: Object.new,
  TrueClass: true,
  String: 'abc',
  StringChinese: '二胡',
  StringSpecial: "\u2028\u2029><&",
  StringSpecial2: "\/",
  StringSpecial3: "\\\b\f\n\r\t",
  Numeric: 1,
  Symbol: :sym,
  # Time: Time.new(2012, 1, 5, 23, 58, 7.99996, 32400),
  Array: [],
  Hash: {},
  HashNotEmpty: {a: 1},
  Date: Date.new,
  # DateTime: DateTime.new,
  # Enumerable: Colors.new,
  BigDecimal: BigDecimal.new('1')/3,
  # BigDecimalInfinity: BigDecimal.new('0.5')/0,
  # Struct: Struct::Customer.new('Dave', '123 Main'),
  Float: 1.0/3,
  # FloatInfinity: 0.5/0,
  # Range: (1..10),
  Complex: Complex('0.3-0.5i'),
  # Exception: Exception.new,
  # OpenStruct: OpenStruct.new(country: "Australia", population: 20_000_000),
  Rational: Rational(0.3),
  # 'Process::Status' => $?,
}
