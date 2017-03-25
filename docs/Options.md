# Oj Options

To change default serialization mode use the following form. Attempting to
modify the Oj.default_options Hash directly will not set the changes on the
actual default options but on a copy of the Hash:

```ruby
Oj.default_options = {:mode => :compat }
```

### Common (for serializer and parser) options

 * `:mode` [Symbol] mode for dumping and loading JSON. **Important format details [here](http://www.ohler.com/dev/oj_misc/encoding_format.html)**

    - `:object` mode will dump any `Object` as a JSON `Object` with keys that
      match the Ruby `Object`'s variable names without the '@' prefix
      character. This mode has the best performance and is the default

    - `:strict` mode will only allow the 7 basic JSON types to be serialized. Any
      other `Object` will raise an `Exception`

    - `:null` mode replaces any `Object` that is not one of the JSON types with a JSON `null`

    - `:compat` mode attempts to be compatible with other systems. It will
      serialize any `Object`, but will check to see if the `Object` implements an
      `to_hash` or `to_json` method. If either exists, that method is used for
      serializing the `Object`.  Since `as_json` is more flexible and produces
      more consistent output, it is preferred over the `to_json` method. If
      neither the `to_json` or `to_hash` methods exists, then the Oj internal
      `Object` variable encoding is used

 * `:time_format` [Symbol] time format when dumping in :compat and :object mode

    - `:unix` time is output as a decimal number in seconds since epoch including
      fractions of a second.

    - `:unix_zone` similar to the `:unix` format but with the timezone encoded in
      the exponent of the decimal number of seconds since epoch.

    - `:xmlschema` time is output as a string that follows the XML schema definition.

    - `:ruby` time is output as a string formatted using the Ruby `to_s` conversion.

 * `:second_precision` [Fixnum] number of digits after the decimal when dumping
   the seconds portion of time

 * `:bigdecimal_as_decimal` [Boolean] dump BigDecimal as a decimal number
   or as a String

 * `:create_id` [String] create id for json compatible object encoding,
   default is `json_create`.

 * `:allow_gc` [Boolean] allow or prohibit GC during parsing, default is
   true (allow).

 * `:quirks_mode` [Boolean] Allow single JSON values instead of
   documents, default is true (allow).

 * `:allow_invalid_unicode` [Boolean] Allow invalid unicode, default is
   false (don't allow)

 * `:omit_nil` [Boolean] If true, Hash and Object attributes with nil values
   are omitted

### Serializer options

 * `:indent` [Fixnum] number of spaces to indent each element in an JSON
   document, zero is no newline between JSON elements, negative indicates no
   newline between top level JSON elements in a stream
   
 * `:circular` [Boolean] support circular references while dumping

 * `:escape_mode` [Symbol] determines the characters to escape

    - `:newline` allows unescaped newlines in the output

    - `:json` follows the JSON specification. This is the default mode

    - `:xss_safe` escapes HTML and XML characters such as `&` and `<`

    - `:ascii` escapes all non-ascii or characters with the hi-bit set

 * `:use_to_json` [Boolean] call `to_json()` methods on dump, default is
   false

 * `:use_as_json` [Boolean] call `as_json()` methods on dump, default is
   false

 * `:nan` [Symbol] How to dump Infinity, -Infinity, and
   NaN in :null, :strict, and :compat mode. Default is :auto

    - `:null` places a null

    - `:huge` places a huge number

    - `:word` places Infinity or NaN

    - `:raise` raises and exception

    - `:auto` uses default for each mode which are `:raise` for `:strict`, `:null` for `:null`, and `:word` for `:compat`

### Parser options

 * `:auto_define` [Boolean] automatically define classes if they do not
   exist

 * `:symbol_keys` [Boolean] use symbols instead of strings for hash keys

 * `:class_cache` [Boolean] cache classes for faster parsing (if
   dynamically modifying classes or reloading classes then don't use this)

 * `:bigdecimal_load` [Symbol] load decimals as BigDecimal instead of as a
   Float

    - `:bigdecimal` convert all decimal numbers to BigDecimal

    - `:float` convert all decimal numbers to Float

    - `:auto` the most precise for the number of digits is used

 * `:float_precision` [Fixnum] number of digits of precision when dumping
   floats, 0 indicates use Ruby

 * `:nilnil` [Boolean] if true a nil input to load will return nil and
   not raise an Exception

 * `:empty_string` [Boolean] if true an empty input will not raise an
   Exception, default is true (allow). When Oj.mimic_JSON is used,
   default is false (raise exception when empty string is encountered)

 * `:hash_class` [Class] Class to use instead of Hash on load
