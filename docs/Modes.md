# Oj Modes

Oj uses modes to switch the load and dump behavior. Initially Oj supported on
the :object mode which uses a format that allows Juby object encoding and
decoding in a manner that lets almost any Ruby object be encoded and decoded
without monkey patching the object classes. From that start other demands were
made the were best met by giving Oj multiple modes of operation. The current
modes are:

 - `:null`
 - `:strict`
 - `:compat` or `:json`
 - `:rails`
 - `:object`
 - `:custom`

## :null Mode

TBD

## :strict Mode

TBD

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

To revert back to unoptimized version just remove the Oj flag on that class.

```ruby
Oj.remove_to_json(Rational)
```

## :rails Mode

TBD

## :object Mode

TBD

## :custom Mode

TBD

## Options

### Options Matrix

Not all options are available in all modes. The options matrix identifies the
options available in each mode. An x in the matrix indicates the option is
supported in that mode. A number indicates the footnotes describe additional
information.

    | Option                 | type    | :null   | :strict | :compat | :rails  | :object | :custom |
    | ---------------------- | ------- | ------- | ------- | ------- | ------- | ------- | ------- |
    | :allow_blank           | Boolean |         |         |       1 |       1 |         |         |
    | :allow_gc              | Boolean |       x |       x |       x |       x |       x |       x |
    | :allow_invalid_unicode | Boolean |         |         |         |         |       x |       x |
    | :ascii_only            | Boolean |       x |       x |       2 |       2 |       x |       x |
    | :array_nl              |         |         |         |         |         |         |       x |
    | :auto_define           |         |         |         |         |         |         |       x |
    | :bigdecimal_as_decimal | Boolean |         |         |         |         |       x |       x |
    | :bigdecimal_load       |         |         |         |         |         |         |       x |
    | :circular              | Boolean |       x |       x |       x |       x |       x |       x |
    | :class_cache           | Boolean |         |         |         |         |       x |       x |
    | :create_id             |         |         |         |         |         |         |       x |
    | :empty_string          |         |         |         |         |         |         |       x |
    | :escape_mode           |         |         |         |         |         |         |       x |
    | :float_precision       |         |         |         |         |         |         |       x |
    | :hash_class            |         |         |         |         |         |         |       x |
    | :indent                | Integer |       x |       x |       3 |       ? |       x |       x |
    | :indent_str            | String  |         |         |       x |         |         |       x |
    | :mode                  | Symbol  |       - |       - |       - |       - |       - |       - |
    | :nan                   |         |         |         |         |         |         |       x |
    | :nilnil                |         |         |         |         |         |         |       x |
    | :object_nl             |         |         |         |         |         |         |       x |
    | :omit_nil              |         |         |         |         |         |         |       x |
    | :quirks_mode           |         |         |         |       4 |         |         |       x |
    | :second_precision      |         |         |         |         |         |         |       x |
    | :space                 |         |         |         |         |         |         |       x |
    | :space_before          |         |         |         |         |         |         |       x |
    | :symbol_keys           |         |         |         |         |         |         |       x |
    | :time_format           |         |         |         |         |         |         |       x |
    | :use_as_json           | Boolean |         |         |         |       x |         |       x |
    | :use_to_hash           | Boolean |         |         |         |         |         |       x |
    | :use_to_json           | Boolean |         |         |         |       ? |         |       x |

 1. :allow_blank an alias for :nilnil.

 2. The :ascii_only options is an undocumented json gem option.

 3. The integer indent value in the default options will be honored by since
    the json gem expects a String type the indent in calls to 'to_json()',
    'Oj.generate()', or 'Oj.generate_fast()' expect a String and not an
    integer.

 4. The quirks mode option is no longer supported in the most recent json
    gem. It is supported by Oj for backward compatibility with older json gem
    versions.

### Options Details

TBD