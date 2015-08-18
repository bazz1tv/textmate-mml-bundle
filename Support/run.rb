#!/usr/bin/env ruby

require ENV['TM_BUNDLE_SUPPORT'] + "/build.rb"
cmd = "open '#{@nsf_path}'"
doCMD(cmd)