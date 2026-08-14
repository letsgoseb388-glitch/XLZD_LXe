# XLZD_LXe Simulation - Updated August 8th, 2026

This repository contains the Geant4 simulation used to generate training data for the XLZD RESuM surrogate-model pipeline.

The simulation models gamma transport in a liquid xenon cylinder across a range of detector geometries. It fires 2447 keV gammas isotropically from the surface of a vacuum skin surrounding the xenon volume and records every energy deposit inside the xenon. Detector radius and half-height are set at runtime, so a single build produces data across the full geometry space. The resulting per-deposit CSVs are consumed downstream by the CNP–MFGP workflow.

For the machine-learning pipeline that reads this simulation's output, see the [XLZD ML repository](https://github.com/apatnaik0/XLZD).

---

## Core idea

A liquid xenon cylinder sits inside a thin vacuum skin, both centered on the origin. Gammas are generated on the skin surface and travel inward; each energy deposit inside the xenon is recorded as a row of data.

For each detector geometry, the simulation produces:

```text
detector geometry (R, Z) + source position  ->  a set of energy-deposit positions inside the xenon
```

The downstream pipeline sorts these deposits into nested cylindrical shells and learns how their distribution changes with geometry.

At a high level:

```text
Macro sets geometry (R, Z) and event count
        │
        ▼
Geant4 simulation (gamma transport in LXe)
        │
        ▼
ROOT output (per-deposit ntuple)
        │
        ▼
HDF5 conversion
        │
        ▼
Pipeline CSV  ──►  RESuM CNP–MFGP workflow
```

---

## Repository layout

```text
.
├── src/
│   ├── construction.cc
│   ├── generator.cc
│   ├── stepping.cc
│   └── run.cc
├── convert_to_hdf5.py
├── prepare_simulation_csv.py
├── generate_manifest.py
├── geometry_sweep.py
├── simulation_data/
└── README.md
```

### `src/`

The Geant4 simulation source.

- `construction.cc` — geometry: a LXe cylinder inside a vacuum skin, both centered on the origin. Radius and half-height are exposed as `/detector/` messenger commands.
- `generator.cc` — primary generator: 2447 keV gammas, positions sampled on the skin surface (side, top cap, bottom cap) weighted by area, isotropic direction. Radius and half-height are exposed as `/generator/` messenger commands.
- `stepping.cc` — records one row per energy-deposit step inside the LXe volume. Deposits outside the xenon are not scored.
- `run.cc` — ROOT file management and ntuple column definitions.

### Python utilities

- `convert_to_hdf5.py` — converts the ROOT output to HDF5.
- `prepare_simulation_csv.py` — converts HDF5 to the CSV format the RESuM pipeline reads, keeping the eight pipeline columns and adding a `global_event_id`.
- `generate_manifest.py` — scans `simulation_data/` and writes `file_manifest.csv` with the geometry and fidelity of each file.
- `geometry_sweep.py` — runs a list of geometries end to end (simulate, convert, write CSV).

### `simulation_data/`

Local output CSVs and the generated manifest.

This data should not be committed to version control; the files are large.

Includes `file_manifest.csv`, which dictates the detector geometry, coordinate centering, and fidelity of each input file for the pipeline.

---

## Requirements

- Geant4 (developed with 11.4.1). The physics list is `G4EmStandardPhysics_option4` + `G4DecayPhysics`.
- CMake and a C++17 compiler.
- ROOT (used through Geant4's analysis manager).
- Python 3 with `uproot`, `h5py`, `numpy`, and `pandas`.

---

## Building

The compiled binary is architecture-specific. A binary built on an ARM Mac will not run on an x86_64 machine, so the code has to be built from source on whatever machine it will run on. Copying the `XLZD_LXe` binary between machines of different architectures will fail at execution.

Source Geant4's environment first, so that `geant4-config` is on the path:

```bash
source /path/to/geant4/bin/geant4.sh
```

Then build:

```bash
cd XLZD_LXe
mkdir build && cd build
cmake ..
make -j4
```

On the ARM Mac used for development, the working invocation needs a policy override and explicit expat paths, because CMake finds Homebrew's expat rather than the system one:

```bash
cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
      -DEXPAT_LIBRARY=/opt/homebrew/Cellar/expat/2.8.1/lib/libexpat.dylib \
      -DEXPAT_INCLUDE_DIR=/opt/homebrew/Cellar/expat/2.8.1/include \
      ..
make -j4
```

Those expat paths are specific to the Mac and will not exist on the server. On Linux/x86_64, plain `cmake ..` usually finds expat without extra flags; add the `-DEXPAT_LIBRARY=` and `-DEXPAT_INCLUDE_DIR=` flags pointing at the local install only if CMake reports it missing. The `CMAKE_POLICY_VERSION_MINIMUM` flag is only needed if the CMake version complains about an old minimum policy in the Geant4 config.

---

## Running a single geometry

The binary reads a macro file. Geometry is set through messenger commands. The detector and generator radii and half-heights are independent variables and must be set to the same values:

```text
/run/initialize
/detector/radius 300 mm
/detector/halfHeight 300 mm
/generator/radius 300 mm
/generator/halfHeight 300 mm
/run/beamOn 10000
```

Run the simulation and convert the output:

```bash
./XLZD_LXe run.mac
python3 ../convert_to_hdf5.py LXe_output.root
python3 ../prepare_simulation_csv.py LXe_output.h5 \
    --radius 300 --halfheight 300 --output-dir ../simulation_data
```

This writes `R300_H300_2447keVgamma.csv` to `simulation_data/`. The filename rounds the geometry to whole millimeters (`int(radius)`), so a geometry of 90.77 mm is written as `R90`.

Radius and half-height are independent, so R and Z can differ (e.g. `radius 300`, `halfHeight 100`). Event count is set with `/run/beamOn`; low-fidelity runs use 10,000 events and high-fidelity runs use 50,000.

---

## Running a sweep

`geometry_sweep.py` holds a list of `(radius, halfheight, n_events, fidelity)` tuples and runs each one through the full chain. Edit the `geometries` list, then:

```bash
python3 geometry_sweep.py
```

It writes each macro, runs the simulation, converts to HDF5, and writes the CSV to `simulation_data/`. The `build_dir`, `sim_dir`, and `output_dir` paths at the top of the script point at the development machine and need adjusting for another environment.

---

## Output format

Each row is one energy-deposit step inside the LXe volume, with 18 columns.

Pipeline columns (read by RESuM):

```text
sx, sy, sz    source position on the skin (mm), from the primary vertex
E0            primary energy, fixed at 2.447 MeV
x, y, z       deposit position (mm)
ETPC          energy deposited at this step (MeV)
```

Additional columns (kept for later use):

```text
px, py, pz    momentum direction unit vector
stepNumber, stepLength (mm), globalTime (ns)
processName   process for this step (compt, eIoni, phot, ...)
trackID, parentID, particleType
```

`prepare_simulation_csv.py` keeps only the eight pipeline columns and prepends a sequential `global_event_id`.

Two properties of the data are worth knowing:

**Coordinates are centered on 0.** The LXe volume sits at the origin, so `z` runs from −halfHeight to +halfHeight. The RESuM preprocessing shifts this to a bottom-origin frame using the `z_center` value in the manifest; for this simulation's output `z_center = 0`.

**Rows are per deposit, not per event.** A single gamma Compton-scatters several times before leaving the xenon, so it produces several rows sharing the same source position. Most deposits carry a small fraction of the 2447 keV, because full absorption in a detector this size is rare. This is expected for the positional analysis, which uses deposit locations rather than per-event summed energy.

---

## Manifest

`generate_manifest.py` scans `simulation_data/` and writes `file_manifest.csv` with columns `filename, R, Z, z_center, fidelity`. R and Z are parsed from each filename, `z_center` is 0, and fidelity is 0 (LF) unless the geometry stem is listed in the `HF_GEOMETRIES` set at the top of the script. Add new HF geometries to that set as they are produced.
