#!/bin/bash

cd $SOFTWARELIB

git clone git@gitlab.com:libeigen/eigen.git
cd $SOFTWARELIB/eigen
git reset --hard ba4d7304e2e165a71187a5ff7b6799733e0025d4

cd $CROSSLAYER_FW