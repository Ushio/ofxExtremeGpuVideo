require "open-uri"
require 'fileutils'

url = "http://ushiobucket1.s3.amazonaws.com/GpuVideo/footage.gv"
filename = File.basename(url)

open(filename, 'wb') do |file|
  open(url) do |data|
      file.write(data.read)
    end
end

FileUtils.copy(filename, "./../converter-osx/bin/data/", {:verbose => true})
FileUtils.copy(filename, "./../converter-win/bin/data/", {:verbose => true})
FileUtils.copy(filename, "./../Unity/Example/Assets/StreamingAssets", {:verbose => true})
FileUtils.rm(filename, {:verbose => true})
