#! /bin/bash

# /etc/init.d/fbmuck
#
# Site init script for fbmuck.
#
# This script references the file /etc/fbmuck.conf.
# This script is started with the arguments:
#      start, stop. restart should be silently ignored.
#
#
# What this script is supposed to do:
#
# load /etc/fbmuck.conf.
# Compose a list of worlds authorised to run at system runlevel.
#    This will likely be a subset of (
#         /var/lib/games/fbmuck/*/
#         ${HOME}/fbmuck/*/
#    )
#
# Start:
# Now loop through each (dir) in the above list:
#      Check to see if dir/config exists.   (should)
#      Check to see if dir/clean_db exists. (should)
#      Check to see if dir/core exists. (should not)
#     If there is any problem with the above,
#        email the maintainer listed in dir/config,
#        or email the site administrator if no dir/config.
#     If all is well,
#        call 'start-muck' script on dir
#        email maintainer in dir/config regarding exit status
#              of 'start-muck' script.
#
