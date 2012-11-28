
module Oj
  
  def self.mimic_loaded(mimic_paths=[])
    gems_dir = File.dirname(File.dirname(File.dirname(File.dirname(__FILE__))))
    Dir.foreach(gems_dir) do |gem|
      next unless (gem.start_with?('json-') || gem.start_with?('json_pure'))
      $LOADED_FEATURES << File.join(gems_dir, gem, 'lib', 'json.rb')
    end
    mimic_paths.each { |p| $LOADED_FEATURES << p }
  end
end
