# frozen_string_literal: true

# A model as Rails dumps one: as_json returns its attributes in a Hash.
class Post
  def initialize(i)
    @attributes = {
      'id' => i,
      'title' => "Post number #{i}",
      'body' => 'Lorem ipsum dolor sit amet, <consectetur> adipiscing & elit. ' * 3,
      'published' => i.odd?,
      'rating' => (i % 5) + 0.5,
      'price' => BigDecimal("#{i}.99"),
      'created_at' => Time.utc(2026, 5, 1, 10, 0, i % 60, 250_000),
      'author' => {'id' => i % 9, 'name' => "Author #{i % 9}"},
    }
  end

  def as_json(_options=nil)
    @attributes
  end
end
