sed -i "s|\(^[^# ]* \).*/xia-daq-gui-online\(/firmware/.*$\)|\1$(pwd)\2|g" parset/cfgPixie16.txt