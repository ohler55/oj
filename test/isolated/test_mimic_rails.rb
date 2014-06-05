# encoding: UTF-8

require 'helper'
require 'isolated/shared_mimic'

require 'rails/all'
Oj.mimic_JSON

class MimicRails < SharedMimicTest

  def test_mimic_activesupport
    begin
      ActiveSupport::JSON.decode("{")
      puts "Failed"
    rescue ActiveSupport::JSON.parse_error
      assert(true)
    rescue Exception
      assert(false, 'Expected a JSON::ParserError')
    end
  end

end # MimicRails
