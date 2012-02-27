# Oj gem
A fast JSON parser and Object marshaller as a Ruby gem.

## <a name="installation">Installation</a>
    gem install oj

## <a name="source">Source</a>

*GitHub* *repo*: https://github.com/ohler55/oj

*RubyGems* *repo*: https://rubygems.org/gems/oj

## <a name="build_status">Build Status</a>

[![Build Status](https://secure.travis-ci.org/ohler55/oj.png?branch=master)](http://travis-ci.org/ohler55/oj)

## <a name="links">Links of Interest</a>

*Fast XML parser and marshaller on RubyGems*: https://rubygems.org/gems/ox

*Fast XML parser and marshaller on GitHub*: https://rubygems.org/gems/ox

## <a name="release">Release Notes</a>

### Release 0.8.0

- Auto creation of data classes when unmarshalling Objects if the Class is not defined

## <a name="description">Description</a>

Optimized JSON (Oj), as the name implies was written to provide speed
optimized JSON handling. It was designed as a faster alternative to Yajl and
other the common Ruby JSON parsers. So far is has achieved that at about 2
time faster than Yajl for parsing and 3 or more times faster writing JSON.

Oj has several dump or serialization modes which control how Objects are
converted to JSON. These modes are set with the :effort option in either the
default options or as one of the options to the dump() method.

- :strict mode will only allow the 7 basic JSON types to be serialized. Any other Object
will raise and Exception. 

- :null mode replaces any Object that is not one of the JSON types is replaced by a JSON null.

- :object mode will dump any Object as a JSON Object with keys that match the
Ruby Object's variable names without the '@' character. This is the highest
performance mode.

- :compat mode is is the compatible with other systems. It will serialize any
Object but will check to see if the Object implements a to_hash() or to_json()
method. If either exists that method is used for serializing the Object. The
to_hash() is more flexible and produces more consistent output so it has a
preference over the to_json() method. If neither the to_json() or to_hash()
methods exist then the Oj internal Object variable encoding is used.

Oj is compatible with Ruby 1.8.7, 1.9.2, 1.9.3, JRuby, and RBX.

## <a name="plans">Planned Releases</a>

- Release 0.9: Support for circular references.

- Release 1.0: A JSON stream parser.

## <a name="compare">Comparisons</a>

The following table shows the difference is speeds between several
serialization packages. The tests had to be scaled back due to limitation of
some of the gems. I finally gave up trying to get JSON Pure to serialize
without errors with Ruby 1.9.3. It had internal errors on anything other than
a simple JSON structure. The errors encountered were:

- MessagePack fails to convert Bignum to JSON

- JSON Pure fails to serialize any numbers or Objects with the to_json() method

Options were added to the test/perf_strict.rb test to run the test without
Object encoding and without Bignums.

None of the packages except Oj were able to serialize Ruby Objects that did
not have a to_json() method or were of the 7 native JSON types.

A perf_obj.rb file was added for comparing different Object marshalling
packages.

It is also worth noting that although Oj is slightly behind MessagePack for
parsing, Oj serialization is much faster than MessagePack even though Oj uses
human readable JSON vs the binary MessagePack format.

The results:

with Object and Bignum encoding:

    100000 Oj.load()s in 1.456 seconds or 68.7 loads/msec
    100000 Yajl::Parser.parse()s in 2.681 seconds or 37.3 parses/msec
    100000 JSON::Ext::Parser parse()s in 2.804 seconds or 35.7 parses/msec
    100000 JSON::Pure::Parser parse()s in 27.494 seconds or 3.6 parses/msec
    MessagePack failed: RangeError: bignum too big to convert into `unsigned long long'
    100000 Ox.load()s in 3.165 seconds or 31.6 loads/msec
    Parser results:
    gem       seconds  parses/msec  X faster than JSON::Pure (higher is better)
    oj        1.456     68.7        18.9
    yajl      2.681     37.3        10.3
    msgpack failed to generate JSON
    pure     27.494      3.6         1.0
    ext       2.804     35.7         9.8
    ox        3.165     31.6         8.7
    
    100000 Oj.dump()s in 0.484 seconds or 206.7 dumps/msec
    100000 Yajl::Encoder.encode()s in 2.167 seconds or 46.2 encodes/msec
    JSON::Ext failed: TypeError: wrong argument type JSON::Pure::Generator::State (expected Data)
    JSON::Pure failed: TypeError: wrong argument type JSON::Pure::Generator::State (expected Data)
    MessagePack failed: RangeError: bignum too big to convert into `unsigned long long'
    100000 Ox.dump()s in 0.554 seconds or 180.4 dumps/msec
    Parser results:
    gem       seconds  dumps/msec  X faster than Yajl (higher is better)
    oj        0.484    206.7        4.5
    yajl      2.167     46.2        1.0
    msgpack failed to generate JSON
    pure failed to generate JSON
    ext failed to generate JSON
    ox        0.554    180.4        3.9

without Objects or numbers (for JSON Pure) JSON:

    100000 Oj.load()s in 0.739 seconds or 135.3 loads/msec
    100000 Yajl::Parser.parse()s in 1.421 seconds or 70.4 parses/msec
    100000 JSON::Ext::Parser parse()s in 1.512 seconds or 66.2 parses/msec
    100000 JSON::Pure::Parser parse()s in 16.953 seconds or 5.9 parses/msec
    100000 MessagePack.unpack()s in 0.635 seconds or 157.6 packs/msec
    100000 Ox.load()s in 0.971 seconds or 103.0 loads/msec
    Parser results:
    gem       seconds  parses/msec  X faster than JSON::Pure (higher is better)
    oj        0.739    135.3        22.9
    yajl      1.421     70.4        11.9
    msgpack   0.635    157.6        26.7
    pure     16.953      5.9         1.0
    ext       1.512     66.2        11.2
    ox        0.971    103.0        17.5
    
    100000 Oj.dump()s in 0.174 seconds or 575.1 dumps/msec
    100000 Yajl::Encoder.encode()s in 0.729 seconds or 137.2 encodes/msec
    100000 JSON::Ext generate()s in 7.171 seconds or 13.9 generates/msec
    100000 JSON::Pure generate()s in 7.219 seconds or 13.9 generates/msec
    100000 Msgpack()s in 0.299 seconds or 334.8 unpacks/msec
    100000 Ox.dump()s in 0.210 seconds or 475.8 dumps/msec
    Parser results:
    gem       seconds  dumps/msec  X faster than JSON::Pure (higher is better)
    oj        0.174    575.1       41.5
    yajl      0.729    137.2        9.9
    msgpack   0.299    334.8       24.2
    pure      7.219     13.9        1.0
    ext       1.512     66.2        4.8
    ox        0.210    475.8       34.3

### Simple JSON Writing and Parsing:

    require 'oj'
    
    h = { 'one' => 1, 'array' => [ true, false ] }
    json = Oj.dump(h)
    
    # json =
    # {
    #   "one":1,
    #   "array":[
    #     true,
    #     false
    #   ]
    # }

    h2 = Oj.parse(json)
    puts "Same? #{h == h2}"
    # true
