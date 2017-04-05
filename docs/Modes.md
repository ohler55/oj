# Oj Modes

Oj uses modes to switch the load and dump behavior. Initially Oj supported on
the :object mode which uses a format that allows Juby object encoding and
decoding in a manner that lets almost any Ruby object be encoded and decoded
without monkey patching the object classes. From that start other demands were
made the were best met by giving Oj multiple modes of operation. The current
modes are:

 - `:strict`
 - `:null`
 - `:compat` or `:json`
 - `:rails`
 - `:object`
 - `:custom`

Since modes detemine what the JSON output will look like and alternatively
what Oj expects when the `Oj.load()` method is called, mixing the output and
input mode formats will most likely not behave as intended. If the object mode
is used for producing JSON then use object mode for reading. The same is true
for each mode. It is possible to mix but only for advanced users.

## :strict Mode

Strict mode follows the JSON specifications and only supports the JSON native
types, Boolean, nil, String, Hash, Array, and Number are encoded as
expected. Encountering any other type causes an Exception to be raised. This
is the safest mode as it is just simple translation, no code outside Oj or the
core Ruby is execution on loading. Very few options are supported by this mode
other than formatting options.

## :null Mode

Null mode is similar to the :strict mode except that a JSON null is inserted
if a non-native type is encountered instead of raising an Exception.

## :compat or :json Mode

The `:compat` mode mimics the json gem. The json gem is built around the use
of the `to_json(*)` method defined for a class. Oj attempts to provide the
same functionality by being a drop in replacement with a few exceptions. First
a description of the json gem behavior and then the differenecs between the
json gem and the Oj.mimic_JSON behavior.

The json gem monkey patches core and base library classes with a `to_json(*)`
method. This allows calls such as `obj.to_json()` to be used to generate a
JSON string. The json gem also provides the JSON.generate(), JSON.dump(), and
JSON() functions. These functions generally act the same with some exceptions
such as JSON.generate(), JSON(), and to_json raise an exception when
attempting to encode infinity while JSON.dump() returns a the string
"Infinity". The String class is also monkey patched with to_json_raw() and
to_json_raw_object(). Oj in mimic mode mimics this behavior including the
seemly inconsistent behavior with NaN and Infinity.

Any class can define a to_json() method and JSON.generate(), JSON.dump(), and
JSON() functions will call that method when an object of that type is
encountered when traversing a Hash or Array. The core classes monkey patches
can be over-ridden but unless the to_json() method is called directory the
to_json() method will be ignored. Oj in mimic mode follow the same logic,

The json gem includes additions. These additions change the behavior of some
library and core classes. These additions also add the as_json() method and
json_create() class method. They are activated by requiring the appropriate
files. As an example, to get the modified to_json() for the Rational class
this line would be added.

```ruby
require 'json/add/rational'
```

Oj in mimic mode does not include these files although it will support the
modified to_json() methods. In keeping with the goal of providing a faster
encoder Oj offers an alternative. To activate faster addition version of the
to_json() method call

```ruby
Oj.add_to_json(Rational)
```

To revert back to the unoptimized version, just remove the Oj flag on that
class.

```ruby
Oj.remove_to_json(Rational)
```

## :rails Mode

TBD

## :object Mode

Object mode is for fast Ruby object serialization and deserialization. That
was the primary purpose of Oj when it was first developed. As such it is the
default mode unless changed in the Oj default options. In :object mode Oj
generates JSON that follows conventions which allow Class and other
information such as Object IDs for circular reference detection to be encoded
in a JSON document. The formatting follows the rules describe on the
{file:Encoding.md} page.

## :custom Mode

Custom mode honors all options. It provides the most flexibility although it
can not be configured to be exactly like any of the other modes. Each mode has
some special aspect that makes it unique. For example, the `:object` mode has
it's own unique format for object dumping and loading. The `:compat` mode
mimic the json gem including methods called for encoding and inconsistencies
between `JSON.dump()`, `JSON.generate()`, and `JSON()`.

## Options Matrix

Not all options are available in all modes. The options matrix identifies the
options available in each mode. An `x` in the matrix indicates the option is
supported in that mode. A number indicates the footnotes describe additional
information.

    | Option                 | type    | :null   | :strict | :compat | :rails  | :object | :custom |
    | ---------------------- | ------- | ------- | ------- | ------- | ------- | ------- | ------- |
    | :allow_blank           | Boolean |         |         |       1 |       1 |         |       x |
    | :allow_gc              | Boolean |       x |       x |       x |       x |       x |       x |
    | :allow_invalid_unicode | Boolean |         |         |         |         |       x |       x |
    | :allow_nan             | Boolean |         |         |       x |       x |       x |       x |
    | :array_class           | Class   |         |         |       x |       x |         |       x |
    | :array_nl              | String  |         |         |         |         |         |       x |
    | :ascii_only            | Boolean |       x |       x |       2 |       2 |       x |       x |
    | :auto_define           | Boolean |         |         |         |         |       x |       x |
    | :bigdecimal_as_decimal | Boolean |         |         |         |         |       x |       x |
    | :bigdecimal_load       | Boolean |         |         |         |         |         |       x |
    | :circular              | Boolean |       x |       x |       x |       x |       x |       x |
    | :class_cache           | Boolean |         |         |         |         |       x |       x |
    | :create_additions      | Boolean |         |         |       x |       x |         |       x |
    | :create_id             | String  |         |         |       x |       x |         |       x |
    | :empty_string          | Boolean |         |         |         |         |         |       x |
    | :escape_mode           | Symbol  |         |         |         |         |         |       x |
    | :float_precision       | Fixnum  |       x |       x |         |         |         |       x |
    | :hash_class            | Class   |         |         |       x |       x |         |       x |
    | :indent                | Integer |       x |       x |       3 |       3 |       x |       x |
    | :indent_str            | String  |         |         |       x |       x |         |       x |
    | :match_string          | Hash    |         |         |       x |       x |         |       x |
    | :max_nesting           | Fixnum  |       4 |       4 |       x |       x |       4 |       4 |
    | :mode                  | Symbol  |       - |       - |       - |       - |       - |       - |
    | :nan                   | Symbol  |         |         |         |         |         |       x |
    | :nilnil                | Boolean |         |         |         |         |         |       x |
    | :object_class          | Class   |         |         |       x |       x |         |       x |
    | :object_nl             | String  |         |         |       x |       x |         |       x |
    | :omit_nil              | Boolean |       x |       x |       x |       x |       x |       x |
    | :quirks_mode           | Boolean |         |         |       5 |         |         |       x |
    | :second_precision      | Fixnum  |         |         |         |         |       x |       x |
    | :space                 | String  |         |         |       x |       x |         |       x |
    | :space_before          | String  |         |         |       x |       x |         |       x |
    | :symbol_keys           | Boolean |       x |       x |       x |       x |       x |       x |
    | :time_format           | Symbol  |         |         |         |         |       x |       x |
    | :use_as_json           | Boolean |         |         |         |       x |         |       x |
    | :use_to_hash           | Boolean |         |         |         |         |         |       x |
    | :use_to_json           | Boolean |         |         |         |         |         |       x |
     ----------------------------------------------------------------------------------------------

 1. :allow_blank an alias for :nilnil.

 2. The :ascii_only options is an undocumented json gem option.

 3. The integer indent value in the default options will be honored by since
    the json gem expects a String type the indent in calls to 'to_json()',
    'Oj.generate()', or 'Oj.generate_fast()' expect a String and not an
    integer.

 4. The max_nesting option is for the json gem and rails only. It exists for
    compatibility. For other Oj dump modes the maximum nesting is set to over
    1000. If reference loops exist in the object being dumped then using the
    `:circular` option is a far better choice. It adds a slight overhead but
    detects an object that appears more than once in a dump and does not dump
    that object a second time.

 5. The quirks mode option is no longer supported in the most recent json
    gem. It is supported by Oj for backward compatibility with older json gem
    versions.

