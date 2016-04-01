require 'shellwords'
require 'fileutils'

name = File.basename(ARGV[0], ".*")
FileUtils.mkdir(name) unless FileTest.exist?(name)
# -r 30 必要ない？

meta = `ffmpeg -i #{Shellwords.escape(ARGV[0])} 2>&1`
File.write("#{name}/meta.txt", meta)

system("ffmpeg -i #{Shellwords.escape(ARGV[0])} -an -vcodec png #{name}/%05d.png")
# system("ffmpeg -i #{Shellwords.escape(ARGV[0])} -ac 2 -ar 44100 -f wav #{name}/sound.wav")
