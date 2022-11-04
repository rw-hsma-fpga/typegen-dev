#!/bin/bash

./t3t_ttf2pbm config.yaml $1 Saturn2AnycubicEco.yaml  && \
./t3t_pbm2stl config.yaml $1 Saturn2AnycubicEco.yaml

