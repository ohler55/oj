#!/usr/bin/env ruby 
# encoding: UTF-8
test_dir = File.dirname(__FILE__)
$oj_dir = File.dirname(File.expand_path(File.dirname(__FILE__)))
%w(lib ext).each do |dir|
  $: << File.join($oj_dir, dir)
end

require 'oj'
require 'parallel'
require 'securerandom'
require 'json'

data_dir     = File.join(test_dir, "test_744-data")
events_file  = File.join(data_dir, "airnav_events")
summary_file = File.join(data_dir, "airnav_summary.json")

$stderr.puts "Loading #{summary_file}"
summary = JSON.parse(IO.read(summary_file))
file_order = summary["aircrafts"].values.sort_by { |s| s["record_offset"] }


$stderr.puts "Loading #{events_file}"

queue = ::SizedQueue.new(20)

class Event
    def initialize(line = nil)
      @line = line
      @data = nil
      @hexid = nil
      @timestamp = nil
      Oj::Doc.open(@line) do |doc|
         @timestamp = doc.fetch("/t")
         @hexid = doc.fetch("/ms") || ""
      end
    end
end



def read_lines(input, count)
  Array.new.tap do |a|
    count.times do
      data = input.gets
      break if data.nil?
      #$stderr.puts "Length: #{data.length}"
      # replicating what happens on initial read
      #d = Oj::Doc.open(data)
      #d.close
      Oj::Doc.open(data) do |doc|
          doc.fetch("/t")
          doc.fetch("/ms")
      end
      a << data
    end
  end
end


# put the items on the aircraft_queue in a separate thread as the queue
# needs to have items on it when the parallel starts
Thread.new do

  File.open(events_file) do |input|
    while !input.eof? do
    #file_order.each_with_index do |aircraft_summary, i|
      aircraft_events = read_lines(input, Random.rand(1..200))
      #$stderr.puts " #{i+1} / #{file_order.size} #{aircraft_summary["hexid"]} #{aircraft_events.size} - #{so_far}/#{summary["event_count"]}"
      queue << [aircraft_events, SecureRandom.hex(3)]
    end

    # poison pill in the queue
    queue << ::Parallel::Stop
  end
end

# Fork process for each aircraft up to the number of processors on the
# machine - that is the default for parallel
#
begin
  Parallel.each(queue, in_processes: 8) do |event_list, hexid|
    #$stderr.puts "Worker #{Parallel.worker_number} #{Process.pid} start processing #{hexid} with #{event_list.size} events"

    before = Time.now
    event_list.map{ |line| Event.new(line) }
    after = Time.now

    #$stderr.puts "Worker #{Parallel.worker_number} #{Process.pid} finish processing #{hexid} with #{event_list.size} events in #{"%0.3f" % (after - before)} seconds"

    [Parallel.worker_number, hexid]
  end
rescue ::Parallel::DeadWorker => e
  $stderr.puts "Dead Worker!: #{e}"
end

