#!/home/sofware/x86_64/bin/tclsh
#
################################################################################
# Description:
#           this script is used to validate client golden files. It ascertains
# that number of checked out licenses are within limits, i.e. a positive number
# less than $tot_license limit in license file.
#
# Usage:
#   Inputs:
#       1. out_file - this is output or ref file of client.<n> test
#       2. tot_license - read this from license file given to plmgrd in 
#          client.<n> test
#   Output:
#       1. Error message.
#       2. Exit statue.
#   Limitation:
#       1. It can only test 1 product license, e.g. either guna-client, 
#          guna-server not both.
#
################################################################################

set out_file    client.1.out
set tot_license 2 ; # get it from license file given to plmgrd

set fp [open $out_file r]
if { $fp == "" } {
    puts "ERROR: could not open file $out_file."
    exit
}

set exit_status 0
set checked_out 0
set line_no 0
while { [gets $fp line] >= 0 } {
    incr line_no
    if { [lindex $line 0] == "INFO:" && [lindex $line 1] == "request" } {
        set req [string index [lindex $line 2] 0]
        set grant [lindex $line 3]
    } else {
        continue
    }

    if { $grant != "granted." } {
        continue
    }

    if { $req == "0" } {
        incr checked_out
    } elseif {$req == "1"} {
        incr checked_out -1
    } else {
        puts "ERROR ($out_file:$line_no): malformed request \"$req\"."
    }

    if { [expr $checked_out > $tot_license] || [expr $checked_out < 0] } {
        puts "FAIL ($out_file:$line_no): number of checked out licenses $checked_out are not within range."
        set exit_status 1
    }
}

close $fp
exit $exit_status

