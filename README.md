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

[Need for Speed](http://www.ohler.com/dev/need_for_speed/need_for_speed.html) for an overview of how Oj::Doc was designed.

*Fast XML parser and marshaller on RubyGems*: https://rubygems.org/gems/ox

*Fast XML parser and marshaller on GitHub*: https://github.com/ohler55/ox

## <a name="release">Release Notes</a>

### Release 1.2.8

 - Included a contribution by nevans to fix a math.h issue with an old fedora linux machine.

 - Included a fix to the documentation found by mat.

## <a name="description">Description</a>

Optimized JSON (Oj), as the name implies was written to provide speed
optimized JSON handling. It was designed as a faster alternative to Yajl and
other the common Ruby JSON parsers. So far is has achieved that at about 2
time faster than Yajl for parsing and 3 or more times faster writing JSON.

Oj has several dump or serialization modes which control how Objects are
converted to JSON. These modes are set with the :mode option in either the
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

Oj is compatible with Ruby 1.8.7, 1.9.2, 1.9.3, JRuby, RBX, and the latest 2.0dev.

Oj is also compatible with Rails. Just make sure the Oj gem is installed and
[multi_json](https://github.com/intridea/multi_json) will pick it up and use it.

## <a name="compare">Comparisons</a>

### Fast Oj::Doc parser comparisons

The fast Oj::Doc parser is compared to the Yajl and JSON::Pure parsers with
strict JSON documents. No object conversions are included, just simple JSON.

Since the Oj::Doc deviation from the conventional parsers comparisons of not
only parsing but data access is also included. These tests use the
perf_fast.rb test file. The first benchmark is for just parsing. The second is
for doing a get on every leaf value in the JSON data structure. The third
fetchs a value from a specific spot in the document. With Yajl and JSON this
is done with a set of calls to fetch() for each level in the document. For
Oj::Doc a single fetch with a path is used.

The benchmark results are:

    > perf_fast.rb -g 1 -f
    --------------------------------------------------------------------------------
    Parse Performance
    Oj::Doc.parse 100000 times in 0.164 seconds or 609893.696 parse/sec.
    Yajl.parse 100000 times in 3.168 seconds or 31569.902 parse/sec.
    JSON::Ext.parse 100000 times in 3.282 seconds or 30464.826 parse/sec.
    
    Summary:
       System  time (secs)  rate (ops/sec)
    ---------  -----------  --------------
      Oj::Doc       0.164      609893.696
         Yajl       3.168       31569.902
    JSON::Ext       3.282       30464.826
    
    Comparison Matrix
    (performance factor, 2.0 row is means twice as fast as column)
                 Oj::Doc       Yajl  JSON::Ext
    ---------  ---------  ---------  ---------
      Oj::Doc       1.00      19.32      20.02
         Yajl       0.05       1.00       1.04
    JSON::Ext       0.05       0.96       1.00
    
    --------------------------------------------------------------------------------
    Parse and get all values Performance
    Oj::Doc.parse 100000 times in 0.417 seconds or 240054.540 parse/sec.
    Yajl.parse 100000 times in 5.159 seconds or 19384.191 parse/sec.
    JSON::Ext.parse 100000 times in 5.269 seconds or 18978.638 parse/sec.
    
    Summary:
       System  time (secs)  rate (ops/sec)
    ---------  -----------  --------------
      Oj::Doc       0.417      240054.540
         Yajl       5.159       19384.191
    JSON::Ext       5.269       18978.638
    
    Comparison Matrix
    (performance factor, 2.0 row is means twice as fast as column)
                 Oj::Doc       Yajl  JSON::Ext
    ---------  ---------  ---------  ---------
      Oj::Doc       1.00      12.38      12.65
         Yajl       0.08       1.00       1.02
    JSON::Ext       0.08       0.98       1.00
    
    --------------------------------------------------------------------------------
    fetch nested Performance
    Oj::Doc.fetch 100000 times in 0.094 seconds or 1059995.760 fetch/sec.
    Ruby.fetch 100000 times in 0.503 seconds or 198851.434 fetch/sec.
    
    Summary:
     System  time (secs)  rate (ops/sec)
    -------  -----------  --------------
    Oj::Doc       0.094     1059995.760
       Ruby       0.503      198851.434
    
    Comparison Matrix
    (performance factor, 2.0 row is means twice as fast as column)
             Oj::Doc     Ruby
    -------  -------  -------
    Oj::Doc     1.00     5.33
       Ruby     0.19     1.00

What the results mean are that for getting just a few values from a JSON
document Oj::Doc is 20 times faster than any other parser and for accessing
all values it is still over 12 times faster than any other Ruby JSON parser.

### Conventional Oj parser comparisons

The following table shows the difference is speeds between several
serialization packages compared to the more conventional Oj parser. The tests
had to be scaled back due to limitation of some of the gems. I finally gave up
trying to get JSON Pure to serialize without errors with Ruby 1.9.3. It had
internal errors on anything other than a simple JSON structure. The errors
encountered were:

- MessagePack fails to convert Bignum to JSON

- JSON Pure fails to serialize any numbers or Objects with the to_json() method

Options were added to the test/perf_strict.rb test to run the test without
Object encoding and without Bignums.

None of the packages except Oj were able to serialize Ruby Objects that did
not have a to_json() method or were of the 7 native JSON types.

A perf_obj.rb file was added for comparing different Object marshalling
packages.

It is also worth noting that although Oj is just slightly behind MessagePack for
parsing.

Oj supports circular references when in :object mode and when the :circular
flag is true. None of the other gems tested supported circular
references. They failed in the following manners when the input included
circular references.

 - Yajl core dumps Ruby

 - JSON fails and raises an Exception

 - MessagePack fails and raises an Exception

The benchmark results are:

with Object and Bignum encoding:

    > perf_strict.rb 
    --------------------------------------------------------------------------------
    Load/Parse Performance
    Oj:compat.load 100000 times in 1.481 seconds or 67513.146 load/sec.
    Oj.load 100000 times in 1.066 seconds or 93796.400 load/sec.
    JSON::Ext.parse 100000 times in 3.023 seconds or 33074.875 parse/sec.
    JSON::Pure.parse 100000 times in 18.908 seconds or 5288.799 parse/sec.
    Ox.load 100000 times in 1.240 seconds or 80671.900 load/sec.
    
    Summary:
        System  time (secs)  rate (ops/sec)
    ----------  -----------  --------------
            Oj       1.066       93796.400
            Ox       1.240       80671.900
     Oj:compat       1.481       67513.146
     JSON::Ext       3.023       33074.875
    JSON::Pure      18.908        5288.799
    
    Comparison Matrix
    (performance factor, 2.0 row is means twice as fast as column)
                        Oj          Ox   Oj:compat   JSON::Ext  JSON::Pure
    ----------  ----------  ----------  ----------  ----------  ----------
            Oj        1.00        1.16        1.39        2.84       17.73
            Ox        0.86        1.00        1.19        2.44       15.25
     Oj:compat        0.72        0.84        1.00        2.04       12.77
     JSON::Ext        0.35        0.41        0.49        1.00        6.25
    JSON::Pure        0.06        0.07        0.08        0.16        1.00
    
    
    --------------------------------------------------------------------------------
    Dump/Encode/Generate Performance
    Oj:compat.dump 100000 times in 0.789 seconds or 126715.249 dump/sec.
    Oj.dump 100000 times in 0.457 seconds or 218798.751 dump/sec.
    JSON::Ext.generate 100000 times in 4.371 seconds or 22878.630 generate/sec.
    Ox.dump 100000 times in 0.501 seconds or 199425.256 dump/sec.
    
    Summary:
       System  time (secs)  rate (ops/sec)
    ---------  -----------  --------------
           Oj       0.457      218798.751
           Ox       0.501      199425.256
    Oj:compat       0.789      126715.249
    JSON::Ext       4.371       22878.630
    
    Comparison Matrix
    (performance factor, 2.0 row is means twice as fast as column)
                      Oj         Ox  Oj:compat  JSON::Ext
    ---------  ---------  ---------  ---------  ---------
           Oj       1.00       1.10       1.73       9.56
           Ox       0.91       1.00       1.57       8.72
    Oj:compat       0.58       0.64       1.00       5.54
    JSON::Ext       0.10       0.11       0.18       1.00
    
    
    The following packages were not included for the reason listed
    ***** MessagePack: RangeError: bignum too big to convert into `unsigned long long'
    ***** Yajl: RuntimeError: Yajl parse and encode did not return the same object as the original.
    ***** JSON::Pure: TypeError: wrong argument type JSON::Pure::Generator::State (expected Data)

without Objects or numbers (for JSON Pure, Yajl, and Messagepack) JSON:

    > perf_strict.rb -n -o
    --------------------------------------------------------------------------------
    Load/Parse Performance
    JSON::Ext.parse 10000 times in 0.314 seconds or 31797.615 parse/sec.
    Oj:compat.load 10000 times in 0.194 seconds or 51415.203 load/sec.
    Oj.load 10000 times in 0.186 seconds or 53834.352 load/sec.
    Yajl.parse 10000 times in 0.310 seconds or 32292.962 parse/sec.
    Ox.load 10000 times in 0.215 seconds or 46474.232 load/sec.
    MessagePack.unpack 10000 times in 0.166 seconds or 60156.165 unpack/sec.
    
    Summary:
         System  time (secs)  rate (ops/sec)
    -----------  -----------  --------------
    MessagePack       0.166       60156.165
             Oj       0.186       53834.352
      Oj:compat       0.194       51415.203
             Ox       0.215       46474.232
           Yajl       0.310       32292.962
      JSON::Ext       0.314       31797.615
    
    Comparison Matrix
    (performance factor, 2.0 means row is twice as fast as column)
                 MessagePack           Oj    Oj:compat           Ox         Yajl    JSON::Ext
    -----------  -----------  -----------  -----------  -----------  -----------  -----------
    MessagePack         1.00         1.12         1.17         1.29         1.86         1.89
             Oj         0.89         1.00         1.05         1.16         1.67         1.69
      Oj:compat         0.85         0.96         1.00         1.11         1.59         1.62
             Ox         0.77         0.86         0.90         1.00         1.44         1.46
           Yajl         0.54         0.60         0.63         0.69         1.00         1.02
      JSON::Ext         0.53         0.59         0.62         0.68         0.98         1.00
    
    
    --------------------------------------------------------------------------------
    Dump/Encode/Generate Performance
    JSON::Ext.generate 10000 times in 0.457 seconds or 21863.563 generate/sec.
    Oj.dump 10000 times in 0.271 seconds or 36862.418 dump/sec.
    Oj:compat.dump 10000 times in 0.279 seconds or 35894.012 dump/sec.
    Yajl.encode 10000 times in 0.122 seconds or 81863.877 encode/sec.
    Ox.dump 10000 times in 0.291 seconds or 34399.843 dump/sec.
    MessagePack.pack 10000 times in 0.072 seconds or 138632.803 pack/sec.
    
    Summary:
         System  time (secs)  rate (ops/sec)
    -----------  -----------  --------------
    MessagePack       0.072      138632.803
           Yajl       0.122       81863.877
             Oj       0.271       36862.418
      Oj:compat       0.279       35894.012
             Ox       0.291       34399.843
      JSON::Ext       0.457       21863.563
    
    Comparison Matrix
    (performance factor, 2.0 means row is twice as fast as column)
                 MessagePack         Yajl           Oj    Oj:compat           Ox    JSON::Ext
    -----------  -----------  -----------  -----------  -----------  -----------  -----------
    MessagePack         1.00         1.69         3.76         3.86         4.03         6.34
           Yajl         0.59         1.00         2.22         2.28         2.38         3.74
             Oj         0.27         0.45         1.00         1.03         1.07         1.69
      Oj:compat         0.26         0.44         0.97         1.00         1.04         1.64
             Ox         0.25         0.42         0.93         0.96         1.00         1.57
      JSON::Ext         0.16         0.27         0.59         0.61         0.64         1.00
    
    
    The following packages were not included for the reason listed
    ***** JSON::Pure: RuntimeError: can't modify frozen object
    
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

    h2 = Oj.load(json)
    puts "Same? #{h == h2}"
    # true

### Object JSON format:

In :object mode Oj generates JSON that follows conventions which allow Class
and other information such as Object IDs for circular reference detection. The
formating follows the following rules.

1. JSON native types, true, false, nil, String, Hash, Array, and Number are
encoded normally.

2. A Symbol is encoded as a JSON string with a preceeding `:` character.

3. The `^` character denotes a special key value when in a JSON Object sequence.

4. A Ruby String that starts with `:` or the sequence `^i` or `^r` are encoded by
excaping the first character so that it appears as `\u005e` or `\u003a` instead of
`:` or `^`.

5. A `"^c"` JSON Object key indicates the value should be converted to a Ruby
class. The sequence `{"^c":"Oj::Bag"}` is read as the Oj::Bag class.

6. A `"^t"` JSON Object key indicates the value should be converted to a Ruby
Time. The sequence `{"^t":1325775487.000000}` is read as Jan 5, 2012 at 23:58:07.

87. A `"^o"` JSON Object key indicates the value should be converted to a Ruby
Object. The first entry in the JSON Object must be a class with the `"^o"`
key. After that each entry is treated as a variable of the Object where the
key is the variable name without the preceeding `@`. An example is
`{"^o":"Oj::Bag","x":58,"y":"marbles"}`.

8. A `"^u"` JSON Object key indicates the value should be converted to a Ruby
Struct. The first entry in the JSON Object must be a class with the `"^u"`
key. After that each entry is is given a numeric position in the struct and
that is used as the key in the JSON Object. An example is `{"^u":["Range",1,7,false]}`.

9. When encoding an Object, if the variable name does not begin with an `@`
character then the name preceeded by a `~` character. This occurs in the
Exception class. An example is `{"^o":"StandardError","~mesg":"A Message","~bt":[".\/tests.rb:345:in `test_exception'"]}`.

10. If a Hash entry has a key that is not a String or Symbol then the entry is
encoded with a key of the form `"^#n"` where n is a hex number. The value that
is an Array where the first element is the key in the Hash and the second is
the value. An example is `{"^#3":[2,5]}`.

11. A `"^i"` JSON entry in either an Object or Array is the ID of the Ruby
Object being encoded. It is used when the :circular flag is set. It can appear
in either a JSON Object or in a JSON Array. In an Object the `"^i"` key has a
corresponding reference Fixnum. In an array the sequence will include an
embedded reference number. An example is
`{"^o":"Oj::Bag","^i":1,"x":["^i2",true],"me":"^r1"}`.

12. A `"^r"` JSON entry in an Object is a references to a Object or Array that
already appears in the JSON String. It must match up with a previous `"^i"`
ID. An example is `{"^o":"Oj::Bag","^i":1,"x":3,"me":"^r1"}`.

13. If an Array element is a String and starts with `"^i"` then the first
character, the `^` is encoded as a hex character sequence. An example is
`["\u005ei37",3]`.

### License:

    Copyright (c) 2012, Peter Ohler
    All rights reserved.
    
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    
     - Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    
     - Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    
     - Neither the name of Peter Ohler nor the names of its contributors may be
       used to endorse or promote products derived from this software without
       specific prior written permission.
    
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
