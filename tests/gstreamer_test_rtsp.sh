for i in $(seq 0 3)
do
gst-launch-1.0 -q rtspsrc location=rtsp://192.168.1.131:8554/cam${i}${1} latency=20 ! rtph265depay  ! h265parse ! queue ! avdec_h265 ! queue ! glimagesink 1> /dev/null &
done
