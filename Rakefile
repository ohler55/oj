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
  [
   'test/tests.rb', # basic tests
   #Dir.glob('test/isolated/test_*.rb'), # tests mimic over-ride of JSON gem functions
   'test/isolated_compatibility/test_compatibility_json.rb',
   Dir.glob('text/json_gem/json_*.rb'),
   #'test/isolated_compatibility/test_compatibility_rails.rb',
   ].flatten.each do |file|
    cmd = "ruby -Itest #{file}"
    puts "\n" + "#"*90
    puts cmd
    Bundler.with_clean_env do
      status = system(cmd)
    end
    exitcode = 1 unless status
  end

  # verifying that JSON tests work for native implemntation
  Dir.glob('test/json_gem/*_test.rb').each do |file|
    cmd = "ruby -Itest #{file}"
    puts "\n" + "#"*90
    puts cmd
    Bundler.with_clean_env do
      status = system(cmd)
    end
    exitcode = 1 unless status
  end

  # run JSON tests for Oj.mimic_JSON
  Dir.glob('test/json_gem/*_test.rb').each do |file|
    cmd = "MIMIC_JSON=1 ruby -Itest #{file}"
    puts "\n" + "#"*90
    puts cmd
    Bundler.with_clean_env do
      status = system(cmd)
    end
    exitcode = 1 unless status
  end

  Rake::Task['test'].invoke
  exit(1) if exitcode == 1
end

task :default => :test_all
