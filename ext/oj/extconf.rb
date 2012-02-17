require 'mkmf'

$CPPFLAGS += ' -Wall'
#puts "*** $CPPFLAGS: #{$CPPFLAGS}"
extension_name = 'oj'
dir_config(extension_name)
create_makefile(extension_name)
