#!/bin/sh

./numb \
    --userid 1001 \
    --originserverurl http://10.0.0.1,http://10.0.0.2,http://10.0.0.3,http://10.0.0.4 \
    --cachedir /usr/space/tmp \
    --keytimeout 60 \
    --adminserver \
    --sourcemulticastip 10.0.0.5 \
    --listeningport 81 \
    --nokeycheck \
    --workerthreads 2
