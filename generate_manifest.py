import pandas as pd
from pathlib import Path

# Define the simulation runs with their metadata
# (filename_stem, radius_mm, halfheight_mm, fidelity)
# fidelity: 0=LF, 1=HF
runs = [
    ("R100_H100_2447keVgamma", 100, 100, 0, True),
    ("R200_H200_2447keVgamma", 200, 200, 0, True),
    ("R300_H300_2447keVgamma", 300, 300, 0, True),
    ("R400_H400_2447keVgamma", 400, 400, 0, True),
    ("R500_H500_2447keVgamma", 500, 500, 1, True),
]

rows = []
for stem, radius, halfheight, fidelity, realistic in runs:
    rows.append({
        "file_stem": stem,
        "radius_mm": radius,
        "halfheight_mm": halfheight,
        "fidelity": fidelity,
        "energy_keV": 2447,
        "source": "gamma",
        "realistic": realistic,
    })

df = pd.DataFrame(rows)
output_path = Path.home() / "Desktop/XLZD_LXe/simulation_data/file_manifest.csv"
df.to_csv(output_path, index=False)
print(f"Manifest saved to {output_path}")
print(df)
