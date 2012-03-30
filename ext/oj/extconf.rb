require 'mkmf'

extension_name = 'oj'
dir_config(extension_name)

parts = RUBY_DESCRIPTION.split(' ')
type = parts[0]
type = 'ree' if 'ruby' == type && RUBY_DESCRIPTION.include?('Ruby Enterprise Edition')
version = RUBY_VERSION.split('.')
puts ">>>>> Creating Makefile for #{type} version #{RUBY_VERSION} <<<<<"

dflags = {
  'RUBY_TYPE' => type,
  (type.upcase + '_RUBY') => nil,
  'RUBY_VERSION' => RUBY_VERSION,
  'RUBY_VERSION_MAJOR' => version[0],
  'RUBY_VERSION_MINOR' => version[1],
  'RUBY_VERSION_MICRO' => version[2],
  'HAS_RB_TIME_TIMESPEC' => ('ruby' == type && '1.9.3' == RUBY_VERSION) ? 1 : 0,
  'HAS_ENCODING_SUPPORT' => ('ruby' == type && '1' == version[0] && '9' == version[1]) ? 1 : 0,
  'HAS_NANO_TIME' => ('ruby' == type && '1' == version[0] && '9' == version[1]) ? 1 : 0,
  'HAS_RSTRUCT' => ('ruby' == type || 'ree' == type) ? 1 : 0,
  'HAS_IVAR_HELPERS' => ('ruby' == type && '1' == version[0] && '9' == version[1]) ? 1 : 0,
  'HAS_PROC_WITH_BLOCK' => ('ruby' == type && '1' == version[0] && '9' == version[1]) ? 1 : 0,
  'HAS_TOP_LEVEL_ST_H' => ('ruby' == type &&  '1' == version[0] && '8' == version[1]) ? 1 : 0,
}

dflags.each do |k,v|
  if v.nil?
    $CPPFLAGS += " -D#{k}"
  else
    $CPPFLAGS += " -D#{k}=#{v}"
  end
end
$CPPFLAGS += ' -Wall'
#puts "*** $CPPFLAGS: #{$CPPFLAGS}"
create_makefile(extension_name)
