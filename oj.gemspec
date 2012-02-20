
require 'date'
require File.join(File.dirname(__FILE__), 'lib/oj/version')

Gem::Specification.new do |s|
  s.name = "oj"
  s.version = ::Oj::VERSION
  s.authors = "Peter Ohler"
  s.date = Date.today.to_s
  s.email = "peter@ohler.com"
  s.homepage = "https://github.com/ohler55/oj"
  s.summary = "A fast JSON parser and serializer."
  s.description = %{The fastest JSON parser and object serializer. }

  s.files = Dir["{lib,ext,test}/**/*.{rb,h,c}"] + ['LICENSE', 'README.md']

  s.extensions = ["ext/oj/extconf.rb"]
  # s.executables = []

  s.require_paths = ["lib", "ext"]

  s.has_rdoc = true
  s.extra_rdoc_files = ['README.md']
  s.rdoc_options = ['--main', 'README.md']
  
  s.rubyforge_project = 'oj'
end
