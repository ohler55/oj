module Oj
  # A Simple Callback Parser (SCP) for JSON. The Oj::ScHandler class should be
  # subclassed and then used with the Oj.sc_parse() method. The Scp methods will
  # then be called as the file is parsed. The handler does not have to be a
  # subclass of the ScHandler class as long as it responds to the desired
  # methods.
  #
  # @example
  # 
  #  require 'oj'
  #
  #  class MyHandler < ::Oj::ScHandler
  #    def initialize()
  #      @hash_cnt = 0
  #    end
  #
  #    def start_hash()
  #      @hash_cnt += 1
  #    end
  #  end
  #
  #  cnt = MyHandler.new()
  #  File.open('any.json', 'r') do |f|
  #    Oj.sc_parse(cnt, f)
  #  end
  #
  # To make the desired methods active while parsing the desired method should
  # be made public in the subclasses. If the methods remain private they will
  # not be called during parsing.
  #
  #    def hash_start(); end
  #    def hash_end(); end
  #    def array_start(); end
  #    def array_end(); end
  #    def add_value(value); end
  #    def error(message, line, column); end
  #
  class ScHandler
    # Create a new instance of the ScHandler class.
    def initialize()
    end

    # To make the desired methods active while parsing the desired method should
    # be made public in the subclasses. If the methods remain private they will
    # not be called during parsing.
    private

    def hash_start()
    end

    def hash_end()
    end

    def array_start()
    end

    def array_end()
    end

    def add_value(value)
    end
    
    def hash_set(h, key, value)
    end
    
    def array_append(a, value)
    end
    
  end # ScHandler
end # Oj
