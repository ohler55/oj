# Oj::Parser

The original Oj parser is a performant parser that supports several
modes. As of this writing Oj is almost 10 years old. A dinosaur in the
coding world. It was time for an upgrade. Dealing with issues over the
years it became clear that a few things could have been done
better. The new Oj parser is a response that not only attempts to
address some of the issues but also give the Oj parser a significant
boost in performance. The new parser is the Oj::Parser. It takes a
different approach to JSON parsing than the now legacy Oj parser. Not
really a legacy parser since the new parser doesn't support a JSON gem
drop-in replacement but it is as much as or more than 3 times faster
than the previous parser.

## Annoyances

The `Oj.load` parser has a few things that continue to be the reason
for many of the issue on the project. The most significant area is
compatibility with both Rails and the JSON gem as they battle it out
for which behavior will win out in any particular situation. The
reason for the conflict is that each monkey patches all classes to
behave the way that package expects. There is nothing Oj can do to
address those design decision but it only impacts object creation on
the parsing side so the new Oj::Parser offers an alternative while
still supporting the JSONB gem model for object creation It turns out
the new parser is mostly compatible but it make no claim to match
behavior and what ever inconsistencies are present in either package.

The real annoyances that can and are addressed are around the handling
of options. For `Oj.load` there is a set of default options that
can be set and the same options can be specified for each call to
parse or load. This approach as a couple of downsides. One the
defaults are shared across all calls to parse no matter what the
desire mode is. The second is that having to provide all the options
on each parse call incurrs a performance penalty and is just annoying
to repeat the same set of options over may calls.

## How

It's all wonderfull to wish for a faster parser that solves all the
annoyances of the previous parser but how was it done is a much more
interesting question to answer.

At the core, the API for parsing was changed. Instead of a sinle
global parser any number of parsers can be created and each is seprate
from the others. The parser itself is able to rip through a JSON
string, stream, or file and then make calls to a delegate to processes
the JSON elements according to the delegate behavior. This is similar
to the `Oj.load` parser but the new parser takes advantage of
character maps, reducing conditional branching, and calling function
pointers.

### Options

As mentioned, only way to change the options issues was to change the
API. Instead of having a shared set of default options a separate
parser is created and configured for each application. Options are set
with methods on the parser so no more guessing what options are
available. With options isolated to individual parsers there is no
unintended leakage to other parse applications.

### Structure

A relative small amount of time is spent in the actual parsing of JSON
in `Oj.load`. Most of the time is spent building the Ruby
objects. Even cutting the parsing time in half only gives a 10%
improvement in performance but 10% is still an improvement.

The `Oj::Parser` is designed to reduce conditional branching. To do
that it uses character maps for the various states that the parser
goes through when parsing. There is no recursion as the JSON elements
are parsed and the use of a character map for each mode means the
parser function can and is re-entrant so partial blocks of JSON can be
parsed and the results combined.

There are no Ruby calls in the parser itself. Instead delegates are
used to implement the various behaviors of the parser such as
validation (validate), callbacks (SAJ), or building Ruby objects
(usual). The delegates are where all the Ruby related optimizations
take place.

Considering JSON file parsing, `Oj.load_file` is able to read a file a
block at a time and the new `Oj::Parser` does the same. There was
change in how that is done though. `Oj.load_file` sets up a reader
that must be called for each character. Basically a buffered
reader. `Oj::Parser` drops down a level and uses a re-entrant parser
that takes a block of bytes at a time so there is no call needed for
each character but rather just iterating over the block read from the
file.

Reading a block at a time also allows for an efficient second thread
to be used for reading blocks. That feature is not in the first
iteration of the `Oj::Parser` but it set the stage for it in the
future. The same approach was used successfully in
[OjC](https://github.com/ohler55/ojc) which is where the code for the
parser was taken from.

### Delegates

There are three delegates; validate, SAJ, and usual.

#### Validate

The validate delegate is trivial in that does nothing other than let
the parser complete so there are no options for the validate
delegate. By not making any Ruby calls other than to start the parsing
the validate delegate is the best performer as might be expected.

#### SAJ (Simple API for JSON)

The SAJ delegate is compatible with the SAJ handlers used with
`Oj.saj_parse` so it needs to keep track of keys for the
callbacks. Two optimizations are used. The first is a reuseable key
stack while the second is a string cache similar to the Ruby intern
function.

When parsing a Hash (JSON object) element the key is passed to the
callback function if the SAJ handler responds to the method. The key
is also provided when closing an Array or Hash that is part of a
parent Hash. A key stack supports this.

If the option is turned on a lookup is made and previously cached key
VALUEs are used. This avoids creating the string for the key and
setting the encoding on it. The cache used is a auto expanding hash
implementation that is limited to strings less than 35 characters
which covers most keys. Larger strings use the slower string creation
approach. The use of the cache reduces object creation which save on
both memory allocation and time. It is not appropriate for one time
parsing of say all the keys in a dictionary but is ideally suited for
loading similar JSON multiple times.

#### Usual

By far the more complex of the delegates is the usual delegate. The
usual delegate build Ruby Objects when parsing JSON. It incorporates
many options and makes use of a number of optimizations.

##### Reduce Branching

In keeping with the goal of reducing conditional branching most of the
options are implemented by changing a function pointer accrding to the
option selected. For example when turning on or off `:symbol_keys` the
function to calculate the key is changed so no decision needs to be
made during parsing. Using this approach option branching happens when
the option is set and not each time when parsing.

##### Cache

Creating Ruby Objects whether Strings, Array, or some other class is
expensive. Well expensive when running at the speeds Oj runs at. One
way to reduce Object creation is to cache those objects on the
assumption that they will most likely be used again. This is
especially true of Hash keys and Object attribute IDs. When creating
Object from a class name in the JSON a class cache saves resolving the
string to a class each time. Of course there are times when caching is
not preferred so caching is optional.

The Oj cache implementation is an auto expanding hash. When certain
limits are reached the hash is expanded and rehashed. Rehashing can
take some time as the number of items cached increases so there is
also an option to start with a larger cache size to avoid or reduce
the likelihood of a rehash.

The Oj cache has an advantage over the Ruby intern function
(`rb_intern()`) in that several steps are need for some cached
itels. As an example Object attribute IDs are created by adding an `@`
character prefix to a string and then converting to a ID. This is done
once when inserting into the cache and after that only a lookup is
needed.

##### Bulk Insert

The Ruby C extension functions available are extensive and offer many
options across the board. The bulk insert functions for both Arrays
and Hashes are much faster than appending or setting functions that
set one value at a time. The Array bulk insert is around 15 times
faster and for Hash it is about 3 times faster.

To take advantage of the bulk inserts arrays of VALUEs are
needed. With a little planning there VALUE arrays can be reused which
leads into another optimization, the use of stacks.

##### Stacks

Parsing requires memory to keep track values when parsing nested JSON
elements. That can be done on the call stack making use of recursive
calls or it can be done with a stack managed by the parser. The
`Oj.load` method maintains a stack for Ruby object and builds the
output as the parsing progresses.

The new parser uses three different stacks. One stack for values, one
for keys, and one for collections (Array and Hash). By postponing the
creation of the collection elements the bulk insertions for Array and
Hash can be used. For arrays the use of a value stack and creating the
array after all elements have been identified gives a 15x improvement
in array creation.

For Hash the story is a little different. The bulk insert for Hash
alternates keys and values but there is a wrinkle in that
approach. Since Ruby Object creation is triggered by the occurance of
an element that matches a creation identifier the creation of a
collection is not just Array and Hash but also Object and setting
Object attributes uses an ID and not a VALUE. For that reason the keys
should not be created as String or Symbol types as they would be
ignored and the VALUE creation wasted. Using the bulk insert for Hash
gives a 3x improvement for that part of the object building.

Looking at the Object creation the JSON gem expects a class method of
`#json_create(arg)`. The single argument is the Hash resulting from
the parsing assuming that the parser parsed to a Hash first. This is
less than ideal from a performance perspective so `Oj::Parser`
provides an option to take that approach or to use the much more
efficient approach of never creating the Hash but instead create the
Object and then set the attributes directly.

To further improve performance and reduce the amount of memory
allocations and frees the stacks are reused from one call to `#parse`
to another.

## Results

The results are even better than expected. Running the
[perf_parser.rb](https://github.com/ohler55/oj/blob/develop/test/perf_parser.rb)
file shows the improvements. There are four comparisons all run on a
MacBook Pro with Intel processor.

### Validation

Without a comparible parser that just validates a JSON document the
new `Oj::Parser.new(:validate)` is the closest which is the Oj SAJ
callback parser with a nil handler. In that case the comparison is:

```
             System  time (secs)  rate (ops/sec)
-------------------  -----------  --------------
Oj::Parser.validate       0.101      494369.136
       Oj::Saj.none       0.205      244122.745
```

The `Oj::Parser.new(:validate)` is 2.03 times faster!

### Callback

Oj has two callback parsers. One is SCP and the other SAJ. Both are
similar in that a handler is provided that implements methods for
processing the various element types in a JSON document. Comparing
`Oj.saj_parse` to `Oj::Parser.new(:saj)` with a all callback methods
implemented gives the following raw results:

```
        System  time (secs)  rate (ops/sec)
--------------  -----------  --------------
Oj::Parser.saj       0.783       63836.986
   Oj::Saj.all       1.182       42315.397
```

The `Oj::Parser.new(:saj)` is 1.51 times faster.

### Parse to Ruby primitives

Parsing to Ruby primitives and Array and Hash is possible with most
parsers including the build in JSON gem parser. The raw results are:

```
          System  time (secs)  rate (ops/sec)
----------------  -----------  --------------
Oj::Parser.usual       0.452      110544.876
 Oj::strict_load       0.699       71490.257
       JSON::Ext       1.009       49555.094
```

The `Oj::Parser.new(:saj)` is 1.55 times faster than `Oj.load` and
2.23 times faster than the JSON gem.

### Object

Oj supports two modes for Object serialization and
deserialization. Comparing to the JSON gem compatible mode the
following raw results are:

```
          System  time (secs)  rate (ops/sec)
----------------  -----------  --------------
Oj::Parser.usual       0.071      703502.033
 Oj::compat_load       0.225      221762.927
       JSON::Ext       0.401      124638.859
```

The `Oj::Parser.new(:saj)` is 3.17 times faster than `Oj.load` in
`:compat` mode and 5.64 times faster than the JSON gem.

## Summary

With a performance boost of from 1.5 to over 3x over the `Oj.load`
parser the new `Oj::Parser` is a big win in the performance arena. The
isolation of options is another feature that should make life easier
for developers.
