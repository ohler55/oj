# Vulnerabilities

Exploits can only be initiated against applications hence all
vulnerabilities start with an attack vector from an application. Oj is
a library and is only reachable by calls from an application. Does
that mean an application is solely responsible for exposing a
vulnerability? No, but it does put a great deal of the blame on an
application for any exploit vector. A library can be considered
responsible for a vulnerability if a bug is exploitable through the
normal and expected use of its APIs.

## Normal and Expected Use

Oj APIs are either for JSON parsing or writing. JSON can be written to
a file or data store. It can also be used for exchanging data between
applications. In most if not all cases JSON documents reside behind
a firewall in the private portion of the network.

Of course a developer could build an application that accepted any
JSON over a publicly exposed endpoint and then proceed to parse that
JSON document. Such an application opens the door for any number of
exploits no matter which JSON parser is used. A few possible
exploits for the ill advised application are exceeding available
memory, DDoS attacks, and depending on how the parsed data is used
more severe issues. So for developers reading this, don't do this.

### Parsers

Oj includes a number of parsers and parse modes. Each has slightly
different use cases.

The `Oj.load` function with :strict and:null modes are used strictly for
parsing JSON into Hash, Array, String, numbers, booleans, or nil. They
are relatively safe in that they can not be used to create any other
types The :compat, :rails, and :custom mods are more open in that they
can be used to create objects with more complex behavior. Those must
be used under a more controlled environment. The remaining :object
mode definition indicates it should only be used to parse JSON
generated with a :object mode dump. Attempting to generate the JSON
manually could potentially lead to issues.

The callback parsers `Oj.sc_parse` and `Oj.saj_parse` leave the
handling of parsed data to the application, which puts the onus back on the
application to not create a vulnerability.

The `Oj::Doc` allows a JSON document to be navigated with Ruby object
creation postponed until needed. Any potential vulnerabilities in `Oj::Doc`
would be due to trying to parse unfiltered JSON passed directly to the
methods.

The newer `Oj::Parser` variations match the `Oj.load` modes for saj
and usual. The introduction of a validation mode and a debug
mode. Using those methods on reasonably sized JSON should be fine but
to reiterate, no parser should be used on public supplied JSON with
some kind of control or filtering. The safe mode makes some effort to
pre-filter JSON based on size, nesting, and a few other parameters so
the layer between user supplied data can be thinner. It's still risky
to accept public user JSON.

### Writers

It's hard to envision the Oj writers being exposed to a public user
since writers take a Ruby object and generate JSON from those
objects. The only way to exploit a write operation would be to either
corrupt the application methods or for the developer to intentionally
cause an error.

## Unusual Use

If it isn't clear what constitutes unusual use of the Oj APIs, some
examples are listed.

### Memory Overruns

Expecting to parse large JSON documents without allocating enough
memory in a fiber or application container could cause out of memory
errors. Of course the same could be true for any operation that uses
memory. It is not specific to Oj parsers.

### Exposing Oj APIs to the Public

An application that accepts any JSON document is prone to any number
of exploits varying from huge document submition to slow read or DDos
attacks.

### Writing Invalid :object Mode JSON

An application that attempts to generate :object mode JSON could end
up with invalid encoding. That could potentially be exploitable
depending on the where the cutom generated encoding is on an attach
vector.

## Summary

It all boils down to use Oj as intended and barring a new bug all
vulterabilities will be from the application. The same can be said for
virtually and library.
