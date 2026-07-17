import uproot
import numpy as np
import h5py
import sys

root_file = sys.argv[1] if len(sys.argv) > 1 else "LXe_output.root"
hdf5_file = root_file.replace(".root", ".h5")

print(f"Converting {root_file} to {hdf5_file}...")

f = uproot.open(root_file)
d = f['LXeDeposits']

with h5py.File(hdf5_file, 'w') as hf:
    for col in ['sx', 'sy', 'sz', 'E0', 'x', 'y', 'z', 'ETPC']:
        hf.create_dataset(col, data=d[col].array(library='np'))

print(f"Done! {len(d['sx'].array())} deposits saved to {hdf5_file}")
