# Oj gem
[![Build Status](https://img.shields.io/travis/ohler55/oj/master.svg)](http://travis-ci.org/ohler55/oj?branch=master) ![Gem](https://img.shields.io/gem/v/oj.svg) ![Gem](https://img.shields.io/gem/dt/oj.svg)

A *fast* JSON parser and Object marshaller as a Ruby gem.

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

By default Oj uses the :object mode which is used to marshal and unmarshal Ruby
objects. Deserialize arbitrary JSON in object mode may lead to unexpected
results. :compat mode is a better choice for rails and :strict mode is a better
choice for general JSON parsing. See the options section below for details.

## Installation
```
gem install oj
```

or in Bundler:

```
gem 'oj'
```

## Further Reading

For more details on options, modes, advanced features, and more follow these
links.

 - {file:Options.md} for parse and dump options.
 - {file:Modes.md} for details on modes for strict JSON compliance, mimicing the JSON gem, and mimicing Rails and ActiveSupport behavior.
 - {file:Encoding.md} describes the :object encoding format.
 - {file:Compatibility.md} lists current compatibility with Rubys and Rails.
 - {file:Advanced.md} for fast parser and marshalling features.
 - {file:Security.md} for security considerations.

## Releases

See [CHANGELOG.md](CHANGELOG.md)

## Links

 - *Documentation*: http://www.ohler.com/oj, http://rubydoc.info/gems/oj

- *GitHub* *repo*: https://github.com/ohler55/oj

- *RubyGems* *repo*: https://rubygems.org/gems/oj

Follow [@peterohler on Twitter](http://twitter.com/#!/peterohler) for announcements and news about the Oj gem.

#### Performance Comparisons

 - [Oj Strict Mode Performance](http://www.ohler.com/dev/oj_misc/performance_strict.html) compares Oj strict mode parser performance to other JSON parsers.

- [Oj Compat Mode Performance](http://www.ohler.com/dev/oj_misc/performance_compat.html) compares Oj compat mode parser performance to other JSON parsers.

 - [Oj Object Mode Performance](http://www.ohler.com/dev/oj_misc/performance_object.html) compares Oj object mode parser performance to other marshallers.

 - [Oj Callback Performance](http://www.ohler.com/dev/oj_misc/performance_callback.html) compares Oj callback parser performance to other JSON parsers.

#### Links of Interest

 - *Fast XML parser and marshaller on RubyGems*: https://rubygems.org/gems/ox

 - *Fast XML parser and marshaller on GitHub*: https://github.com/ohler55/ox

 - [Oj Object Encoding Format](http://www.ohler.com/dev/oj_misc/encoding_format.html) describes the OJ Object JSON encoding format.

 - [Need for Speed](http://www.ohler.com/dev/need_for_speed/need_for_speed.html) for an overview of how Oj::Doc was designed.

 - *OjC, a C JSON parser*: https://www.ohler.com/ojc also at https://github.com/ohler55/ojc

 - *Piper Push Cache, push JSON to browsers*: http://www.piperpushcache.com
