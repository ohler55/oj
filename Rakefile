#!/usr/bin/env rake

require 'bundler/gem_tasks'
require 'rake/extensiontask'
require 'rake/testtask'

Rake::ExtensionTask.new('oj') do |ext|
  ext.lib_dir = 'lib/oj'
end

=begin
Rake::TestTask.new(:test) do |test|
  test.libs << 'test'
  test.pattern = 'test/test_*.rb'
  test.options = "-v"
end
=end

task :test_all => [:clean, :compile] do
  exitcode = 0
  status = 0

  cmds = "ruby test/tests.rb && ruby test/tests_mimic.rb && ruby test/tests_mimic_addition.rb"
  puts "\n" + "#"*90
  puts cmds
  Bundler.with_original_env do
    status = system(cmds)
  end
  exitcode = 1 unless status

  # Verifying that json gem tests work for native implementation for Ruby 2.4.0
  # and above only. We know the older versions do not pass the 2.4.0 unit
  # tests.
  if RUBY_VERSION >= '2.4'
    Dir.glob('test/json_gem/*_test.rb').each do |file|
      cmd = "REAL_JSON_GEM=1 bundle exec ruby -Itest #{file}"
      puts "\n" + "#"*90
      puts cmd
      Bundler.with_original_env do
        status = system(cmd)
      end
      exitcode = 1 unless status
    end
  end

  Rake::Task['test'].invoke
  exit(1) if exitcode == 1
end

task :default => :test_all

begin
  require "rails/version"

  if Rails.version =~ /^4\.\d/
    Rake::TestTask.new "activesupport4" do |t|
      t.libs << 'test'
      t.pattern = 'test/activesupport4/*_test.rb'
      t.warning = true
      t.verbose = true
    end
    Rake::Task[:test_all].enhance ["activesupport4"]
  end

  if Rails.version =~ /^5\.\d/
    Rake::TestTask.new "activesupport5" do |t|
      t.libs << 'test'
      t.pattern = 'test/activesupport5/*_test.rb'
      t.warning = true
      t.verbose = true
    end
    Rake::Task[:test_all].enhance ["activesupport5"]
  end

  if Rails.version =~ /^6\.\d/
    Rake::TestTask.new "activesupport6" do |t|
      t.libs << 'test'
      t.pattern = 'test/activesupport6/*_test.rb'
      t.warning = true
      t.verbose = true
    end
    Rake::Task[:test_all].enhance ["activesupport6"]
  end

  Rake::TestTask.new "activerecord" do |t|
    t.libs << 'test'
    t.pattern = 'test/activerecord/*_test.rb'
    t.warning = true
    t.verbose = true
  end
  Rake::Task[:test_all].enhance ["activerecord"]

rescue LoadError
end
