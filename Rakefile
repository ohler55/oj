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

task :test_all => [:compile] do
  exitcode = 0
  [
   File.join('test', 'tests.rb'), # basic tests
   #Dir.glob(File.join('test', 'isolated', 'test_*.rb')), # tests mimic over-ride of JSON gem functions
   File.join('test', 'isolated_compatibility', 'test_compatibility_json.rb'),
   #File.join('test', 'isolated_compatibility', 'test_compatibility_rails.rb'),
   ].flatten.each do |isolated|
    rout, wout = IO.pipe
    puts "\n" + "-"*10 + " File: #{isolated} " + "-"*10
    status = system("ruby -Itest #{isolated}")
    exitcode = 1 unless status
  end

  Rake::Task['test'].invoke
  exit(1) if exitcode == 1
end

task :default => :test_all
