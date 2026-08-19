import h5py
import numpy as np
import argparse
from pathlib import Path

def convert_h5_to_h5(h5_path: str, radius_mm: float, halfheight_mm: float, fidelity: int, output_dir: str = "."):
    """Convert raw simulation HDF5 to the model-ready HDF5 format expected by the XLZD pipeline.

    Same columns and global_event_id as the CSV version, but saved as compressed HDF5
    instead of CSV for smaller files and faster IO.
    """

    # Read the raw simulation h5 (flat datasets written by convert_to_hdf5.py)
    with h5py.File(h5_path, 'r') as f:
        cols = {
            'E0':   f['E0'][:],
            'sx':   f['sx'][:],
            'sy':   f['sy'][:],
            'sz':   f['sz'][:],
            'ETPC': f['ETPC'][:],
            'x':    f['x'][:],
            'y':    f['y'][:],
            'z':    f['z'][:],
        }

    # Shift z and sz from centered (-H..+H) to bottom-origin (0..2H) to match convention.
    cols['z']  = cols['z']  + halfheight_mm
    cols['sz'] = cols['sz'] + halfheight_mm

    n = len(cols['E0'])
    global_event_id = np.arange(n, dtype=np.int64)

    # Generate filename: R{radius}_H{halfheight}_F{fidelity}_2447keVgamma.h5
    component = f"R{int(radius_mm)}_H{int(halfheight_mm)}_F{int(fidelity)}"
    filename = f"{component}_2447keVgamma.h5"
    output_path = Path(output_dir) / filename
    output_path.parent.mkdir(parents=True, exist_ok=True)

    # Write compressed h5. Column order matches the CSV version:
    # global_event_id, E0, sx, sy, sz, ETPC, x, y, z
    with h5py.File(output_path, 'w') as out:
        out.create_dataset('global_event_id', data=global_event_id, compression='gzip')
        for name in ['E0', 'sx', 'sy', 'sz', 'ETPC', 'x', 'y', 'z']:
            out.create_dataset(name, data=cols[name], compression='gzip')

    print(f"Saved {n} deposits to {output_path}")
    return output_path

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("h5_file", help="Input raw HDF5 file from simulation")
    parser.add_argument("--radius", type=float, required=True, help="LXe cylinder radius in mm")
    parser.add_argument("--halfheight", type=float, required=True, help="LXe cylinder half-height in mm")
    parser.add_argument("--fidelity", type=float, required=True, help="Fidelity of file")
    parser.add_argument("--output-dir", default=".", help="Output directory for HDF5")
    args = parser.parse_args()

    convert_h5_to_h5(args.h5_file, args.radius, args.halfheight, args.fidelity, args.output_dir)
