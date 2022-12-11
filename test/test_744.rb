#!/usr/bin/env ruby 

test_dir = File.dirname(__FILE__)
$oj_dir = File.dirname(File.expand_path(File.dirname(__FILE__)))
%w(lib ext).each do |dir|
  $: << File.join($oj_dir, dir)
end

require 'oj'
require 'parallel'
require 'json'

data_dir     = File.join(test_dir, "test_744-data")
events_file  = File.join(data_dir, "small-dataset")
summary_file = File.join(data_dir, "summary.json")

# this file needs to be loded, or probably a whole lot of memory needs to be allocated in order to
# Get the GC into a state where it might actually run.
#
# using a different parser for this other file just to be sure
summary = JSON.load(IO.read(summary_file))
file_order = summary["aircrafts"].values.sort_by { |s| s["record_offset"] }

# This is extracted from the codebase - a parent process reads chunks of lines from
# the input file, does a quick parse of the # json and then makes some decisions
# then the raw original array of input strings from the input is used
def read_lines(input, count)
  Array.new.tap do |a|
    count.times do
      data = input.gets
      break if data.nil?
      # replicating what happens on initial read, this does nothing here, but its
      # what happens in the upstream code. It is thrown away here
      Oj::Doc.open(data) do |doc|
        doc.fetch("/t")
        doc.fetch("/ms")
      end
      a << data
    end
  end
end

queue = ::SizedQueue.new(200)

# In a producer thread, the parent process reads chunks of lines from the input file
# The parent process loads the input file of newline oriented json and puts an array
# on the SizedQueue of with element 0 being an array of lines read from the input file
# and element 1 being - effectively a random string of up to 6 chars.
#
# Each readline_lines invociation will generally consume somewhere
# between 1..200 lines of input from the events file.
#
Thread.new do
  File.open(events_file) do |input|
    file_order.each_with_index do |aircraft_summary, i|
      aircraft_events = read_lines(input, aircraft_summary["event_count"])
      queue << [aircraft_events, aircraft_summary["hexid"]]
    end

    # poison pill in the queue
    queue << ::Parallel::Stop
  end
end

# Using the Parallel gem, spawn forked process workers that will each be
# fed 1 elemnt from the SizedQueue.
#
# The block here is in a forked child process. In the child process
# the input strings are then loaded into objects, a stripped down version
# is here as the Event.
#
# Yes, this does mean the JSON is parsed again in the child processe even though
# it was parsed once in the parent already too. That's the way it is in this
# application at the moment.
class Event
  def initialize(line = nil)
    @line = line
    @data = nil
    @hexid = nil
    @timestamp = nil
    # do a quick load of the data for most minor purposes, accessing any of
    # the other fields will result in a full parse
    quick_parse
  end

  def quick_parse
    Oj::Doc.open(@line) do |doc|
      @timestamp = doc.fetch("/t")
      @hexid = doc.fetch("/ms") || ""
    end
  end
end

Parallel.each(queue, in_processes: 24) do |event_list, hexid|
  begin

#    $stderr.puts "Worker #{Parallel.worker_number} start processing #{hexid} with #{event_list.size} events"
#
#    before = Time.now
    event_list.map{ |line| Event.new(line) }
#    after = Time.now
#
#    $stderr.puts "Worker #{Parallel.worker_number} finish processing #{hexid} with #{event_list.size} events in #{"%0.3f" % (after - before)} seconds"

    [Parallel.worker_number, hexid]
  rescue => e
  end
end

