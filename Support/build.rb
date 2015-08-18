#!/usr/bin/env ruby

=begin

I am sure this is ugly. 
If you know better, 
don't judge, 
just fix it.

=end

require "open3"
require "#{ENV['TM_SUPPORT_PATH']}/lib/exit_codes"


# Utils
def doCMD(cmd)
  print cmd + "\n\n"

  stdin, stdout, stderr = Open3.popen3(cmd)

  while msg = stdout.gets
    print msg
  end

  while err = stderr.gets
    print err
  end
end


# Clean-up method
def full_cleanup
  cmd = "rm -f '#{@file_path}/define.inc'"
  doCMD(cmd)
  
  cmd = "rm -f '#{@file_path}/effect.h'"
  doCMD(cmd)
  
  cmd = "rm -f '#{@nes_path}/ppmck.sym'"
  doCMD(cmd)
  
  if (defined? @build_all) then
    # When building multiple files ppmck names the .h file based on the first file passed to it, which is not always the current file we have open in textmate
    cmd = "rm -f '#{@file_path}/#{@files.first.sub(/mml$/, 'h')}'"
  else
    cmd = "rm -f '#{@file_path}/#{@file_name.sub(/mml$/, 'h')}'"
  end
  doCMD(cmd)
end


# Some routes
@support_path = ENV['TM_BUNDLE_SUPPORT']
@nesasm_path  = @support_path + "/bin/nesasm"
@ppmckc_path  = @support_path + "/bin/ppmckc"
@nes_path     = @support_path + "/nes_include"

# required for nesasm to function
ENV['NES_INCLUDE'] = @nes_path

@file_path    = File.dirname(ENV['TM_FILEPATH'])
@file_name    = ENV['TM_FILENAME']
@nsf_path     = @file_path + "/" + @file_name.sub(/mml$/, "nsf")

# all files in directory
Dir.chdir(@file_path)
@files = Dir.glob("*.mml")

@files_list = "'#{@file_path}/#{@files.join("' '#{@file_path}/")}'"


# Multiple MML to ASM
if (defined? @build_all) then
  cmd = "'#{@ppmckc_path}' -i -u #{@files_list}"
# MML to ASM
else
  cmd = "'#{@ppmckc_path}' -i '#{@file_path}/#{@file_name}'"
end
doCMD(cmd)


if (File.exist? "#{@file_path}/effect.h")
  # ASM to NES
  cmd = "'#{@nesasm_path}' -s -raw '#{@nes_path}/ppmck.asm'"
  doCMD(cmd)
  
  
  if (File.exist? "#{@nes_path}/ppmck.nes") then
    # NES to NSF
    cmd = "mv '#{@nes_path}/ppmck.nes' '#{@nsf_path}'"
    doCMD(cmd)
    
    full_cleanup()
  else
    @fail_msg = "I AM ERROR. Something went wrong with nesasm."
    print "\n\n #{@fail_msg} \n\n"
    
    full_cleanup()
    TextMate.exit_show_tool_tip
  end

else
  @fail_msg = "Something went wrong with ppmck."
  print "\n\n #{@fail_msg} \n\n"
  
  cmd = "rm -f '#{@file_path}/define.inc'"
  doCMD(cmd)
  
  TextMate.exit_show_tool_tip
end