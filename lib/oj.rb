# Optimized JSON (Oj), as the name implies was written to provide speed
# optimized JSON handling.
# 
# Oj has several dump or serialization modes which control how Objects are
# converted to JSON. These modes are set with the :mode option in either the
# default options or as one of the options to the dump() method.
# 
# - :strict mode will only allow the 7 basic JSON types to be serialized. Any other Object
#   will raise and Exception. 
# 
# - :null mode replaces any Object that is not one of the JSON types is replaced by a JSON null.
# 
# - :object mode will dump any Object as a JSON Object with keys that match
#   the Ruby Object's variable names without the '@' character. This is the
#   highest performance mode.
# 
# - :compat mode is is the compatible with other systems. It will serialize
#   any Object but will check to see if the Object implements a to_hash() or
#   to_json() method. If either exists that method is used for serializing the
#   Object. The to_hash() is more flexible and produces more consistent output
#   so it has a preference over the to_json() method. If neither the to_json()
#   or to_hash() methods exist then the Oj internal Object variable encoding
#   is used.
module Oj
end

begin
  # This require exists to get around Rubinius failing to load bigdecimal from
  # the C extension.
  require 'bigdecimal'
rescue Exception
  # ignore
end

require 'oj/version'
require 'oj/bag'
require 'oj/error'
require 'oj/mimic'
require 'oj/saj'
require 'oj/schandler'

require 'oj/oj' # C extension
