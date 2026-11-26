# Security and Optimization

Two settings in Oj are useful for parsing but do expose a
vulnerability if used from an untrusted source. Symbolized keys can
cause memory to be filled, since with some options those Symbols are never garbage collected. The same is
true for auto defining classes; memory will also be exhausted if too
many classes are automatically defined. Auto defining is a useful
feature during development and from trusted sources but it allows too
many classes to be created in the object load mode and auto defined is
used with an untrusted source. The `Oj.safe_load()` method sets and
uses the most strict and safest options. It should be used by
developers who find it difficult to understand the options available
in Oj.

The options in Oj are designed to provide flexibility to the
developer. This flexibility allows Objects to be serialized and
deserialized. An Object is deserialized by allocating it without
calling its initializer and then setting its instance variables. A few
methods are called on what that builds: `exception` and
`set_backtrace` on an `Exception`, and `replace` on a subclass of
`Hash`, `Array` or `String` for a `self` key. As in any system, check
your inputs before working with them. Taking an arbitrary `String`
from a user and evaluating it is never a good idea from an unsecure
source. The same is true for `Object` attributes as they are not more
than `String`s. Always check inputs from untrusted sources.

## The mode a document is loaded in

`Oj.load()` defaults to the `:object` mode unless the mode is
changed. That mode takes the class of an Object from the document, so
the document decides which of the classes already loaded in the
process is allocated and what its instance variables are. Classes are
looked up and not created unless `:auto_define` is turned
on. `Oj.load` :object mode is intended for storing objects or for
transmitting them between trusted applications. Parsing JSON not generated
by Oj in :object mode should never be attempted.

For a document from an untrusted source, set the parser mode to not
create objects such as `:strict` mode or `Oj.safe_load`.

```ruby
Oj.load(json, mode: :strict)
Oj.strict_load(json)
Oj.safe_load(json)
```

or set the mode globally for the process:

```ruby
Oj.default_options = {mode: :strict}
```

`Oj.object_load()` is the explicit form of the current default and
stays in `:object` mode whatever the default is set to.

## Found a potential vulnerability

Exploits can only be initiated against applications hence all
vulnerabilities start with an attack vector from an application. Oj is
a library and is only reachable by calls from an application. Does
that mean an application is solely responsible for exposing a
vulnerability? No, but it does put a great deal of the blame on an
application for any exploit vector. A library can be considered
responsible for a vulnerability if a bug is exploitable through the
normal and expected use of its APIs. In any case reporting a bug is
encouraged. If the bug is a vulnerability report it as such. If it is
not exploitable then an issue is more appropriate.

## Usual use

The usual use of Oj is to be called by an application such as a Rails
application. A web page accepts user input, formats a JSON document
based on that input and transmits or saves that JSON document over a
secure transport or writes to a database. Offering an end user on an
untrusted network the ability to submit any random JSON for parsing is
highly discouraged.

Even with trusted exchanges a weak web application that allows any
input that is then dropped into a JSON document could introduce a
vulnerability. For example, a front end that allowed users to specify
hash keys for a JSON object as well as content could be an issue if
the developer also allowed for creation of objects with the
`create_additions` option. In such a case Oj might be considered a
contributor to a vulnerability although the fix might simply be to
warn against writing such an application.

Please see the [security policy](https://github.com/ohler55/oj/security/policy) for
reporting.
