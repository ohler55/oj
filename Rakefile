#!/usr/bin/env rake
require 'bundler/gem_tasks'
require 'rake/testtask'

Rake::TestTask.new(:test) do |test|
  test.libs << 'test'
  test.pattern = 'test/test_*.rb'
end

task :test_all do
  exit_status = 0

  Dir.glob(File.join('test', 'isolated', '*.rb')).each do |isolated|
    rout, wout = IO.pipe
    puts "File: #{isolated}"
    pid = Process.spawn("ruby -Itest #{isolated}", out: wout, pgroup: 0)
    _, status = Process.wait2(pid)
    wout.close
    exit_status = status.exitstatus if status.exitstatus != 0
    puts rout.readlines
    puts
  end

  Rake::Task['test'].invoke
  exit(exit_status) if exit_status > 0
end

task :default => :test_all
