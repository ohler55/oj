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

    MessagePack failed to pack! RangeError: bignum too big to convert into `unsigned long long'.
    Skipping.
    --------------------------------------------------------------------------------
    Load/Parse Performance
    Oj.load 100000 times in 1.384 seconds or 72230.276 load/sec.
    Yajl.parse 100000 times in 2.475 seconds or 40401.331 parse/sec.
    JSON::Ext.parse 100000 times in 2.562 seconds or 39037.263 parse/sec.
    JSON::Pure.parse 100000 times in 20.914 seconds or 4781.518 parse/sec.
    Ox.load 100000 times in 1.517 seconds or 65923.576 load/sec.
    
    Summary:
        System  time (secs)  rate (ops/sec)
    ----------  -----------  --------------
            Oj       1.384       72230.276
            Ox       1.517       65923.576
          Yajl       2.475       40401.331
     JSON::Ext       2.562       39037.263
    JSON::Pure      20.914        4781.518
    
    Comparison Matrix
    (performance factor, 2.0 row is means twice as fast as column)
                        Oj          Ox        Yajl   JSON::Ext  JSON::Pure
    ----------  ----------  ----------  ----------  ----------  ----------
            Oj        1.00        1.10        1.79        1.85       15.11
            Ox        0.91        1.00        1.63        1.69       13.79
          Yajl        0.56        0.61        1.00        1.03        8.45
     JSON::Ext        0.54        0.59        0.97        1.00        8.16
    JSON::Pure        0.07        0.07        0.12        0.12        1.00
    
    
    --------------------------------------------------------------------------------
    Dump/Encode/Generate Performance
    Oj.dump 100000 times in 0.819 seconds or 122096.842 dump/sec.
    Yajl.encode 100000 times in 2.221 seconds or 45014.913 encode/sec.
    JSON::Ext.generate 100000 times in 5.082 seconds or 19678.462 generate/sec.
    ***** JSON::Pure.generate failed! TypeError: wrong argument type JSON::Pure::Generator::State (expected Data)
    Ox.dump 100000 times in 0.532 seconds or 188014.455 dump/sec.
    
    Summary:
       System  time (secs)  rate (ops/sec)
    ---------  -----------  --------------
           Ox       0.532      188014.455
           Oj       0.819      122096.842
         Yajl       2.221       45014.913
    JSON::Ext       5.082       19678.462
    
    Comparison Matrix
    (performance factor, 2.0 row is means twice as fast as column)
                      Ox         Oj       Yajl  JSON::Ext
    ---------  ---------  ---------  ---------  ---------
           Ox       1.00       1.54       4.18       9.55
           Oj       0.65       1.00       2.71       6.20
         Yajl       0.24       0.37       1.00       2.29
    JSON::Ext       0.10       0.16       0.44       1.00

without Objects or numbers (for JSON Pure) JSON:

    --------------------------------------------------------------------------------
    Load/Parse Performance
    Oj.load 100000 times in 0.737 seconds or 135683.185 load/sec.
    Yajl.parse 100000 times in 1.352 seconds or 73978.778 parse/sec.
    JSON::Ext.parse 100000 times in 1.433 seconds or 69780.554 parse/sec.
    JSON::Pure.parse 100000 times in 12.974 seconds or 7707.624 parse/sec.
    Ox.load 100000 times in 0.904 seconds or 110596.591 load/sec.
    MessagePack.unpack 100000 times in 0.644 seconds or 155281.191 unpack/sec.
    
    Summary:
         System  time (secs)  rate (ops/sec)
    -----------  -----------  --------------
    MessagePack       0.644      155281.191
             Oj       0.737      135683.185
             Ox       0.904      110596.591
           Yajl       1.352       73978.778
      JSON::Ext       1.433       69780.554
     JSON::Pure      12.974        7707.624
    
    Comparison Matrix
    (performance factor, 2.0 row is means twice as fast as column)
                 MessagePack           Oj           Ox         Yajl    JSON::Ext   JSON::Pure
    -----------  -----------  -----------  -----------  -----------  -----------  -----------
    MessagePack         1.00         1.14         1.40         2.10         2.23        20.15
             Oj         0.87         1.00         1.23         1.83         1.94        17.60
             Ox         0.71         0.82         1.00         1.49         1.58        14.35
           Yajl         0.48         0.55         0.67         1.00         1.06         9.60
      JSON::Ext         0.45         0.51         0.63         0.94         1.00         9.05
     JSON::Pure         0.05         0.06         0.07         0.10         0.11         1.00
    
    
    --------------------------------------------------------------------------------
    Dump/Encode/Generate Performance
    Oj.dump 100000 times in 0.161 seconds or 620058.906 dump/sec.
    Yajl.encode 100000 times in 0.765 seconds or 130637.498 encode/sec.
    JSON::Ext.generate 100000 times in 3.306 seconds or 30250.212 generate/sec.
    JSON::Pure.generate 100000 times in 7.067 seconds or 14150.026 generate/sec.
    Ox.dump 100000 times in 0.178 seconds or 561312.123 dump/sec.
    MessagePack.pack 100000 times in 0.306 seconds or 326301.535 pack/sec.
    
    Summary:
         System  time (secs)  rate (ops/sec)
    -----------  -----------  --------------
             Oj       0.161      620058.906
             Ox       0.178      561312.123
    MessagePack       0.306      326301.535
           Yajl       0.765      130637.498
      JSON::Ext       3.306       30250.212
     JSON::Pure       7.067       14150.026
    
    Comparison Matrix
    (performance factor, 2.0 row is means twice as fast as column)
                          Oj           Ox  MessagePack         Yajl    JSON::Ext   JSON::Pure
    -----------  -----------  -----------  -----------  -----------  -----------  -----------
             Oj         1.00         1.10         1.90         4.75        20.50        43.82
             Ox         0.91         1.00         1.72         4.30        18.56        39.67
    MessagePack         0.53         0.58         1.00         2.50        10.79        23.06
           Yajl         0.21         0.23         0.40         1.00         4.32         9.23
      JSON::Ext         0.05         0.05         0.09         0.23         1.00         2.14
     JSON::Pure         0.02         0.03         0.04         0.11         0.47         1.00
    
    
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

### Object JSON format:

In :object mode Oj generates JSON that follows conventions which allow Class
and other information such as Object IDs for circular reference detection. The
formating follows the following rules.

1. JSON native types, true, false, nil, String, Hash, Array, and Number are
encoded normally.

2. If a Hash uses Symbols as keys those keys appear as Strings with a leading
':' character.

3. The '^' character denotes a special key value when in a JSON Object sequence.

4. If a String begins with a ':' character such as ':abc' it is encoded as {"^s":":abc"}.

5. If a Symbol begins with a ':' character such as :":abc" is is encoded as {"^m":":abc"}.

6. A "^c" JSON Object key indicates the value should be converted to a Ruby
class. The sequence {"^c":"Oj::Bag"} is read as the Oj::Bag class.

7. A "^t" JSON Object key indicates the value should be converted to a Ruby
Time. The sequence {"^t":1325775487.000000} is read as Jan 5, 2012 at
23:58:07.

8. A "^o" JSON Object key indicates the value should be converted to a Ruby
Object. The first entry in the JSON Object must be a class with the "^o"
key. After that each entry is treated as a variable of the Object where the
key is the variable name without the preceeding '@'. An example is
{"^o":"Oj::Bag","x":58,"y":"marbles"}.

9. A "^u" JSON Object key indicates the value should be converted to a Ruby
Struct. The first entry in the JSON Object must be a class with the "^u"
key. After that each entry is is given a numeric position in the struct and
that is used as the key in the JSON Object. An example is
{"^u":["Range",1,7,false]}.

10. When encoding an Object, if the variable name does not begin with an '@'
character then the name preceeded by a '~' character. This occurs in the
Exception class. An example is {"^o":"StandardError","~mesg":"A
Message","~bt":[".\/tests.rb:345:in `test_exception'"]}

11. If a Hash entry has a key that is not a String or Symbol then the entry is
encoded with a key of the form "^#n" where n is a hex number. The value that
is an Array where the first element is the key in the Hash and the second is
the value. An example is {"^#3":[2,5]}.

12. A "^i" JSON entry in either an Object or Array is the ID of the Ruby
Object being encoded. It is used when the :circular flag is set. It can appear
in either a JSON Object or in a JSON Array. In an Object the "^i" key has a
corresponding reference Fixnum. In an array the sequence will include an
embedded reference number. An example is
{"^o":"Oj::Bag","^i":1,"x":["^i2",true],"me":"^r1"}.

13. A "^r" JSON entry in an Object is a references to a Object or Array that
already appears in the JSON String. It must match up with a previous "^i"
ID. An example is {"^o":"Oj::Bag","^i":1,"x":3,"me":"^r1"}.

14. If an Array element is a String and starts with "^i" then the first character, the ^ is encoded as a hext character.
An example is ["\u005ei37"},3].
