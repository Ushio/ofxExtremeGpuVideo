require 'shellwords'
require 'fileutils'

Dir.glob("*.mov") do |filename|
  name = File.basename(filename, ".*") + ".gvintermediate"
  FileUtils.mkdir(name) unless FileTest.exist?(name)
  system("ffmpeg -i #{filename} -an -vcodec png #{name}/%05d.png -r 30")
end

#
# name = File.basename(ARGV[0], ".*") + ".gvintermediate"
# FileUtils.mkdir(name) unless FileTest.exist?(name)
# # -r 30 必要ない？
#
# meta = `ffmpeg -i #{Shellwords.escape(ARGV[0])} 2>&1`
# File.write("#{name}/meta.txt", meta)
#
# system("ffmpeg -i #{Shellwords.escape(ARGV[0])} -an -vcodec png #{name}/%05d.png")
# system("ffmpeg -i #{Shellwords.escape(ARGV[0])} -ac 2 -ar 44100 -f wav #{name}/sound.wav")
