# Oj gem
A fast JSON parser and Object marshaller as a Ruby gem.

## <a name="installation">Installation</a>
    gem install oj

## <a name="documentation">Documentation</a>

*Documentation*: http://www.ohler.com/oj

## <a name="source">Source</a>

*GitHub* *repo*: https://github.com/ohler55/oj

*RubyGems* *repo*: https://rubygems.org/gems/oj

## <a name="follow">Follow @oxgem on Twitter</a>

[Follow @peterohler on Twitter](http://twitter.com/#!/peterohler) for announcements and news about the Oj gem.

## <a name="build_status">Build Status</a>

[![Build Status](https://secure.travis-ci.org/ohler55/oj.png?branch=master)](http://travis-ci.org/ohler55/oj)

## <a name="links">Links of Interest</a>

[Need for Speed](http://www.ohler.com/dev/need_for_speed/need_for_speed.html) for an overview of how Oj::Doc was designed.

*Fast XML parser and marshaller on RubyGems*: https://rubygems.org/gems/ox

*Fast XML parser and marshaller on GitHub*: https://github.com/ohler55/ox

## <a name="release">Release Notes</a>

### Release 2.0.6

 - Worked around an undocumented feature in linux when using make that misreports the stack limits.

### Release 2.0.5

 - DateTimes are now output the same in compat mode for both 1.8.7 and 1.9.3 even though they are implemented differently in each Ruby.

 - Objects implemented as data structs can now change the encoding by implemented either to_json(), as_json(), or to_hash().

 - Added an option to allow BigDecimals to be dumped as either a string or as a number. There was no agreement on which
   was the best or correct so both are possible with the correct option setting.

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

Oj is compatible with Ruby 1.8.7, 1.9.2, 1.9.3, JRuby, RBX, and the latest 2.0dev. Note that JRuby now disables support
for extentions by default and is no longer supporting C extensions. Bugs reported in the JRuby MRI are no longer being
fixed and there is at least one that has caused a test to be commented out for JRuby in the Oj test suite. JRuby can be
build with extensions enabled. Check the documenation for JRuby installs in your environment.

Oj is also compatible with Rails. Just make sure the Oj gem is installed and
[multi_json](https://github.com/intridea/multi_json) will pick it up and use it.

Oj offers two alternative APIs for processing JSON. The fastest one is the Oj::Doc API. The Oj::Doc API takes a
completely different approach by opening a JSON document and providing calls to navigate around the JSON while it is
open. With this approach JSON access can be well over 20 times faster than conventional JSON parsing.

Another API, the Oj::Saj API follows an XML SAX model and walks the JSON document depth first and makes callbacks for
each element. The Oj::Saj API is useful when only portions of the JSON are of interest. Performance up to 20 times
faster than conventional JSON are possible. The API is simple to use but does require a different approach than the
conventional parse followed by access approach used by conventional JSON parsing.

## <a name="proper_use">Proper Use</a>

Two settings in Oj are useful for parsing but do expose a vunerability if used from an untrusted source. Symbolizing
keys can be used to cause memory to be filled up since Ruby does not garbage collect Symbols. The same is true for auto
defining classes. Memory can be exhausted if too many classes are automatically defined. Auto defining is a useful
feature during development and from trusted sources but it does allow too many classes to be created in the object load
mode and auto defined is used with an untrusted source. The Oj.strict_load() method sets uses the most strict and safest
options. It should be used by developers who find it difficult to understand the options available in Oj.

The options in Oj are designed to provide flexibility to the developer. This flexibility allows Objects to be serialized
and deserialized. No methods are ever called on these created Objects but that does not stop the developer from calling
methods on the Objects created. As in any system, check your inputs before working with them. Taking an arbitrary String
from a user and evaluating it is never a good idea from an unsecure source. The same is true for Object attributes as
they are not more than Strings. Always check inputs from untrusted sources.

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

It is also worth noting that although Oj is slightly behind MessagePack for
parsing, Oj serialization is much faster than MessagePack even though Oj uses
human readable JSON vs the binary MessagePack format.

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

    --------------------------------------------------------------------------------
    Load/Parse Performance
    Oj:compat.load 100000 times in 0.806 seconds or 124051.164 load/sec.
    Oj.load 100000 times in 0.810 seconds or 123384.587 load/sec.
    Yajl.parse 100000 times in 1.441 seconds or 69385.996 parse/sec.
    JSON::Ext.parse 100000 times in 1.567 seconds or 63797.848 parse/sec.
    JSON::Pure.parse 100000 times in 13.500 seconds or 7407.247 parse/sec.
    Ox.load 100000 times in 0.954 seconds or 104836.748 load/sec.
    MessagePack.unpack 100000 times in 0.651 seconds or 153707.817 unpack/sec.
    
    Summary:
         System  time (secs)  rate (ops/sec)
    -----------  -----------  --------------
    MessagePack       0.651      153707.817
      Oj:compat       0.806      124051.164
             Oj       0.810      123384.587
             Ox       0.954      104836.748
           Yajl       1.441       69385.996
      JSON::Ext       1.567       63797.848
     JSON::Pure      13.500        7407.247
    
    Comparison Matrix
    (performance factor, 2.0 row is means twice as fast as column)
                 MessagePack    Oj:compat           Oj           Ox         Yajl    JSON::Ext   JSON::Pure
    -----------  -----------  -----------  -----------  -----------  -----------  -----------  -----------
    MessagePack         1.00         1.24         1.25         1.47         2.22         2.41        20.75
      Oj:compat         0.81         1.00         1.01         1.18         1.79         1.94        16.75
             Oj         0.80         0.99         1.00         1.18         1.78         1.93        16.66
             Ox         0.68         0.85         0.85         1.00         1.51         1.64        14.15
           Yajl         0.45         0.56         0.56         0.66         1.00         1.09         9.37
      JSON::Ext         0.42         0.51         0.52         0.61         0.92         1.00         8.61
     JSON::Pure         0.05         0.06         0.06         0.07         0.11         0.12         1.00
    
    
    --------------------------------------------------------------------------------
    Dump/Encode/Generate Performance
    Oj:compat.dump 100000 times in 0.173 seconds or 578526.262 dump/sec.
    Oj.dump 100000 times in 0.179 seconds or 558362.880 dump/sec.
    Yajl.encode 100000 times in 0.776 seconds or 128794.279 encode/sec.
    JSON::Ext.generate 100000 times in 3.511 seconds or 28483.812 generate/sec.
    JSON::Pure.generate 100000 times in 7.389 seconds or 13533.717 generate/sec.
    Ox.dump 100000 times in 0.196 seconds or 510589.629 dump/sec.
    MessagePack.pack 100000 times in 0.317 seconds or 315307.220 pack/sec.
    
    Summary:
         System  time (secs)  rate (ops/sec)
    -----------  -----------  --------------
      Oj:compat       0.173      578526.262
             Oj       0.179      558362.880
             Ox       0.196      510589.629
    MessagePack       0.317      315307.220
           Yajl       0.776      128794.279
      JSON::Ext       3.511       28483.812
     JSON::Pure       7.389       13533.717
    
    Comparison Matrix
    (performance factor, 2.0 row is means twice as fast as column)
                   Oj:compat           Oj           Ox  MessagePack         Yajl    JSON::Ext   JSON::Pure
    -----------  -----------  -----------  -----------  -----------  -----------  -----------  -----------
      Oj:compat         1.00         1.04         1.13         1.83         4.49        20.31        42.75
             Oj         0.97         1.00         1.09         1.77         4.34        19.60        41.26
             Ox         0.88         0.91         1.00         1.62         3.96        17.93        37.73
    MessagePack         0.55         0.56         0.62         1.00         2.45        11.07        23.30
           Yajl         0.22         0.23         0.25         0.41         1.00         4.52         9.52
      JSON::Ext         0.05         0.05         0.06         0.09         0.22         1.00         2.10
     JSON::Pure         0.02         0.02         0.03         0.04         0.11         0.48         1.00
    
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
Time. The sequence `{"^t":1325775487.000000}` is read as Jan 5, 2012 at
23:58:07.

7. A `"^o"` JSON Object key indicates the value should be converted to a Ruby
Object. The first entry in the JSON Object must be a class with the `"^o"`
key. After that each entry is treated as a variable of the Object where the
key is the variable name without the preceeding `@`. An example is
`{"^o":"Oj::Bag","x":58,"y":"marbles"}`. `"^O"` is the same except that it is
for built in or odd classes that don't obey the normal Ruby rules. Examples
are Rational, Date, and DateTime.

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
