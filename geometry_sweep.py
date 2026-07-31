import subprocess
import os
from pathlib import Path

# Geometry configurations to sweep
# (radius_mm, halfheight_mm, n_events, fidelity)
# fidelity: 0=LF, 1=HF
geometries = [
    (100, 100, 10000, 0),
    (200, 200, 10000, 0),
    (300, 300, 10000, 0),
    (400, 400, 10000, 0),
    (500, 500, 10000, 0),
    (500, 500, 50000, 1),  # one HF run at largest size
]

build_dir = Path.home() / "Desktop/XLZD_LXe/build"
sim_dir = Path.home() / "Desktop/XLZD_LXe"
output_dir = Path.home() / "Desktop/XLZD_LXe/simulation_data"
output_dir.mkdir(exist_ok=True)

for radius, halfheight, nevents, fidelity in geometries:
    print(f"\n=== Running R={radius}mm H={halfheight}mm events={nevents} fidelity={fidelity} ===")
    
    # Write run.mac
    mac_content = f"""
/run/initialize
/detector/radius {radius} mm
/detector/halfHeight {halfheight} mm
/generator/radius {radius} mm
/generator/halfHeight {halfheight} mm
/run/beamOn {nevents}
"""
    mac_path = build_dir / "run_sweep.mac"
    mac_path.write_text(mac_content)
    
    # Remove old output
    root_file = build_dir / "LXe_output.root"
    h5_file = build_dir / "LXe_output.h5"
    if root_file.exists(): root_file.unlink()
    if h5_file.exists(): h5_file.unlink()
    
    # Run simulation
    subprocess.run(["./XLZD_LXe", "run_sweep.mac"], cwd=build_dir, check=True)
    
    # Convert to HDF5
    subprocess.run(["python3", "../convert_to_hdf5.py", "LXe_output.root"], cwd=build_dir, check=True)
    
    # Convert to CSV
    subprocess.run([
        "python3", "../prepare_simulation_csv.py", "LXe_output.h5",
        "--radius", str(radius),
        "--halfheight", str(halfheight),
        "--output-dir", str(output_dir)
    ], cwd=build_dir, check=True)
    
    print(f"Done: R{radius}_H{halfheight}_2447keVgamma.csv saved to {output_dir}")

print("\n=== Sweep complete ===")
