Put one file named ding.wav in this directory: a 16-bit PCM WAV, any sample
rate or channel count. It plays once on every real power-on (not on waking
the reader from its own sleep). Without this file, the firmware falls back
to a short synthesized beep instead of staying silent.

If you own the PaperColor's factory firmware, you can extract its original
"Dings2" boot chime from your own flash backup instead of supplying your own
clip. With esptool, first back up the full flash to a local file, then pull
the WAV bytes out of that backup:

  esptool.py --chip esp32s3 --port PORT read-flash 0 0x1000000 factory-backup.bin
  dd if=factory-backup.bin of=ding.wav bs=1 skip=$((16#352ec)) count=$((16#4fea6))
  file ding.wav   # should report RIFF (little-endian) data, WAVE audio

This repository does not ship ding.wav itself, since it comes from
M5Stack's proprietary factory firmware. Extract your own copy from firmware
you legitimately own.
