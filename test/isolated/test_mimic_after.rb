# encoding: UTF-8

require 'json'
require 'helper'

class MimicAfter < Minitest::Test

  # dump
  def test_dump_string
    Oj.mimic_JSON
    Oj.default_options= {:indent => 2} # JSON this will not change anything
    json = JSON.dump([1, true, nil])
    assert_equal(%{[
  1,
  true,
  null
]
}, json)
  end

end # MimicAfter
