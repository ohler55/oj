# frozen_string_literal: true

ORIG_ARGV = ARGV.dup

require "bundler/setup"
require "active_support/core_ext/kernel/reporting"

silence_warnings do
  Encoding.default_internal = Encoding::UTF_8
  Encoding.default_external = Encoding::UTF_8
end

require "active_support/testing/autorun"
require "active_support/testing/method_call_assertions"
require "active_support/testing/error_reporter_assertions"

ENV["NO_RELOAD"] = "1"
require "active_support"

Thread.abort_on_exception = true

# Show backtraces for deprecated behavior for quicker cleanup.
ActiveSupport.deprecator.behavior = :raise

ActiveSupport::Cache.format_version = 7.1

# Disable available locale checks to avoid warnings running the test suite.
I18n.enforce_available_locales = false

class ActiveSupport::TestCase
  if Process.respond_to?(:fork) && !Gem.win_platform?
    parallelize
  else
    # These tests flip ActiveSupport globals as they run
    # (use_standard_json_time_format, escape_html_entities_in_json,
    # JSON::Encoding.time_precision), so forked workers are fine but threaded
    # ones race on them. Upstream never hits this because it always forks.
    # workers: 1 keeps the suite serial; it takes well under a second anyway.
    parallelize(workers: 1)
  end

  include ActiveSupport::Testing::MethodCallAssertions
  include ActiveSupport::Testing::ErrorReporterAssertions
end
