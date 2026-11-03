# frozen_string_literal: true

# A plain Ruby object, as object mode dumps it with its instance variables
# and its class.
class Account
  attr_reader :id, :owner, :balance, :opened, :tags, :limits

  def initialize(i)
    @id = i
    @owner = "Owner #{i}"
    @balance = (i * 10.5) + 0.25
    @opened = Time.utc(2026, 3, 1, 9, 0, i % 60)
    @tags = ['checking', "branch-#{i % 7}"]
    @limits = {'daily' => 500, 'monthly' => 10_000}
  end
end
