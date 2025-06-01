#!/usr/bin/env ruby
# frozen_string_literal: true

$LOAD_PATH << __dir__

require 'helper'

class RealWorldTest < Minitest::Test

  def test_realworld_data
    files = ['activitypub.json', 'canada.json', 'citm_catalog.json', 'ohai.json', 'twitter.json']

    # The files in `test/data` are generated from the original JSON files using the following snippet. 
    # Ensure a tested version of Oj is used to generate these files.

    # files.each do |f|
    #   data = Oj.load_file("#{__dir__}/test/data/#{f}")
    #   prefix = File.basename(f, '.json')
    #   File.write("#{__dir__}/test/data/#{prefix}.compat.json", Oj.dump(data, mode: :compat))
    #   File.write("#{__dir__}/test/data/#{prefix}.rails.json", Oj.dump(data, mode: :rails))
    # end

    files.each do |file|
      data = Oj.load(File.read("test/data/#{file}"))
      basename = File.basename(file, '.json')

      expected = File.read("test/data/#{basename}.compat.json")
      json = Oj.dump(data, mode: :compat)
      assert_equal(expected, json)

      expected = File.read("test/data/#{basename}.rails.json")
      json = Oj.dump(data, mode: :rails)
      assert_equal(expected, json)
    end
  end
end