import h5py
import numpy as np
import pandas as pd
import argparse
from pathlib import Path

def convert_h5_to_csv(h5_path: str, radius_mm: float, halfheight_mm: float, fidelity: int, output_dir: str = "."):
    """Convert simulation HDF5 to CSV format expected by XLZD pipeline."""
    
    f = h5py.File(h5_path, 'r')
    
    # Read only EXPECTED_COLUMNS
    df = pd.DataFrame({
        'E0':   f['E0'][:],
        'sx':   f['sx'][:],
        'sy':   f['sy'][:],
        'sz':   f['sz'][:],
        'ETPC': f['ETPC'][:],
        'x':    f['x'][:],
        'y':    f['y'][:],
        'z':    f['z'][:],
    })
    f.close()

    # Shift z and sz from centered (-H..+H) to bottom-origin (0..2H) to match convention
    df['z']  = df['z']  + halfheight_mm
    df['sz'] = df['sz'] + halfheight_mm
    
    # Add global_event_id
    df.insert(0, 'global_event_id', np.arange(len(df), dtype=np.int64))
    
    # Generate filename: R{radius}_H{halfheight}_2447keVgamma.csv
    component = f"R{int(radius_mm)}_H{int(halfheight_mm)}_F{int(fidelity)}"
    filename = f"{component}_2447keVgamma.csv"
    output_path = Path(output_dir) / filename
    
    output_path.parent.mkdir(parents=True, exist_ok=True)
    df.to_csv(output_path, index=False)
    
    print(f"Saved {len(df)} deposits to {output_path}")
    return output_path

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("h5_file", help="Input HDF5 file from simulation")
    parser.add_argument("--radius", type=float, required=True, help="LXe cylinder radius in mm")
    parser.add_argument("--halfheight", type=float, required=True, help="LXe cylinder half-height in mm")
    parser.add_argument("--fidelity", type=float, required=True, help="Fidelity of file")
    parser.add_argument("--output-dir", default=".", help="Output directory for CSV")
    args = parser.parse_args()
    
    convert_h5_to_csv(args.h5_file, args.radius, args.halfheight, args.fidelity, args.output_dir)
