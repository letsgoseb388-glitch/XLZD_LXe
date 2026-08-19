import pandas as pd
import re
from pathlib import Path

# Directory holding the simulation CSVs
data_dir = Path.home() / "Desktop/XLZD_LXe/simulation_data"

# Which geometries are high fidelity (1) vs low fidelity (0).
# Keyed by the R_H stem, e.g. "R500_H500". Anything not listed defaults to LF (0).
HF_GEOMETRIES = {
    "R500_H500",   # original HF run
    "R90_H108",    # first new HF point
}

# Pipeline expects z_center in the coordinate system of each file.
# The prep scripts shift z to bottom-origin (0..2H), so the center is at the
# half-height. z_center is set per-file to that value below.

# Match filenames like: R90_H108_2447keVgamma.csv  ->  R=90, H=108
pattern = re.compile(r"R(\d+)_H(\d+)_.*\.csv$")

rows = []
for csv_path in sorted(data_dir.glob("*.csv")):
    name = csv_path.name
    if name == "file_manifest.csv":
        continue
    m = pattern.match(name)
    if not m:
        print(f"Skipping (no R/H match): {name}")
        continue

    radius = int(m.group(1))
    halfheight = int(m.group(2))
    stem = f"R{radius}_H{halfheight}"
    fidelity = 1 if stem in HF_GEOMETRIES else 0

    rows.append({
        "filename": name,        # full filename incl. extension
        "R": radius,             # detector radius (mm)
        "Z": halfheight,         # detector centered-z extent / half-height (mm)
        "z_center": halfheight,  # data shifted to bottom-origin, center is at half-height
        "fidelity": fidelity,    # 0 = LF, 1 = HF
    })

if not rows:
    raise SystemExit(f"No matching CSVs found in {data_dir}")

df = pd.DataFrame(rows, columns=["filename", "R", "Z", "z_center", "fidelity"])
out_path = data_dir / "file_manifest.csv"
df.to_csv(out_path, index=False)

print(f"Wrote {len(df)} rows to {out_path}\n")
print(df.to_string(index=False))
