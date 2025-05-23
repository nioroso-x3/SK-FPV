for i in $(seq 0 3)
do
gst-launch-1.0 -q rtspsrc location=rtsp://192.168.250.30:8554/cam${i}${1} latency=10 ! rtph265depay  ! h265parse ! queue ! nvh265dec ! queue ! videoconvert ! autovideosink 1> /dev/null &
done
