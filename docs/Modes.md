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

TBD

## :rails Mode

TBD

## :object Mode

TBD

## :custom Mode

TBD

## Options Matrix

Not all options are available in all modes. The options matrix identifies the
options available in each mode. An x in the matrix indicates the option is
supported in that mode. A number indicates the footnotes describe additional
information.

    | Option                 | type    | :null   | :strict | :compat | :rails  | :object | :custom |
    | ---------------------- | ------- | ------- | ------- | ------- | ------- | ------- | ------- |
    | :allow_gc              | Boolean |       x |       x |       x |       x |       x |       x |
    | :allow_invalid_unicode | Boolean |         |         |         |         |       x |       x |
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
    | :indent                | Integer |       x |       x |       1 |       ? |       x |       x |
    | :indent_str            | String  |         |         |       x |         |         |       x |
    | :mode                  | Symbol  |       - |       - |       - |       - |       - |       - |
    | :nan                   |         |         |         |         |         |         |       x |
    | :nilnil                |         |         |         |         |         |         |       x |
    | :object_nl             |         |         |         |         |         |         |       x |
    | :omit_nil              |         |         |         |         |         |         |       x |
    | :quirks_mode           |         |         |         |         |         |         |       x |
    | :second_precision      |         |         |         |         |         |         |       x |
    | :space                 |         |         |         |         |         |         |       x |
    | :space_before          |         |         |         |         |         |         |       x |
    | :symbol_keys           |         |         |         |         |         |         |       x |
    | :time_format           |         |         |         |         |         |         |       x |
    | :use_as_json           | Boolean |         |         |         |       x |         |       x |
    | :use_to_hash           | Boolean |         |         |         |         |         |       x |
    | :use_to_json           | Boolean |         |         |         |       ? |         |       x |

 1. The integer indent value in the default options will be honored by since
    the json gem expects a String type the indent in calls to 'to_json()',
    'Oj.generate()', or 'Oj.generate_fast()' expect a String and not an
    integer.
