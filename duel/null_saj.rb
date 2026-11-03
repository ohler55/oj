# frozen_string_literal: true

# Callbacks for Oj::Parser.new(:saj) and Oj.saj_parse, which do nothing, so
# the parser's own work is what's measured.
class NullSaj
  def hash_start(key)
  end

  def hash_end(key)
  end

  def array_start(key)
  end

  def array_end(key)
  end

  def add_value(value, key)
  end
end
