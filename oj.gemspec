
require 'date'
require File.join(File.dirname(__FILE__), 'lib/oj/version')

Gem::Specification.new do |s|
  s.name = "oj"
  s.version = ::Oj::VERSION
  s.authors = "Peter Ohler"
  s.date = Date.today.to_s
  s.email = "peter@ohler.com"
  s.homepage = "http://www.ohler.com/oj"
  s.metadata = {
    "bug_tracker_uri" => "https://github.com/ohler55/oj/issues",
    "changelog_uri" => "https://github.com/ohler55/oj/blob/master/CHANGELOG.md",
    "documentation_uri" => "http://www.ohler.com/oj/doc/index.html",
    "homepage_uri" => "http://www.ohler.com/oj/",
    "source_code_uri" => "https://github.com/ohler55/oj",
    "wiki_uri" => "https://github.com/ohler55/oj/wiki"
  }
  s.summary = "A fast JSON parser and serializer."
  s.description = "The fastest JSON parser and object serializer."
  s.licenses = ['MIT']
  s.required_ruby_version = ">= 2.0"

  s.files = Dir["{lib,ext,test}/**/*.{rb,h,c}"] + ['LICENSE', 'README.md'] + Dir["pages/*.md"]
  s.test_files = Dir["test/**/*.rb"]
  s.extensions = ["ext/oj/extconf.rb"]

  s.extra_rdoc_files = ['README.md'] + Dir["pages/*.md"]
  s.rdoc_options = ['--title', 'Oj', '--main', 'README.md']

  s.add_development_dependency 'rake-compiler', '>= 0.9', '< 2.0'
  s.add_development_dependency 'minitest', '~> 5'
  s.add_development_dependency 'test-unit', '~> 3.0'
  s.add_development_dependency 'wwtd', '~> 0'
end
