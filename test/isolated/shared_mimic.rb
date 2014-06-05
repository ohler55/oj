# encoding: UTF-8

class SharedMimicTest < Minitest::Test
  class Jam
    attr_accessor :x, :y

    def initialize(x, y)
      @x = x
      @y = y
    end

    def eql?(o)
      self.class == o.class && @x == o.x && @y == o.y
    end
    alias == eql?

    def as_json()
      {"json_class" => self.class.to_s,"x" => @x,"y" => @y}
    end

    def self.json_create(h)
      self.new(h['x'], h['y'])
    end

  end # Jam

  def around
    opts = Oj.default_options
    yield
    Oj.default_options = opts
  end

# exception
  def test_exception
    begin
      JSON.parse("{")
      puts "Failed"
    rescue JSON::ParserError
      assert(true)
    rescue Exception
      assert(false, 'Expected a JSON::ParserError')
    end
  end

# dump
  def test_dump_string
    json = JSON.dump([1, true, nil])
    assert_equal(%{[1,true,null]}, json)
  end

  def test_dump_with_options
    Oj.default_options= {:indent => 2} # JSON this will not change anything
    json = JSON.dump([1, true, nil])
    assert_equal(%{[
  1,
  true,
  null
]
}, json)
  end

  def test_dump_io
    s = StringIO.new()
    json = JSON.dump([1, true, nil], s)
    assert_equal(s, json)
    assert_equal(%{[1,true,null]}, s.string)
  end
  # TBD options

# load
  def test_load_string
    json = %{{"a":1,"b":[true,false]}}
    obj = JSON.load(json)
    assert_equal({ 'a' => 1, 'b' => [true, false]}, obj)
  end

  def test_load_io
    json = %{{"a":1,"b":[true,false]}}
    obj = JSON.load(StringIO.new(json))
    assert_equal({ 'a' => 1, 'b' => [true, false]}, obj)
  end

  def test_load_proc
    Oj.mimic_JSON # TBD
    children = []
    json = %{{"a":1,"b":[true,false]}}
    if 'rubinius' == $ruby || '1.8.7' == RUBY_VERSION
      obj = JSON.load(json) {|x| children << x }
    else
      p = Proc.new {|x| children << x }
      obj = JSON.load(json, p)
    end
    assert_equal({ 'a' => 1, 'b' => [true, false]}, obj)
    assert([1, true, false, [true, false], { 'a' => 1, 'b' => [true, false]}] == children ||
           [true, false, [true, false], 1, { 'a' => 1, 'b' => [true, false]}] == children,
           "children don't match")
  end

# []
  def test_bracket_load
    json = %{{"a":1,"b":[true,false]}}
    obj = JSON[json]
    assert_equal({ 'a' => 1, 'b' => [true, false]}, obj)
  end

  def test_bracket_dump
    json = JSON[[1, true, nil]]
    assert_equal(%{[1,true,null]}, json)
  end

# generate
  def test_generate
    json = JSON.generate({ 'a' => 1, 'b' => [true, false]})
    assert(%{{"a":1,"b":[true,false]}} == json ||
           %{{"b":[true,false],"a":1}} == json)
  end
  def test_generate_options
    json = JSON.generate({ 'a' => 1, 'b' => [true, false]},
                         :indent => "--",
                         :array_nl => "\n",
                         :object_nl => "#\n",
                         :space => "*",
                         :space_before => "~")
    assert(%{{#
--"a"~:*1,#
--"b"~:*[
----true,
----false
--]#
}} == json ||
%{{#
--"b"~:*[
----true,
----false
--],#
--"a"~:*1#
}} == json)

  end

# fast_generate
  def test_fast_generate
    json = JSON.generate({ 'a' => 1, 'b' => [true, false]})
    assert(%{{"a":1,"b":[true,false]}} == json ||
           %{{"b":[true,false],"a":1}} == json)
  end

# pretty_generate
  def test_pretty_generate
    json = JSON.pretty_generate({ 'a' => 1, 'b' => [true, false]})
    assert(%{{
  "a" : 1,
  "b" : [
    true,
    false
  ]
}} == json ||
%{{
  "b" : [
    true,
    false
  ],
  "a" : 1
}} == json)
  end

# parse
  def test_parse
    json = %{{"a":1,"b":[true,false]}}
    obj = JSON.parse(json)
    assert_equal({ 'a' => 1, 'b' => [true, false]}, obj)
  end
  def test_parse_sym_names
    json = %{{"a":1,"b":[true,false]}}
    obj = JSON.parse(json, :symbolize_names => true)
    assert_equal({ :a => 1, :b => [true, false]}, obj)
  end
  def test_parse_additions
    jam = Jam.new(true, 58)
    json = Oj.dump(jam, :mode => :compat)
    obj = JSON.parse(json)
    assert_equal(jam, obj)
    obj = JSON.parse(json, :create_additions => true)
    assert_equal(jam, obj)
    obj = JSON.parse(json, :create_additions => false)
    assert_equal({'json_class' => 'SharedMimicTest::Jam', 'x' => true, 'y' => 58}, obj)
    json.gsub!('json_class', 'kson_class')
    JSON.create_id = 'kson_class'
    obj = JSON.parse(json, :create_additions => true)
    JSON.create_id = 'json_class'
    assert_equal(jam, obj)
  end
  def test_parse_bang
    json = %{{"a":1,"b":[true,false]}}
    obj = JSON.parse!(json)
    assert_equal({ 'a' => 1, 'b' => [true, false]}, obj)
  end

# recurse_proc
  def test_recurse_proc
    children = []
    JSON.recurse_proc({ 'a' => 1, 'b' => [true, false]}) { |x| children << x }
    # JRuby 1.7.0 rb_yield() is broken and converts the [true, falser] array into true
    unless 'jruby' == $ruby && '1.9.3' == RUBY_VERSION
      assert([1, true, false, [true, false], { 'a' => 1, 'b' => [true, false]}] == children ||
             [true, false, [true, false], 1, { 'b' => [true, false], 'a' => 1}] == children)
    end
  end
end # Mimic
