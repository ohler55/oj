# frozen_string_literal: true

source 'https://rubygems.org'

gemspec

group :development do
  gem 'minitest', '~> 5'
  gem 'rake-compiler', '>= 0.9', '< 2.0'
  gem 'rubocop', '~> 1.47', require: false
  gem 'rubocop-minitest', '~> 0.28.0', require: false
  gem 'test-unit', '~> 3.0'
  gem 'ruby_memcheck', '~> 3.0' if RUBY_PLATFORM.include?('linux')

  # Duel requires Ruby 3.4 or later
  gem 'duel', '>= 0.1.4' if Gem::Version.new(RUBY_VERSION) >= Gem::Version.new('3.4')
  gem 'stackprof', platforms: :mri
end
