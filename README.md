# Mini 3D Scanner - Mark 2

A low-cost turntable 3D scanner: a distance sensor measures the object's radius
while a stepper rotates the turntable, and a second stepper raises the sensor
between layers. The captured data is converted into a point cloud and an STL
mesh.

## Repository layout

```
.
├── cad_files/                   Fusion 360 design files (Mark 2)
├── data/
│   └── farmer.txt               Sample scan: 78 layers x 200 readings
├── hardware/
│   └── mainScanner/             Arduino sketch (folder name must match .ino)
├── src/
│   └── calcGetData.py           Point cloud + STL reconstruction
├── requirements.txt
└── README.md
```

## How it works

1. The Arduino scans one layer by taking `READINGS_PER_REV` distance readings
   per revolution, then prints `9999.00` and lifts the sensor by
   `LAYER_HEIGHT_MM`.
2. The host records that serial output into `data/farmer.txt`.
3. `src/calcGetData.py` splits the file on the `9999.00` delimiters, converts
   each reading from a distance into a turntable radius, maps
   (radius, theta, z) to Cartesian coordinates, smooths the surface, closes it,
   and exports `output/farmer.stl`.

### Data format

```
7.30      <- distance in cm
7.23
...
9999.00   <- end of layer
7.23      <- next layer starts here
...
```

The sample file (`data/farmer.txt`) contains 78 layers of 200 readings
(15600 measurements). Values are raw distances in centimetres; negative
readings are treated as sensor errors.

## Running the reconstruction

```bash
python -m venv .venv
.venv\Scripts\activate        # Windows
pip install -r requirements.txt

python src/calcGetData.py
```

The script opens a 3D preview and writes `output/farmer.stl`.

### Tuning parameters

All at the top of `src/calcGetData.py`:

| Parameter | Default | Meaning |
| --- | --- | --- |
| `maxDistance` / `minDistance` | `20` / `0` | Valid radius window (cm) |
| `midThreshUpper` / `midThreshLower` | `±0.5` | Dead zone around the centre axis |
| `windowSize` | `3` | Moving-average filter width |
| `interpRes` | `1` | Row resampling step |
| `centerDistance` | `10.3` | Sensor-to-rotation-axis distance (cm) |
| `zDelta` | `0.1` | Layer height (cm) |

To process a different scan, edit `INPUT_FILE` in `src/calcGetData.py`.

## Hardware and firmware

Open `hardware/mainScanner/mainScanner.ino` in the Arduino IDE (keep the
`mainScanner` folder name), adjust the pin map and constants for your build,
then upload. The sketch streams measurements at 115200 baud.

Key constants shared with the Python script:

- `READINGS_PER_REV = 200`
- `LAYERS = 78`
- `LAYER_HEIGHT_MM = 1.0`

The sketch is a skeleton: the sensor code targets an HC-SR04, and the motion
code assumes two step/dir drivers. Replace those parts to match your hardware.

## License

No license has been chosen yet. All rights reserved by the author unless a
license is added.
