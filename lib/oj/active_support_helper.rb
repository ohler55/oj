
require 'active_support/time'

module Oj

  # 
  class ActiveSupportHelper

    def self.createTimeWithZone(utc, zone)
      ActiveSupport::TimeWithZone.new(utc - utc.gmt_offset, ActiveSupport::TimeZone[zone])
    end
  end

end

Oj.register_odd(ActiveSupport::TimeWithZone, Oj::ActiveSupportHelper, :createTimeWithZone, :utc, 'time_zone.name')

