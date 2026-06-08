#!/bin/bash

sudo python3 -m venv --system-site-packages face_rec & PID=$!
wait $PID
source face_rec/bin/activate