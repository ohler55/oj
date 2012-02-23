
require 'etc'

module Sample

  class File
    attr_accessor :name, :ctime, :mtime, :size, :owner, :group, :permissions

    def initialize(filename)
      @name = ::File.basename(filename)
      stat = ::File.stat(filename)
      @ctime = stat.ctime
      @mtime = stat.mtime
      @size = stat.size
      @owner = Etc.getpwuid(stat.uid).name
      @group = Etc.getgrgid(stat.gid).name
      @permissions = {
        :user => [(0 != (stat.mode & 0x0100)) ? 'r' : '-',
                  (0 != (stat.mode & 0x0080)) ? 'w' : '-',
                  (0 != (stat.mode & 0x0040)) ? 'x' : '-'].join(''),
        :group => [(0 != (stat.mode & 0x0020)) ? 'r' : '-',
                   (0 != (stat.mode & 0x0010)) ? 'w' : '-',
                   (0 != (stat.mode & 0x0008)) ? 'x' : '-'].join(''),
        :other => [(0 != (stat.mode & 0x0004)) ? 'r' : '-',
                   (0 != (stat.mode & 0x0002)) ? 'w' : '-',
                   (0 != (stat.mode & 0x0001)) ? 'x' : '-'].join('')
      }
    end

  end # File
end # Sample
