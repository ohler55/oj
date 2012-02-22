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

## <a name="release">Release Notes</a>

### Release 0.6.0

- supports arbitrary Object dumping/serialization

- to_hash() method called if the Object responds to to_hash and the result is converted to JSON

- to_json() method called if the Object responds to to_json

- any Object with variables can be dumped correctly

- default options have been added

## <a name="description">Description</a>

Optimized JSON (Oj), as the name implies was written to provide speed
optimized JSON handling. It was designed as a faster alternative to Yajl and
other the common Ruby JSON parsers. So far is has achieved that at about 2
time faster than Yajl for parsing and 3 or more times faster writing JSON.

Oj has several dump or serialization modes which control how Objects are
converted to JSON. These modes are set with the :effort option in either the
dafault options or as one of the options to the dump() method. The :strict
mode will only allow the 7 basic JSON types to be serialized. Any other Object
will raise and Exception. In the :lazy mode any Object that is not one of the
JSON types is replaced by a JSON null. In :interal mode any Object will be
dumped as a JSON Object with keys that match the Ruby Object's variable names
without the '@' character. This is the highest performance mode. The :internal
mode is not found in other JSON gems. The last mode, the :tolerant mode is the
most tolerant. It will serialize any Object but will check to see if the
Object implements a to_hash() or to_json() method. If either exists that
method is used for serializing the Object. The to_hash() is more flexible and
produces more consistent output so it has a preference over the to_json()
method. If neither the to_json() or to_hash() methods exist then the Oj
internal Object variable encoding is used.

Coming soon: As an Object marshaller with support for circular references.

Coming soon: A JSON stream parser.

Oj is compatible with Ruby 1.8.7, 1.9.2, 1.9.3, JRuby, and RBX.

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
