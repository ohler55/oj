#!/usr/bin/env ruby -wW1

$: << '.'
$: << '..'
$: << '../lib'
$: << '../ext'

if __FILE__ == $0
  if (i = ARGV.index('-I'))
    x,path = ARGV.slice!(i, 2)
    $: << path
  end
end

require 'optparse'
require 'ox'
require 'oj'
require 'sample'
require 'files'

$verbose = 0
$circular = false

do_sample = false
do_files = false

do_load = false
do_dump = false
do_read = false
do_write = false
$iter = 1000

opts = OptionParser.new
opts.on("-v", "increase verbosity")                         { $verbose += 1 }

opts.on("-c", "circular options")                           { $circular = true }

opts.on("-s", "load and dump as sample Ruby object")        { do_sample = true }
opts.on("-f", "load and dump as files Ruby object")         { do_files = true }

opts.on("-l", "load")                                       { do_load = true }
opts.on("-d", "dump")                                       { do_dump = true }
opts.on("-r", "read")                                       { do_read = true }
opts.on("-w", "write")                                      { do_write = true }
opts.on("-a", "load, dump, read and write")                 { do_load = true; do_dump = true; do_read = true; do_write = true }

opts.on("-i", "--iterations [Int]", Integer, "iterations")  { |i| $iter = i }

opts.on("-h", "--help", "Show this display")                { puts opts; Process.exit!(0) }
files = opts.parse(ARGV)

if files.empty?
  data = []
  obj = do_sample ? sample_doc(2) : files('..')
  mars = Marshal.dump(obj)
  xml = Ox.dump(obj, :indent => 0, circular: $circular)
  json = Oj.dump(obj, :indent => 0, circular: $circular)
  File.open('sample.xml', 'w') { |f| f.write(xml) }
  File.open('sample.json', 'w') { |f| f.write(json) }
  File.open('sample.marshal', 'w') { |f| f.write(mars) }
  data << { :file => 'sample.xml', :obj => obj, :xml => xml, :marshal => mars, :json => json }
else
  puts "loading and parsing #{files}\n\n"
  # TBD change to allow xml and json
  data = files.map do |f|
    xml = File.read(f)
    obj = Ox.load(xml);
    mars = Marshal.dump(obj)
    json = Oj.dump(obj, :indent => 0, circular: $circular)
    { :file => f, :obj => obj, :xml => xml, :marshal => mars, :json => json }
  end
end

$ox_load_time = 0
$mars_load_time = 0
$ox_dump_time = 0
$oj_dump_time = 0
$mars_dump_time = 0

def perf_load(d)
  filename = d[:file]
  marshal_filename = 'sample.marshal'
  xml = d[:xml]
  mars = d[:marshal]
  json = d[:json]

  if 0 < $verbose
    obj = Ox.load(xml, :mode => :object, :trace => $verbose)
    return
  end
  start = Time.now
  (1..$iter).each do
    obj = Ox.load(xml, :mode => :object)
  end
  $ox_load_time = Time.now - start
  puts "Parsing #{$iter} times with Ox took #{$ox_load_time} seconds."

  start = Time.now
  (1..$iter).each do
    obj = Oj.load(json, :mode => :object)
  end
  $oj_load_time = Time.now - start
  puts "Parsing #{$iter} times with Oj took #{$oj_load_time} seconds."
  
  start = Time.now
  (1..$iter).each do
    obj = Marshal.load(mars)
  end
  $mars_load_time = Time.now - start
  puts "Marshalling #{$iter} times took #{$mars_load_time} seconds."
  puts ">>> Ox is %0.1f faster than Marshal loading.\n\n" % [$mars_load_time/$ox_load_time]
end

def perf_dump(d)
  obj = d[:obj]

  start = Time.now
  (1..$iter).each do
    xml = Ox.dump(obj, :indent => 2, :circular => $circular)
    #puts "*** ox:\n#{xml}"
  end
  $ox_dump_time = Time.now - start
  puts "Ox dumping #{$iter} times with ox took #{$ox_dump_time} seconds."

  Oj.default_options = {:indent => 2}
  start = Time.now
  (1..$iter).each do
    json = Oj.dump(obj)
  end
  $oj_dump_time = Time.now - start
  puts "Oj dumping #{$iter} times with oj took #{$oj_dump_time} seconds."

  obj = d[:obj]
  start = Time.now
  (1..$iter).each do
    m = Marshal.dump(obj)
  end
  $mars_dump_time = Time.now - start
  puts "Marshal dumping #{$iter} times took #{$mars_dump_time} seconds."
  puts ">>> Ox is %0.1f faster than Marshal dumping.\n\n" % [$mars_dump_time/$ox_dump_time]
end

def perf_read(d)
  ox_read_time = 0
  mars_read_time = 0

  filename = d[:file]
  marshal_filename = 'sample.marshal'
  xml = d[:xml]
  mars = d[:marshal]

  # now load from the file
  start = Time.now
  (1..$iter).each do
    obj = Ox.load_file(filename, :mode => :object)
  end
  ox_read_time = Time.now - start
  puts "Loading and parsing #{$iter} times with ox took #{ox_read_time} seconds."

  start = Time.now
  (1..$iter).each do
    m = File.read(marshal_filename)
    obj = Marshal.load(m)
  end
  mars_read_time = Time.now - start
  puts "Reading and marshalling #{$iter} times took #{mars_read_time} seconds."
  puts ">>> Ox is %0.1f faster than Marshal loading and parsing.\n\n" % [mars_read_time/ox_read_time]

end

def perf_write(d)
  ox_write_time = 0
  mars_write_time = 0
  
  ox_filename = 'out.xml'
  marshal_filename = 'out.marshal'
  obj = d[:obj]

  start = Time.now
  (1..$iter).each do
    xml = Ox.to_file(ox_filename, obj, :indent => 0)
  end
  ox_write_time = Time.now - start
  puts "Ox dumping #{$iter} times with ox took #{ox_write_time} seconds."

  start = Time.now
  (1..$iter).each do
    m = Marshal.dump(obj, circular: $circular)
    File.open(marshal_filename, "w") { |f| f.write(m) }
  end
  mars_write_time = Time.now - start
  puts "Marshal dumping and writing #{$iter} times took #{mars_write_time} seconds."
  puts ">>> Ox is %0.1f faster than Marshal dumping.\n\n" % [mars_write_time/ox_write_time]

end

#if do_sample or do_files
  data.each do |d|
    puts "Using file #{d[:file]}."
    
    perf_load(d) if do_load
    perf_dump(d) if do_dump
    if do_load and do_dump
      puts ">>> Ox is %0.1f faster than Marshal dumping and loading.\n\n" % [($mars_load_time + $mars_dump_time)/($ox_load_time + $ox_dump_time)] unless 0 == $mars_load_time
    end

    perf_read(d) if do_read
    perf_write(d) if do_write

  end
#end

