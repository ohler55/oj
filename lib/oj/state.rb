
module JSON

  # This class exists for json gem compatibility only. While it can be used as
  # the options for other than compatibility a simple Hash is recommended as
  # it is simpler and performs better. The only bit missing by not using a
  # state object is the depth availability which may be the depth during
  # dumping or maybe not since it can be set and the docs for depth= is the
  # same as max_nesting.
  class State < Oj::EasyHash

    def self.from_state(opts)
      s = self.new()
      s.clear()
      s.merge!(opts)
      s
    end

    def initialize(opts = {})
      # Populate with all vars then merge in opts. This class deviates from
      # the json gem in that any of the options can be set with the opts
      # argument. The json gem limits the opts use to 7 of the options.
      self[:indent] = ''
      self[:space] = ''
      self[:space_before] = ''
      self[:array_nl] = ''
      self[:object_nl] = ''
      self[:allow_nan] = false
      self[:buffer_initial_length] = 1024 # completely ignored by Oj
      self[:depth] = 0
      self[:max_nesting] = 100
      self[:check_circular?] = true
      self[:ascii_only] = false

      self.merge!(opts)
    end

    def allow_nan?()
      self[:allow_nan]
    end

    def ascii_only?()
      self[:ascii_only]
    end

    def configure(opts)
      raise TypeError.new('expected a Hash') unless opts.respond_to?(:to_h)
      self.merge!(opts.to_h)
    end

    def generate(obj)
      JSON.generate(obj)
    end

    def merge(opts)
      self.merge!(opts)
    end

    # special rule for this.
    def buffer_initial_length=(len)
      len = 1024 if 0 >= len
      self[:buffer_initial_length] = len
    end

  end # State

  module Ext
    module Generator
      class State < JSON::State
      end
    end # Generator
  end # Ext

end # JSON
