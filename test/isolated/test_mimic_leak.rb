#!/usr/bin/env ruby
# frozen_string_literal: true

$LOAD_PATH << File.join(__dir__, '..')

require 'helper'

Oj.mimic_JSON

class MimicLeak < Minitest::Test
  class RaisesInToJson
    def to_json(*)
      raise ArgumentError.new('raised in to_json')
    end
  end

  class RaisesInWrite
    def write(*)
      raise IOError.new('raised in write')
    end
  end

  def test_generate_raising_in_to_json
    assert_raises(ArgumentError) { JSON.generate(['x' * 5000, RaisesInToJson.new]) }
  end

  def test_to_json_raising_in_to_json
    assert_raises(ArgumentError) { ['x' * 5000, RaisesInToJson.new].to_json }
  end

  def test_dump_raising_in_to_json
    assert_raises(ArgumentError) { JSON.dump(['x' * 5000, RaisesInToJson.new]) }
  end

  def test_dump_raising_in_write
    assert_raises(IOError) { JSON.dump(['x' * 5000], RaisesInWrite.new) }
  end
end
