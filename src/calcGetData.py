from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt
from scipy.ndimage import uniform_filter
import trimesh

BASE_DIR = Path(__file__).resolve().parent.parent
INPUT_FILE = BASE_DIR / "data" / "farmer.txt"
OUTPUT_FILE = BASE_DIR / "output" / "farmer.stl"

'''
Processing variables
'''

maxDistance = 20
minDistance = 0

midThreshUpper = 0.5
midThreshLower = -midThreshUpper

windowSize = 3
interpRes = 1

centerDistance = 10.3
zDelta = 0.1

'''
Load raw data
'''

rawData = np.loadtxt(INPUT_FILE)

'''
Remove erroneous scans
'''

rawData[rawData < 0] = 0

'''
Find delimiter (9999)
'''

indices = np.where(rawData == 9999)[0]

'''
Arrange data into matrix
'''

rows = []
start = 0

for idx in indices:
    rows.append(rawData[start:idx])
    start = idx + 1

'''
Make all rows equal length
'''

min_len = min(len(r) for r in rows)
rows = [r[:min_len] for r in rows]

r = np.array(rows)

'''
Convert distance to turntable coordinates
'''

r = centerDistance - r

r[r > maxDistance] = np.nan
r[r < minDistance] = np.nan

mask = (r > midThreshLower) & (r < midThreshUpper)
r[mask] = np.nan

'''
Generate theta matrix
'''

theta = np.linspace(360, 0, r.shape[1], endpoint=False)
theta = np.deg2rad(theta)

theta = np.tile(theta, (r.shape[0], 1))

'''
Generate z matrix
'''

z = np.arange(0, r.shape[0] * zDelta, zDelta)
z = np.tile(z.reshape(-1, 1), (1, r.shape[1]))

'''
Convert polar coordinates to Cartesian coordinates
'''

x = r * np.cos(theta)
y = r * np.sin(theta)

'''
Remove rows containing only NaN values
'''

valid_rows = ~np.all(np.isnan(x), axis=1)

x = x[valid_rows]
y = y[valid_rows]
z = z[valid_rows]

'''
Replace NaN values with the previous valid point
'''

for i in range(x.shape[0]):

    valid = np.where(~np.isnan(x[i]))[0]

    if len(valid) == 0:
        continue

    latest_x = x[i, valid[0]]
    latest_y = y[i, valid[0]]

    for j in range(x.shape[1]):

        if not np.isnan(x[i, j]):
            latest_x = x[i, j]
            latest_y = y[i, j]
        else:
            x[i, j] = latest_x
            y[i, j] = latest_y

'''
Resample data
'''

interpIdx = np.arange(0, x.shape[0], interpRes)

xInterp = x[interpIdx]
yInterp = y[interpIdx]
zInterp = z[interpIdx]

'''
Apply smoothing filter
'''

xInterp = np.pad(
    xInterp,
    ((0, 0), (windowSize, windowSize)),
    mode="symmetric"
)

yInterp = np.pad(
    yInterp,
    ((0, 0), (windowSize, windowSize)),
    mode="symmetric"
)

xInterp = uniform_filter(xInterp, size=windowSize)
yInterp = uniform_filter(yInterp, size=windowSize)

xInterp = xInterp[:, windowSize:-windowSize]
yInterp = yInterp[:, windowSize:-windowSize]

'''
Wrap first column to last column for a closed surface
'''

xInterp[:, -1] = xInterp[:, 0]
yInterp[:, -1] = yInterp[:, 0]
zInterp[:, -1] = zInterp[:, 0]

'''
Close the top surface
'''

xTop = np.mean(xInterp[-1])
yTop = np.mean(yInterp[-1])

xInterp = np.vstack([
    xInterp,
    np.full(xInterp.shape[1], xTop)
])

yInterp = np.vstack([
    yInterp,
    np.full(yInterp.shape[1], yTop)
])

zTop = zInterp[-1, 0] - zInterp[-2, 0] + zInterp[-1, 0]

zInterp = np.vstack([
    zInterp,
    np.full(zInterp.shape[1], zTop)
])

'''
Display point cloud
'''

fig = plt.figure(figsize=(8, 8))
ax = fig.add_subplot(111, projection="3d")

ax.scatter(
    xInterp,
    yInterp,
    zInterp,
    s=1,
    c="blue"
)

ax.set_xlabel("X")
ax.set_ylabel("Y")
ax.set_zlabel("Z")

ax.set_box_aspect([
    np.ptp(xInterp),
    np.ptp(yInterp),
    np.ptp(zInterp)
])

plt.tight_layout()
plt.show()

'''
Generate STL mesh
'''

vertices = np.column_stack([
    xInterp.flatten(),
    yInterp.flatten(),
    zInterp.flatten()
])

faces = []

rows, cols = xInterp.shape

for i in range(rows - 1):

    for j in range(cols - 1):

        p1 = i * cols + j
        p2 = p1 + 1
        p3 = p1 + cols
        p4 = p3 + 1

        faces.append([p1, p2, p3])
        faces.append([p2, p4, p3])

'''
Create mesh object
'''

mesh = trimesh.Trimesh(
    vertices=vertices,
    faces=np.array(faces),
    process=False
)

'''
Export STL file
'''

OUTPUT_FILE.parent.mkdir(parents=True, exist_ok=True)

mesh.export(OUTPUT_FILE)

print(f"STL exported successfully: {OUTPUT_FILE}")