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

### Release 0.5

This is the first release sith a version of 0.5 indicating it is only half
done. Basic load() and dump() is supported for Hash, Array, NilClass,
TrueClass, FalseClass, Fixnum, Float, Symbol, and String Objects.

## <a name="description">Description</a>

Optimized JSON (Oj), as the name implies was written to provide speed
optimized JSON handling. It was designed as a faster alternative to Yajl and
other the common Ruby JSON parsers. So far is has achieved that at about 2
time faster than Yajl for parsing and 3 or more times faster writing JSON.

Coming soon: As an Object marshaller with support for circular references.

Coming soon: A SAX like JSON stream parser.

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
