$: << File.dirname(__FILE__)
$oj_dir = File.dirname(File.dirname(File.expand_path(File.dirname(__FILE__))))
%w(lib ext).each do |dir|
  $: << File.join($oj_dir, dir)
end

require 'test/unit'
MIMIC_JSON = !!ENV['MIMIC_JSON']

if ENV['MIMIC_JSON']
  require 'oj'
  Oj.mimic_JSON
end
require 'json'

NaN = JSON::NaN if defined?(JSON::NaN)
NaN = 0.0/0 unless defined?(NaN)
