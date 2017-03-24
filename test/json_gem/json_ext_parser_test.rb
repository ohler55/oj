#frozen_string_literal: false
require 'json_gem/test_helper'

class JSONExtParserTest < Test::Unit::TestCase
  include Test::Unit::TestCaseOmissionSupport

  if defined?(JSON::Ext::Parser)
    def test_allocate
      omit_if(MIMIC_JSON, "mimic_JSON")
      parser = JSON::Ext::Parser.new("{}")
      assert_raise(TypeError, '[ruby-core:35079]') do
        parser.__send__(:initialize, "{}")
      end
      parser = JSON::Ext::Parser.allocate
      assert_raise(TypeError, '[ruby-core:35079]') { parser.source }
    end
  end
end
