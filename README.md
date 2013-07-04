# Oj gem
A fast JSON parser and Object marshaller as a Ruby gem.

## <a name="installation">Installation</a>
    gem install oj

## <a name="documentation">Documentation</a>

*Documentation*: http://www.ohler.com/oj

## <a name="source">Source</a>

*GitHub* *repo*: https://github.com/ohler55/oj

*RubyGems* *repo*: https://rubygems.org/gems/oj

Follow [@peterohler on Twitter](http://twitter.com/#!/peterohler) for announcements and news about the Oj gem.

## <a name="build_status">Build Status</a>

[![Build Status](https://secure.travis-ci.org/ohler55/oj.png?branch=master)](http://travis-ci.org/ohler55/oj)

### Current Release 2.1.4

 - Fixed Linux 32 bit rounding bug.

[Older release notes](http://www.ohler.com/dev/oj_misc/release_notes.html).

## <a name="description">Description</a>

Optimized JSON (Oj), as the name implies was written to provide speed optimized
JSON handling. It was designed as a faster alternative to Yajl and other the
common Ruby JSON parsers. So far is has achieved that at about 2 time faster
than any other Ruby JSON parser and 3 or more times faster writing JSON.

Oj has several dump or serialization modes which control how Objects are
converted to JSON. These modes are set with the :mode option in either the
default options or as one of the options to the dump() method.

- :strict mode will only allow the 7 basic JSON types to be serialized. Any
  other Object will raise and Exception.

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

Oj is compatible with Ruby 1.8.7, 1.9.2, 1.9.3, 2.0.0, JRuby, and RBX. Note that JRuby now disables support
for extentions by default and is no longer supporting C extensions. Bugs reported in the JRuby MRI are no longer being
fixed and there is at least one that has caused a test to be commented out for JRuby in the Oj test suite. JRuby can be
build with extensions enabled. Check the documenation for JRuby installs in your environment.

Oj is also compatible with Rails. Just make sure the Oj gem is installed and
[multi_json](https://github.com/intridea/multi_json) will pick it up and use it.

Oj offers a few alternative APIs for processing JSON. The fastest one is the Oj::Doc API. The Oj::Doc API takes a
completely different approach by opening a JSON document and providing calls to navigate around the JSON while it is
open. With this approach JSON access can be well over 20 times faster than conventional JSON parsing.

Other parsers, the Oj::Saj and the Oj::ScHandler API are callback parsers that
walk the JSON document depth first and makes callbacks for each element. Both
callback parser are useful when only portions of the JSON are of
interest. Performance up to 20 times faster than conventional JSON are
possible. The API is simple to use but does require a different approach than
the conventional parse followed by access approach used by conventional JSON
parsing.

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

## <a name="release">Release Notes</a>

### Simple JSON Writing and Parsing:

```ruby
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
```

# Links

## <a name="links">Performance Comparisons</a>

[Oj Strict Mode Performance](http://www.ohler.com/dev/oj_misc/performance_strict.html) compares Oj strict mode parser performance to other JSON parsers.

[Oj Compat Mode Performance](http://www.ohler.com/dev/oj_misc/performance_compat.html) compares Oj compat mode parser performance to other JSON parsers.

[Oj Object Mode Performance](http://www.ohler.com/dev/oj_misc/performance_object.html) compares Oj object mode parser performance to other marshallers.

[Oj Callback Performance](http://www.ohler.com/dev/oj_misc/performance_callback.html) compares Oj callback parser performance to other JSON parsers.

## <a name="links">Links of Interest</a>

*Fast XML parser and marshaller on RubyGems*: https://rubygems.org/gems/ox

*Fast XML parser and marshaller on GitHub*: https://github.com/ohler55/ox

[Oj Object Encoding Format](http://www.ohler.com/dev/oj_misc/encoding_format.html) describes the OJ Object JSON encoding format.

[Need for Speed](http://www.ohler.com/dev/need_for_speed/need_for_speed.html) for an overview of how Oj::Doc was designed.

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
