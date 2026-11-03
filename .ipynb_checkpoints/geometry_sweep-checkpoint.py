import subprocess
import platform, sys
from pathlib import Path
import argparse
import numpy as np
from scipy.stats import qmc
import matplotlib.pyplot as plt
from root_to_csv import cluster_lxe_csv
""" 
Geometry configurations in mm

(radius, half_height, num_events, fidelity)

Fidelity
0: LF ~ 5,000 run events (~2k clusters)
1: HF ~ 1,000,000 run events (~420k clusters)
"""
n_points_lf = 200
n_points_hf = 50
r_min, r_max = 100, 2000
z_min, z_max = 100, 2000
energycut = 0.05

fig, ax = plt.subplots()
all_points = []
for i, n_points in enumerate([n_points_lf, n_points_hf]):
    sampler = qmc.LatinHypercube(d=2, seed=42+i)
    sample = sampler.random(n=n_points)

    # define constants per run
    if i == 0:
        color, fidelity, n_events = 'blue', 0, 5e3
    else:
        color, fidelity, n_events = 'red', 1, 1e6
   
    # Convert [0, 1) samples to inclusive integer ranges
    r = r_min + np.floor(sample[:, 0] * (r_max - r_min + 1)).astype(int)
    z = z_min + np.floor(sample[:, 1] * (z_max - z_min + 1)).astype(int)    
    points = np.column_stack((r, z, 
                              np.full(n_points, n_events), 
                              np.full(n_points, fidelity)
                             ))
    all_points.append(points)
    ax.scatter(points[:, 0], points[:, 1], color=color, label=f"Fidelity {fidelity}")

# Finish building geometries and plot
geometries = np.vstack(all_points)
ax.set_xlabel("Radius (mm)")
ax.set_ylabel("Half-Height (mm)")
ax.legend()
fig.savefig("./points_plot.png")

# Build up input Arguments
parser = argparse.ArgumentParser(
    description="Starting script for running XLZD RESuM simulations"
)
parser.add_argument(
    "--dir",
    type=str,
    default=None,
    help="Main simulation directory. Includes builds, src, runtime, etc.",
)
parser.add_argument(
    "--output",
    type=str,
    default=None,
    help="Output file directory for simulation data."
)
args = parser.parse_args()

# Create main sim directory
if args.dir is None:
    sim_dir = Path(__file__).resolve().parent
else:
    sim_dir = Path(args.dir).resolve()

# Find machine and system and pick right build
system = platform.system()
machine = platform.machine()
if system == "Linux" and machine in ("x86_64", "AMD64"):
    build_dir = sim_dir / "builds" / "linux_x86_64"
elif system == "Darwin" and machine == "arm64":
    build_dir = sim_dir / "builds" / "macos_arm64"
else:
    raise RuntimeError(f"No prebuild executable for {system} {machine}")

# Find executable file and output and make them
executable =  build_dir / "XLZD_LXe"
if not executable.exists():
    raise FileNotFoundError(f"Prebuild executable not found for {system} {machine}: {executable}")
if args.output is None:
    output_dir = sim_dir / "simulation_data"
else:
    output_dir = Path(args.output).resolve()
output_dir.mkdir(parents=True, exist_ok=True)

for radius, halfheight, nevents, fidelity in geometries:
    radius = int(radius)
    halfheight = int(halfheight)
    nevents = int(nevents)
    fidelity = int(fidelity)
    
    print(f"\n=== Running R={radius}mm H={halfheight}mm events={nevents} fidelity={fidelity} ===")
    
    # Write run.mac
    mac_content = f"""
/run/initialize
/detector/radius {radius} mm
/detector/halfHeight {halfheight} mm
/detector/energyCut {energycut} keV
/generator/radius {radius} mm
/generator/halfHeight {halfheight} mm
/generator/mode lines
/generator/spectrumFile ../line_tables/Bi214_lines.txt
/run/reinitializeGeometry
/run/beamOn {nevents}
"""
    # Setup runtime and root files/output path
    runtime = sim_dir / "runtime"
    mac_path = runtime / "run_sweep.mac"
    mac_path.write_text(mac_content)
    root_file = runtime / "LXe_output.root"
    if root_file.exists(): root_file.unlink()
    output_file = output_dir / f"R{radius}_H{halfheight}_F{fidelity}.csv"
    
    # Run simulation
    subprocess.run([str(executable), "run_sweep.mac"], cwd=runtime, check=True)

    # Convert to CSV
    cluster_lxe_csv(root_file, output_file, write_meta=False)
    
    print(f"Done: {output_file.name} saved to {output_dir}")

print("\n=== Sweep complete ===")
