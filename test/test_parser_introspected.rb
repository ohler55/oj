#!/usr/bin/env ruby
# encoding: utf-8

$: << File.dirname(__FILE__)

require 'helper'

class UsualTest < Minitest::Test
  def test_introspection_hash
    p = Oj::Parser.new(:introspected)
    parsed = p.parse('{"user": { "username": "meinac", "first_name": "Mehmet", "middle_name": "Emin", "last_name": "İNAÇ" } }')
    outer_object_introspection = parsed.dig(:__oj_introspection)
    inner_object_introspection = parsed.dig("user", :__oj_introspection)
    assert_equal({ start_byte: 0, end_byte: 104 }, outer_object_introspection)
    assert_equal({ start_byte: 9, end_byte: 102 }, inner_object_introspection)
  end
end
