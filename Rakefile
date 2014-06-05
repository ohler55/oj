#!/usr/bin/env rake
require 'bundler/gem_tasks'
require 'rake/extensiontask'
require 'rake/testtask'

Rake::ExtensionTask.new('oj') do |ext|
  ext.lib_dir = 'lib/oj'
end

Rake::TestTask.new(:test) do |test|
  test.libs << 'test'
  test.pattern = 'test/test_*.rb'
end

task :test_all => [:compile] do
  exitcode = 0

  Dir.glob(File.join('test', 'isolated', 'test_*.rb')).each do |isolated|
    rout, wout = IO.pipe
    puts "File: #{isolated}"
    status = system("ruby -Itest #{isolated}")
    exitcode = 1 unless status
  end

  Rake::Task['test'].invoke
  exit(1) if exitcode == 1
end

task :default => :test_all
