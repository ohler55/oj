
module Oj

  def self.mimic_loaded(mimic_paths=[])
    $LOAD_PATH.each do |d|
      next unless File.exist?(d)

      jfile = File.join(d, 'json.rb')
      $LOADED_FEATURES << jfile unless $LOADED_FEATURES.include?(jfile) if File.exist?(jfile)
      
      Dir.glob(File.join(d, 'json', '**', '*.rb')).each do |file|
        $LOADED_FEATURES << file unless $LOADED_FEATURES.include?(file)
      end
    end
    mimic_paths.each { |p| $LOADED_FEATURES << p }
    $LOADED_FEATURES << 'json' unless $LOADED_FEATURES.include?('json')
  end
end
