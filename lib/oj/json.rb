require 'oj/state'

if defined?(JSON::PRETTY_STATE_PROTOTYPE)
  # There are enough people that try to use both the json gen and oj in mimic
  # mode so don't display the warning.

  # warn "WARNING: oj/json is a compatability shim used by Oj. Requiring the file explicitly is not recommended."
end

unless defined?(JSON::PRETTY_STATE_PROTOTYPE)
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
      h.each do |k, v|
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
        def initialize(src)
          raise TypeError.new("already initialized") unless @source.nil?

          @source = src
        end

        def source()
          raise TypeError.new("already initialized") if @source.nil?

          @source
        end

        def parse()
          raise TypeError.new("already initialized") if @source.nil?

          JSON.parse(@source)
        end

      end # Parser
    end # Ext

    State = ::JSON::Ext::Generator::State unless defined?(::JSON::State)

    begin
      send(:remove_const, :Parser)
    rescue
      # ignore and move on
    end
    Parser = ::JSON::Ext::Parser unless defined?(::JSON::Parser)
    self.parser = ::JSON::Ext::Parser
    self.generator = ::JSON::Ext::Generator

    autoload :GenericObject, 'oj/generic_object'

  end # JSON
end
