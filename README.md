# Oj gem
[![Build Status](https://secure.travis-ci.org/ohler55/oj.png?branch=master)](http://travis-ci.org/ohler55/oj)

A fast JSON parser and Object marshaller as a Ruby gem.

## Using

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

## Installation
```
gem install oj
```

or in Bundler:

```
gem 'oj'
```

## Advanced

Optimized JSON (Oj), as the name implies, was written to provide speed optimized
JSON handling. It was designed as a faster alternative to Yajl and other
common Ruby JSON parsers. So far it has achieved that, and is about 2 times faster
than any other Ruby JSON parser, and 3 or more times faster at serializing JSON.

Oj has several `dump` or serialization modes which control how Ruby `Object`s are
converted to JSON. These modes are set with the `:mode` option in either the
default options or as one of the options to the `dump` method. In addition to
the various options there are also alternative APIs for parsing JSON.

The fastest alternaive parser API is the `Oj::Doc` API. The `Oj::Doc` API takes
a completely different approach by opening a JSON document and providing calls
to navigate around the JSON while it is open. With this approach, JSON access
can be well over 20 times faster than conventional JSON parsing.

The `Oj::Saj` and `Oj::ScHandler` APIs are callback parsers that
walk the JSON document depth first and makes callbacks for each element.
Both callback parser are useful when only portions of the JSON are of
interest. Performance up to 20 times faster than conventional JSON is
possible if only a few elements of the JSON are of interest.

### Options

To change default serialization mode use the following form. Attempting to
modify the Oj.default_options Hash directly will not set the changes on the
actual default options but on a copy of the Hash:

```ruby
Oj.default_options = {:mode => :compat }
```

 * `:mode` [Symbol] mode for dumping and loading JSON. **Important format details [here](http://www.ohler.com/dev/oj_misc/encoding_format.html).**

  - `:object` mode will dump any `Object` as a JSON `Object` with keys that
    match the Ruby `Object`'s variable names without the '@' prefix
    character. This mode has the best performance and is the default.

  - `:strict` mode will only allow the 7 basic JSON types to be serialized. Any
    other `Object` will raise an `Exception`.

  - `:null` mode replaces any `Object` that is not one of the JSON types with a JSON `null`.

  - `:compat` mode attempts to be compatible with other systems. It will
    serialize any `Object`, but will check to see if the `Object` implements an
    `to_hash` or `to_json` method. If either exists, that method is used for
    serializing the `Object`.  Since `as_json` is more flexible and produces
    more consistent output, it is preferred over the `to_json` method. If
    neither the `to_json` or `to_hash` methods exists, then the Oj internal
    `Object` variable encoding is used.

 * `:indent` [Fixnum] number of spaces to indent each element in an JSON
   document, zero is no newline between JSON elements, negative indicates no
   newline between top level JSON elements in a stream
   
 * `:circular` [Boolean] support circular references while dumping.
 
 * `:auto_define` [Boolean] automatically define classes if they do not
   exist.
   
 * `:symbol_keys` [Boolean] use symbols instead of strings for hash keys.
 
 * `:escape_mode` [Symbol] determines the characters to escape.

  - `:newline` allows unescaped newlines in the output.

  - `:json` follows the JSON specification. This is the default mode.
  
  - `:xss_safe` escapes HTML and XML characters such as `&` and `<`.
  
  - `:ascii` escapes all non-ascii or characters with the hi-bit set.
   
 * `:class_cache` [Boolean] cache classes for faster parsing (if
   dynamically modifying classes or reloading classes then don't use this)
   
 * `:time_format` [Symbol] time format when dumping in :compat and :object mode

  - `:unix` time is output as a decimal number in seconds since epoch including
    fractions of a second.
  
  - `:unix_zone` similar to the `:unix` format but with the timezone encoded in
    the exponent of the decimal number of seconds since epoch.
  
  - `:xmlschema` time is output as a string that follows the XML schema definition.
  
  - `:ruby` time is output as a string formatted using the Ruby `to_s` conversion.

 * `:bigdecimal_as_decimal` [Boolean] dump BigDecimal as a decimal number
   or as a String
   
 * `:bigdecimal_load` [Symbol] load decimals as BigDecimal instead of as a
   Float.

  - `:bigdecimal` convert all decimal numbers to BigDecimal.
  
  - `:float` convert all decimal numbers to Float.
  
  - `:auto` the most precise for the number of digits is used.
   
 * `:create_id` [String] create id for json compatible object encoding,
   default is `json_create`.
   
 * `:second_precision` [Fixnum] number of digits after the decimal when dumping
   the seconds portion of time
   
 * `:float_precision` [Fixnum] number of digits of precision when dumping
   floats, 0 indicates use Ruby
   
 * `:use_to_json` [Boolean] call to_json() methods on dump, default is
   false
   
 * `:nilnil` [Boolean] if true a nil input to load will return nil and
   not raise an Exception
   
 * `:allow_gc` [Boolean] allow or prohibit GC during parsing, default is
   true (allow).
   
 * `:quirks_mode` [Boolean] Allow single JSON values instead of
   documents, default is true (allow).

* `:nan` [:null|:huge|:word|:raise|:auto] How to dump Infinity, -Infinity, and
  NaN in null, strict, and compat mode. :null places a null, :huge places a huge
  number, :word places Infinity or NaN, :raise raises and exception, :auto uses
  default for each mode which are :raise for :strict, :null for :null, and :word
  for :compat. Default is :auto.

## Releases

** Release 2.15.0**

 - Fixed bug where encoded strings could be GCed.

 - :nan option added for dumping Infinity, -Infinity, and NaN. This is an
   edition to the API. The default value for the :non option is :auto which uses
   the previous NaN handling on dumping of non-object modes.

**Release 2.14.7**

 - Fixed bug where a comment before another JSON element caused an
   error. Comments are not part of the spec but this keep support consistent.

[Older release notes](http://www.ohler.com/dev/oj_misc/release_notes.html).

## Compatibility

**Ruby**

Oj is compatible with Ruby 1.8.7, 1.9.2, 1.9.3, 2.0.0, 2.1, 2.2, 2.3 and RBX.
Support for JRuby has been removed as JRuby no longer supports C extensions and
there are bugs in the older versions that are not being fixed.

**Rails**

Although up until 4.1 Rails uses [multi_json](https://github.com/intridea/multi_json), an [issue in Rails](https://github.com/rails/rails/issues/9212) causes ActiveSupport to fail to make use Oj for JSON handling.
There is a
[gem to patch this](https://github.com/GoodLife/rails-patch-json-encode) for
Rails 3.2 and 4.0. As of the Oj 2.6.0 release the default behavior is to not use
the `to_json()` method unless the `:use_to_json` option is set. This provides
another work around to the rails older and newer behavior.

The latest ActiveRecord is able to work with Oj by simply using the line:

```
serialize :metadata, Oj
```

In version Rails 4.1, multi_json has been removed, and this patch is unnecessary and will no longer work.
Instead, use the `oj_mimic_json` [gem](https://github.com/ohler55/oj_mimic_json) along with `oj` in your `Gemfile` to have Oj mimic the JSON gem and be used in its place by `ActiveSupport` JSON handling:

```
gem 'oj'
gem 'oj_mimic_json'
```

## Security and Optimization

Two settings in Oj are useful for parsing but do expose a vulnerability if used
from an untrusted source. Symbolized keys can cause memory to be filled with
previous versions of ruby. Ruby 2.1 and below does not garbage collect
Symbols. The same is true for auto defining classes in all versions of ruby;
memory will also be exhausted if too many classes are automatically
defined. Auto defining is a useful feature during development and from trusted
sources but it allows too many classes to be created in the object load mode and
auto defined is used with an untrusted source. The `Oj.strict_load()` method
sets and uses the most strict and safest options. It should be used by
developers who find it difficult to understand the options available in Oj.

The options in Oj are designed to provide flexibility to the developer. This
flexibility allows Objects to be serialized and deserialized. No methods are
ever called on these created Objects but that does not stop the developer from
calling methods on them. As in any system, check your inputs before working with
them. Taking an arbitrary `String` from a user and evaluating it is never a good
idea from an unsecure source. The same is true for `Object` attributes as they
are not more than `String`s. Always check inputs from untrusted sources.

## Links

*Documentation*: http://www.ohler.com/oj, http://rubydoc.info/gems/oj

*GitHub* *repo*: https://github.com/ohler55/oj

*RubyGems* *repo*: https://rubygems.org/gems/oj

Follow [@peterohler on Twitter](http://twitter.com/#!/peterohler) for announcements and news about the Oj gem.

#### Performance Comparisons

[Oj Strict Mode Performance](http://www.ohler.com/dev/oj_misc/performance_strict.html) compares Oj strict mode parser performance to other JSON parsers.

[Oj Compat Mode Performance](http://www.ohler.com/dev/oj_misc/performance_compat.html) compares Oj compat mode parser performance to other JSON parsers.

[Oj Object Mode Performance](http://www.ohler.com/dev/oj_misc/performance_object.html) compares Oj object mode parser performance to other marshallers.

[Oj Callback Performance](http://www.ohler.com/dev/oj_misc/performance_callback.html) compares Oj callback parser performance to other JSON parsers.

#### Links of Interest

*Fast XML parser and marshaller on RubyGems*: https://rubygems.org/gems/ox

*Fast XML parser and marshaller on GitHub*: https://github.com/ohler55/ox

[Oj Object Encoding Format](http://www.ohler.com/dev/oj_misc/encoding_format.html) describes the OJ Object JSON encoding format.

[Need for Speed](http://www.ohler.com/dev/need_for_speed/need_for_speed.html) for an overview of how Oj::Doc was designed.

*OjC, a C JSON parser*: https://www.ohler.com/ojc also at https://github.com/ohler55/ojc

*Piper Push Cache, push JSON to browsers*: http://www.piperpushcache.com
