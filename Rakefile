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
  STDOUT.flush
  exitcode = 0
  status = 0

  cmds = "ruby test/tests.rb -v && ruby test/tests_mimic.rb -v && ruby test/tests_mimic_addition.rb -v"

  STDOUT.syswrite "\n#{'#'*90}\n#{cmds}\n"
  Bundler.with_original_env do
    status = system(cmds)
  end
  exitcode = 1 unless status

  # Verifying that json gem tests work for native implementation for Ruby 2.4.0
  # and above only. We know the older versions do not pass the 2.4.0 unit
  # tests.
  if RUBY_VERSION >= '2.4'
    Dir.glob('test/json_gem/*_test.rb').each do |file|
      cmd = "bundle exec ruby -Itest #{file}"
      STDOUT.syswrite "\n#{'#'*90}\n#{cmd}\n"
      Bundler.with_original_env do
        ENV['REAL_JSON_GEM'] = '1'
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

  if Rails.version =~ /^6\.\d/
    Rake::TestTask.new "activesupport6" do |t|
      t.libs << 'test'
      t.pattern = 'test/activesupport6/*_test.rb'
      t.warning = true
      t.verbose = true
    end
    Rake::Task[:test_all].enhance ["activesupport6"]
  end

  if Rails.version =~ /7\.\d/
    Rake::TestTask.new "activesupport7" do |t|
      t.libs << 'test'
      t.pattern = 'test/activesupport7/*_test.rb'
      t.warning = true
      t.verbose = true
    end
    Rake::Task[:test_all].enhance ["activesupport7"]
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
