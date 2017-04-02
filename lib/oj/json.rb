
require 'oj/state'

module JSON
  NaN = 0.0/0.0 unless defined?(::JSON::NaN)
  Infinity = 1.0/0.0 unless defined?(::JSON::Infinity)
  MinusInfinity = -1.0/0.0 unless defined?(::JSON::MinusInfinity)
  # Taken from the unit test. Note that items like check_circular? are not
  # present.
  PRETTY_STATE_PROTOTYPE = Ext::Generator::State.from_state({
                                              :allow_nan             => false,
                                              :array_nl              => "\n",
                                              :ascii_only            => false,
                                              :buffer_initial_length => 1024,
                                              :depth                 => 0,
                                              :indent                => "  ",
                                              :max_nesting           => 100,
                                              :object_nl             => "\n",
                                              :space                 => " ",
                                              :space_before          => "",
                                            }) unless defined?(::JSON::PRETTY_STATE_PROTOTYPE)
  SAFE_STATE_PROTOTYPE = Ext::Generator::State.from_state({
                                            :allow_nan             => false,
                                            :array_nl              => "",
                                            :ascii_only            => false,
                                            :buffer_initial_length => 1024,
                                            :depth                 => 0,
                                            :indent                => "",
                                            :max_nesting           => 100,
                                            :object_nl             => "",
                                            :space                 => "",
                                            :space_before          => "",
                                            }) unless defined?(::JSON::SAFE_STATE_PROTOTYPE)
  FAST_STATE_PROTOTYPE = Ext::Generator::State.from_state({
                                            :allow_nan             => false,
                                            :array_nl              => "",
                                            :ascii_only            => false,
                                            :buffer_initial_length => 1024,
                                            :depth                 => 0,
                                            :indent                => "",
                                            :max_nesting           => 0,
                                            :object_nl             => "",
                                            :space                 => "",
                                            :space_before          => "",
                                            }) unless defined?(::JSON::FAST_STATE_PROTOTYPE)

  def self.dump_default_options
    Oj::MimicDumpOption.new
  end

  def self.dump_default_options=(h)
    m = Oj::MimicDumpOption.new
    h.each do |k,v|
      m[k] = v
    end
  end

  def self.parser=(p)
    @@parser = p
  end

  def self.parser()
    @@parser
  end

  def self.generator=(g)
    @@generator = g
  end

  def self.generator()
    @@generator
  end

  module Ext
    class Parser
      attr_reader :source

      def initialize(src)
        @source = src
      end

      def parse()
        JSON.parse(@source)
      end
      
    end # Parser
  end # Ext
  
  State = ::JSON::Ext::Generator::State

  Parser = ::JSON::Ext::Parser
  #self.parser = ::JSON::Ext::Parser
  #self.generator = ::JSON::Ext::Generator
  
end # JSON




