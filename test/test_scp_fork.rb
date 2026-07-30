#!/usr/bin/env ruby
# frozen_string_literal: true

$LOAD_PATH << __dir__

require 'helper'
require 'scp_handlers'

# The two Oj.sc_parse tests that read from a pipe the test itself forks a
# writer for. They are here rather than in test_scp.rb because Valgrind traces
# the forked child as well and both processes write to the one XML report,
# which leaves it truncated, so rake test:valgrind skips this file.
class ScpForkTest < Minitest::Test

  def test_pipe
    skip 'needs fork' unless Process.respond_to?(:fork)

    handler = AllHandler.new()
    json = %{{"one":true,"two":false}}
    IO.pipe do |read_io, write_io|
      if fork
        write_io.close
        Oj.sc_parse(handler, read_io)
        read_io.close
        assert_equal([[:hash_start],
                      [:hash_key, 'one'],
                      [:hash_set, 'one', true],
                      [:hash_key, 'two'],
                      [:hash_set, 'too', false],
                      [:hash_end],
                      [:add_value, {}]], handler.calls)
      else
        read_io.close
        write_io.write json
        write_io.close
        Process.exit(0)
      end
    end
  end

  def test_pipe_close
    skip 'needs fork' unless Process.respond_to?(:fork)

    json = %{{"one":true,"two":false}}
    IO.pipe do |read_io, write_io|
      if fork
        write_io.close
        handler = Closer.new(read_io)
        err = nil
        begin
          Oj.sc_parse(handler, read_io)
          read_io.close
        rescue Exception => e
          err = e.class.to_s
        end
        assert_equal('IOError', err)
        assert_equal([[:hash_start],
                      [:hash_key, 'one'],
                      [:hash_set, 'one', true]], handler.calls)
      else
        read_io.close
        write_io.write json[0..11]
        sleep(0.1)
        begin
          write_io.write json[12..]
        rescue Exception
          # ignore, should fail to write
        end
        write_io.close
        Process.exit(0)
      end
    end
  end

end
