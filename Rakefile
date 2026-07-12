#!/usr/bin/env rake
# frozen_string_literal: true

require 'bundler/gem_tasks'
require 'rake/extensiontask'
require 'rake/testtask'

Rake::ExtensionTask.new('oj') do |ext|
  ext.lib_dir = 'lib/oj'
end

if RUBY_PLATFORM.include?('linux')
  begin
    require 'ruby_memcheck'

    RubyMemcheck.config(
      binary_name: 'oj',
      # Valgrind and YJIT interfere with each other, adding noise and slowdown,
      # so keep YJIT disabled while running under Valgrind.
      ruby: "#{FileUtils::RUBY} --disable-yjit",
      # Keep the suppression file under test/ (it is only used by this task and
      # is not part of the packaged gem). ruby_memcheck still loads its own
      # bundled interpreter suppressions in addition to this directory.
      valgrind_suppressions_dir: 'test/valgrind'
    )

    # ruby_memcheck runs every listed file in a single process. Only the files
    # that test/tests.rb loads are known to coexist that way; the other
    # test_*.rb files mutate global state (Oj.default_options, mimic_JSON, ...)
    # or fork, and upstream runs those in their own processes. Mirror the
    # tests.rb set here, minus test_scp which forks and talks over a socket.
    memcheck_test_files = %w[
      test_compat test_custom test_fast test_file test_gc test_hash
      test_integer_range test_long_strings test_max_integer_digits test_null
      test_object test_rails test_saj test_strict test_wab test_writer
    ].map { |name| "test/#{name}.rb" }

    namespace :test do
      task :check_valgrind do
        unless system('command -v valgrind > /dev/null 2>&1')
          abort("\nValgrind is required for `rake test:valgrind` but was not found.\n" \
                "Install it first (Linux only), e.g. `sudo apt-get install valgrind`.\n")
        end
      end

      RubyMemcheck::TestTask.new(valgrind: [:check_valgrind, :compile]) do |t|
        t.libs << 'test'
        t.test_files = memcheck_test_files
        t.verbose    = true
      end
    end
  rescue LoadError
    # ruby_memcheck is an optional, Linux-only development dependency. If it is
    # not installed just skip defining the task instead of breaking the Rakefile.
  end
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
