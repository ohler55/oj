
begin
  require 'ostruct'
rescue Exception
  # ignore
end

module Oj

  # A bit hack-ish but does the trick. The JSON.dump_default_options is a Hash
  # but in mimic we use a C struct to store defaults. This class creates a view
  # onto that struct.
  class MimicDumpOption < Hash
    def initialize()
      oo = Oj.default_options
      self.store(:max_nesting, false)
      self.store(:allow_nan, true)
      self.store(:quirks_mode, oo[:quirks_mode])
      self.store(:ascii_only, (:ascii == oo[:escape_mode]))
    end

    def []=(key, value)
      case key
      when :quirks_mode
        Oj.default_options = {:quirks_mode => value}
      when :ascii_only
        Oj.default_options = {:ascii_only => value}
      end
    end
  end

  def self.mimic_loaded(mimic_paths=[])
    $LOAD_PATH.each do |d|
      next unless File.exist?(d)

      jfile = File.join(d, 'json.rb')
      $LOADED_FEATURES << jfile unless $LOADED_FEATURES.include?(jfile) if File.exist?(jfile)
      
      Dir.glob(File.join(d, 'json', '**', '*.rb')).each do |file|
        $LOADED_FEATURES << file unless $LOADED_FEATURES.include?(file)
      end
    end
    mimic_paths.each { |p| $LOADED_FEATURES << p }
    $LOADED_FEATURES << 'json' unless $LOADED_FEATURES.include?('json')

    if Object.const_defined?('OpenStruct')
      OpenStruct.class_eval do
        # Both the JSON gem and Rails monkey patch as_json. Let them battle it out.
        unless defined?(self.as_json)
          def as_json(*)
            name = self.class.name.to_s
            raise JSON::JSONError, "Only named structs are supported!" if 0 == name.length
            { JSON.create_id => name, 't' => table }
          end
        end
        def self.json_create(h)
          new(h['t'])
        end
      end
    end

    Range.class_eval do
      # Both the JSON gem and Rails monkey patch as_json. Let them battle it out.
      unless defined?(self.as_json)
        def as_json(*)
          {JSON.create_id => 'Range', 'a' => [first, last, exclude_end?]}
        end
      end
      def self.json_create(h)
        new(h['a'])
      end
    end

    Rational.class_eval do
      # Both the JSON gem and Rails monkey patch as_json. Let them battle it out.
      unless defined?(self.as_json)
        def as_json(*)
          {JSON.create_id => 'Rational', 'n' => numerator, 'd' => denominator }
        end
      end
      def self.json_create(h)
        Rational(h['n'], h['d'])
      end
    end

    Regexp.class_eval do
      # Both the JSON gem and Rails monkey patch as_json. Let them battle it out.
      unless defined?(self.as_json)
        def as_json(*)
          {JSON.create_id => 'Regexp', 'o' => options, 's' => source }
        end
      end
      def self.json_create(h)
        new(h['s'], h['o'])
      end
    end

    Struct.class_eval do
      # Both the JSON gem and Rails monkey patch as_json. Let them battle it out.
      unless defined?(self.as_json)
        def as_json(*)
          name = self.class.name.to_s
          raise JSON::JSONError, "Only named structs are supported!" if 0 == name.length
          { JSON.create_id => name, 'v' => values }
        end
      end
      def self.json_create(h)
        new(h['v'])
      end
    end

    Symbol.class_eval do
      # Both the JSON gem and Rails monkey patch as_json. Let them battle it out.
      unless defined?(self.as_json)
        def as_json(*)
          {JSON.create_id => 'Symbol', 's' => to_s }
        end
      end
      def self.json_create(h)
        h['s'].to_sym
      end
    end

    Time.class_eval do
      # Both the JSON gem and Rails monkey patch as_json. Let them battle it out.
      unless defined?(self.as_json)
        def as_json(*)
          {JSON.create_id => 'Symbol', 's' => to_s }
          nsecs = [ tv_usec * 1000 ]
          nsecs << tv_nsec if respond_to?(:tv_nsec)
          nsecs = nsecs.max
          { JSON.create_id => 'Time', 's' => tv_sec, 'n' => nsecs }
        end
      end
      def self.json_create(h)
        if usec = h.delete('u')
          h['n'] = usec * 1000
        end
        if instance_methods.include?(:tv_nsec)
          at(h['s'], Rational(h['n'], 1000))
        else
          at(h['s'], h['n'] / 1000)
        end
      end
    end

    JSON.module_eval do
      def self.dump_default_options
        Oj::MimicDumpOption.new
      end

      def self.dump_default_options=(h)
        m = Oj::MimicDumpOption.new
        h.each do |k,v|
          m[k] = v
        end
      end
    end

  end # self.mimic_loaded

end # Oj
