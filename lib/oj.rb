# Optimized JSON (Oj), as the name implies was written to provide speed
# optimized JSON handling.
#
# Oj uses modes to control how object are encoded and decoded. In addition
# global and options to methods allow additional behavior modifications. The
# modes are:
#
# - :strict mode will only allow the 7 basic JSON types to be serialized. Any other Object
#   will raise an Exception. 
# 
# - :null mode is similar to the :strict mode except any Object that is not
#   one of the JSON base types is replaced by a JSON null.
# 
# - :object mode will dump any Object as a JSON Object with keys that match
#   the Ruby Object's variable names without the '@' character. This is the
#   highest performance mode.
# 
# - :compat or :json mode is the compatible mode for the json gem. It mimics
#   the json gem including the options, defaults, and restrictions.
#
# - :rails is the compatibility mode for Rails or Active support.
#
# - :custom is the most configurable mode.
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
require 'oj/easy_hash'
require 'oj/error'
require 'oj/mimic'
require 'oj/saj'
require 'oj/schandler'

require 'oj/oj' # C extension
