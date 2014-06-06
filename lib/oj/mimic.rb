
module Oj

  def self.mimic_loaded(mimic_paths=[])
    $LOAD_PATH.each do |d|
      next unless File.exist?(d)
      offset = d.size() + 1
      Dir.glob(File.join(d, '**', '*.rb')).each do |file|
        next if file[offset..-1] !~ /^json[\/\\\.]{1}/
        $LOADED_FEATURES << file unless $LOADED_FEATURES.include?(file)
      end
    end
    mimic_paths.each { |p| $LOADED_FEATURES << p }
    $LOADED_FEATURES << 'json' unless $LOADED_FEATURES.include?('json')
  end
end
