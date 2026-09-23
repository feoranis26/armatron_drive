# ARMATRON drive controller

This package runs on the motion-controller Raspberry Pi. It receives UDP wheel
commands from the x86 ROS bridge, drives the mecanum stepper hardware, and
publishes UDP telemetry.

The startup waveform sequence occurs before motor enable is asserted. Valid
UDP command loss for 500 ms explicitly zeros the target velocity and deasserts
the motor-enable line. Shutdown also deasserts that line.

`systemd/armatron-drive.service` is a source-controlled unit for this process.
It preserves the current `taskset -c 3` affinity and uses `Restart=no`. Review
the workspace path, run `sudo ./systemd/install.sh` to link it into
`/etc/systemd/system/`, and validate it with `systemd-analyze verify` before
enabling it.

For a normal refresh from the workspace root, run:

```text
./src/armatron_drive/systemd/refresh.sh
```

It builds as the invoking user, relinks the unit, reloads systemd, and restarts
only `armatron-drive.service`.
