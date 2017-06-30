require "open-uri"
require 'fileutils'

url = "http://ushiobucket1.s3.amazonaws.com/GpuVideo/Atoms-8579.gv"
filename = File.basename(url)

open(filename, 'wb') do |file|
  open(url) do |data|
      file.write(data.read)
    end
end

FileUtils.copy(filename, "./../example-player-osx/bin/data/", {:verbose => true})
FileUtils.copy(filename, "./../example-player-win/bin/data/", {:verbose => true})
FileUtils.copy(filename, "./../Unity/Example/Assets/StreamingAssets", {:verbose => true})
FileUtils.rm(filename, {:verbose => true})
