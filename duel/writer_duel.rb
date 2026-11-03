# frozen_string_literal: true

require_relative 'duel_helper'

# Oj::StringWriter builds a document a value at a time, and Oj::Doc reads
# values out of one by path without building Ruby objects for the rest.

duel 'writer: push 100 records to a StringWriter' do
  once { @records = parsed(small_json)['attributes'] }
  writer = Oj::StringWriter.new(mode: :compat)
  writer.push_array
  100.times do |i|
    writer.push_object
    writer.push_value(i, 'id')
    @records.each { |key, value| writer.push_value(value, key) }
    writer.pop
  end
  writer.pop
  writer.to_s
end

duel 'doc: open twitter.json and fetch by path' do
  once { @json = json_file('twitter') }
  Oj::Doc.open(@json) do |doc|
    doc.fetch('/statuses/1/user/screen_name')
    doc.fetch('/statuses/50/retweet_count')
    doc.fetch('/search_metadata/max_id')
  end
end
