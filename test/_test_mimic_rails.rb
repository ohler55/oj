# encoding: UTF-8

require 'helper'
Oj.mimic_JSON
require 'rails/all'

class MimicRails < Minitest::Test

  def test_mimic_exception
    begin
      ActiveSupport::JSON.decode("{")
      puts "Failed"
    rescue ActiveSupport::JSON.parse_error
      assert(true)
    rescue Exception
      assert(false, 'Expected a JSON::ParserError')
    end
  end

  def test_dump_string
    Oj.default_options= {:indent => 2} # JSON this will not change anything
    json = ActiveSupport::JSON.encode([1, true, nil])
    assert_equal(%{[
  1,
  true,
  null
]
}, json)
  end

end # MimicRails
