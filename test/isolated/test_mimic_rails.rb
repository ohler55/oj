# encoding: UTF-8

require 'helper'
require 'isolated/shared_mimic'

require 'rails/all'
Oj.mimic_JSON

class MimicRails < SharedMimicTest

  def test_activesupport_exception
    begin
      ActiveSupport::JSON.decode("{")
      puts "Failed"
    rescue ActiveSupport::JSON.parse_error
      assert(true)
    rescue Exception
      assert(false, 'Expected a JSON::ParserError')
    end
  end

  def test_activesupport_encode
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
