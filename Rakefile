#!/usr/bin/env rake
# frozen_string_literal: true

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
  $stdout.flush
  exitcode = 0
  status = 0

  cmds = 'bundle exec ruby test/tests.rb -v && bundle exec ruby test/tests_mimic.rb -v && bundle exec ruby test/tests_mimic_addition.rb -v'

  $stdout.syswrite "\n#{'#'*90}\n#{cmds}\n"
  Bundler.with_original_env do
    status = system(cmds)
  end
  exitcode = 1 unless status

  Dir.glob('test/json_gem/*_test.rb').each do |file|
    cmd = "bundle exec ruby -Itest #{file}"
    $stdout.syswrite "\n#{'#'*90}\n#{cmd}\n"
    Bundler.with_original_env do
      ENV['REAL_JSON_GEM'] = '1'
      status = system(cmd)
    end
    exitcode = 1 unless status
  end

  Rake::Task['test'].invoke
  exit(1) if exitcode == 1
end

task :default => :test_all

begin
  require 'rails/version'

  Rake::TestTask.new "activesupport#{Rails::VERSION::MAJOR}" do |t|
    t.libs << 'test'
    t.pattern = "test/activesupport#{Rails::VERSION::MAJOR}/*_test.rb"
    t.warning = true
    t.verbose = true
  end
  Rake::Task[:test_all].enhance ["activesupport#{Rails::VERSION::MAJOR}"]

  Rake::TestTask.new 'activerecord' do |t|
    t.libs << 'test'
    t.pattern = 'test/activerecord/*_test.rb'
    t.warning = true
    t.verbose = true
  end
  Rake::Task[:test_all].enhance ['activerecord']
rescue LoadError => e
  puts "Rake failed #{e}"
end
