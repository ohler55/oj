#!/usr/bin/env rake
# frozen_string_literal: true

require 'bundler/gem_tasks'
require 'rake/extensiontask'
require 'rake/testtask'

Rake::ExtensionTask.new('oj') do |ext|
  ext.lib_dir = 'lib/oj'
end

# The one list both `rake test` and `rake test:valgrind` start from, so a new
# test/test_*.rb is picked up by both without having to be added by hand
# anywhere. test/isolated/ is not matched: each of those needs its own process
# with specific setup. Neither is test/json_gem/ (test_all runs it separately)
# nor the activesupport suites (they have their own tasks).
TEST_FILES = FileList['test/test_*.rb'].to_a.sort.freeze

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

    namespace :test do
      task :check_valgrind do
        unless system('command -v valgrind > /dev/null 2>&1')
          abort("\nValgrind is required for `rake test:valgrind` but was not found.\n" \
                "Install it first (Linux only), e.g. `sudo apt-get install valgrind`.\n")
        end
      end

      # Same set as `rake test`, minus two files. ruby_memcheck loads every
      # file it is given into one process, and these two cannot share one with
      # the rest:
      #
      #   test_generate.rb  Oj.generate calls Oj::Rails.mimic_JSON implicitly,
      #                     after which a parse error is a JSON::ParserError
      #                     rather than an Oj::ParseError, so the tests in
      #                     test_max_integer_digits.rb that name the class fail.
      #                     Nothing undoes that, which is why the file belongs
      #                     alongside test/isolated/.
      #   test_scp_fork.rb  Valgrind traces the child it forks as well and both
      #                     processes write to the one XML report, which leaves
      #                     it truncated: "Premature end of data in tag
      #                     valgrindoutput".
      #
      # Both still run under `rake test`, which gives each file its own
      # process. Removing an entry from this list means fixing what it trips,
      # not editing the list.
      memcheck_test_files = TEST_FILES - %w[
        test/test_generate.rb
        test/test_scp_fork.rb
      ]

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

# test_all invokes this. While it was commented out Rake::Task['test'] silently
# resolved to a synthesized file task for the test/ directory, which has no
# actions, so none of the test/test_*.rb files ran from rake at all.
#
# Each file gets its own process, the same way test_all runs test/json_gem.
# These tests share process-global state -- Oj.default_options, the
# Oj::Parser.saj and Oj::Parser.usual singletons, Oj.mimic_JSON -- so loading
# them all into one process makes the result depend on collection order: with a
# shared process the suite passes on some seeds and fails on others. Isolating
# that state is worth doing, but it is a change to the tests rather than to how
# they are collected.
desc 'Run the test/test_*.rb files, each in its own process'
task :test do
  failed = []

  TEST_FILES.each do |file|
    cmd = "bundle exec ruby -Itest #{file} -v"
    $stdout.syswrite "\n#{'#' * 90}\n#{cmd}\n"
    ok = Bundler.with_original_env { system(cmd) }
    failed << file unless ok
  end
  abort("\nfailed: #{failed.join(' ')}\n") unless failed.empty?
end

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

  # A minor release only gets a suite of its own when its tests diverge enough
  # from the major one to matter, so prefer test/activesupport<major>.<minor>
  # and fall back to test/activesupport<major>. Naming the task after whichever
  # directory is used keeps `rake activesupport8.1` and `rake activesupport8`
  # pointing at the tests their names promise. A pattern that matches nothing
  # still exits 0, so an unresolved name would be a silently green job.
  as_suite = ["#{Rails::VERSION::MAJOR}.#{Rails::VERSION::MINOR}", Rails::VERSION::MAJOR.to_s].find do |v|
    Dir.exist?("test/activesupport#{v}")
  end
  raise "no test/activesupport directory for Rails #{Rails::VERSION::STRING}" if as_suite.nil?

  Rake::TestTask.new "activesupport#{as_suite}" do |t|
    t.libs << 'test'
    t.pattern = "test/activesupport#{as_suite}/*_test.rb"
    t.warning = true
    t.verbose = true
  end
  Rake::Task[:test_all].enhance ["activesupport#{as_suite}"]

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
