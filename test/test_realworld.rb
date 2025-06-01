#!/usr/bin/env ruby
# frozen_string_literal: true

$LOAD_PATH << __dir__

require 'helper'

class RealWorldTest < Minitest::Test

  def test_realworld_data
    files = ['activitypub.json', 'canada.json', 'citm_catalog.json', 'ohai.json', 'twitter.json']

    files.each do |file|
      data = Oj.load(File.read("test/data/#{file}"))
      basename = File.basename(file, '.json')

      expected = File.read("test/data/#{basename}.compat.txt")
      json = Oj.dump(data, mode: :compat)
      assert_equal(expected, json)

      expected = File.read("test/data/#{basename}.rails.txt")
      json = Oj.dump(data, mode: :rails)
      assert_equal(expected, json)
    end
  end
end