
module Oj
  
  def self.mimic_loaded()
    gems_dir = File.dirname(File.dirname(File.dirname(File.dirname(__FILE__))))
    Dir.foreach(gems_dir) do |gem|
      next unless gem.start_with?('json-')
      $LOADED_FEATURES << File.join(gems_dir, gem, 'lib', 'json.rb')
    end
  end
end
