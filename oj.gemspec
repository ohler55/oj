
require 'date'
require File.join(File.dirname(__FILE__), 'lib/oj/version')

Gem::Specification.new do |s|
  s.name = "oj"
  s.version = ::Oj::VERSION
  s.authors = "Peter Ohler"
  s.date = Date.today.to_s
  s.email = "peter@ohler.com"
  s.homepage = "http://www.ohler.com/oj"
  s.summary = "A fast JSON parser and serializer."
  s.description = %{The fastest JSON parser and object serializer. }
  s.licenses = ['MIT', 'GPL-3.0']

  s.files = Dir["{lib,ext,test}/**/*.{rb,h,c}"] + ['LICENSE', 'README.md']

  s.extensions = ["ext/oj/extconf.rb"]

  s.require_paths = ["lib", "ext"]

  s.has_rdoc = true
  s.extra_rdoc_files = ['README.md']
  s.rdoc_options = ['--main', 'README.md']

  s.rubyforge_project = 'oj'

  s.add_development_dependency 'minitest', '~> 5'
  s.add_development_dependency 'minitest-around', '~> 0.2'
  s.add_development_dependency 'rails', '~> 4'
end
