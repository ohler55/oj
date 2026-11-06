#!/usr/bin/env ruby
# frozen_string_literal: true

$LOAD_PATH << __dir__

require 'helper'

class ParserCacheTest < Minitest::Test

  # Expired cache entries must be freed when a parser is collected. The native
  # leak assertion is supplied by rake test:valgrind, not Ruby's heap counters.
  def test_collect_parser_with_expired_cache_entries
    json = '{' + 100.times.map { |i| %("key#{i}":#{i}) }.join(',') + '}'

    10.times do
      parser = Oj::Parser.new(:usual)
      parser.cache_expunge = 3
      assert_equal(100, parser.parse(json).size)
      6.times { GC.start(full_mark: true, immediate_sweep: true) }
      assert_instance_of(Oj::Parser, parser)
      parser = nil # rubocop:disable Lint/UselessAssignment -- Release the parser before GC.
      GC.start(full_mark: true, immediate_sweep: true)
    end
  end

end
