# XLZD_LXe Simulation - Updated October 8th, 2026

This repository contains the Geant4 simulation used to generate training data for the XLZD RESuM surrogate-model pipeline.

The simulation models gamma transport in a liquid xenon cylinder across a range of detector geometries. It fires gammas from the surface of a vacuum skin surrounding the xenon volume and records every energy deposit inside the xenon. By default each event is one gamma whose energy is drawn from the Bi-214 line table; the original single 2447 keV gamma and a full radioactive-decay mode are selectable (see [Source modes](#source-modes)). Detector radius and half-height are set at runtime, so a single build produces data across the full geometry space. The resulting per-deposit CSVs are consumed downstream by the CNP–MFGP workflow.

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
- `generator.cc` — primary generator: positions sampled on the skin surface (side, top cap, bottom cap) weighted by area; energy and direction depend on the source mode (see [Source modes](#source-modes)). Radius and half-height are exposed as `/generator/` messenger commands.
- `options.cc` — the `/generator/mode`, `/generator/angular`, `/generator/isotope`, `/generator/spectrumFile`, `/generator/ionZ`, `/generator/ionA` and `/detector/correctDepositPosition` commands.
- `stacking.cc` — ion mode only: tallies the gammas emitted by the decay and removes betas, neutrinos, alphas, conversion electrons and ground-state daughter ions. Does nothing in the other modes.
- `event.cc` — writes the per-event tables (`LXeEvents`, `LXeGammas`) at the end of every event, including events with no deposit.
- `stepping.cc` — records one row per energy-deposit step inside the LXe volume. Deposits outside the xenon are not scored.
- `run.cc` — ROOT file management and ntuple column definitions (four ntuples, see [Output format](#output-format)).

`include/eventinfo.hh` is the small per-event record (E0, direction, weight, decay-gamma tally) shared by the generator, stacking, stepping and event actions.

### Python utilities

- `convert_to_hdf5.py` — converts the ROOT output to HDF5.
- `prepare_simulation_csv.py` — converts HDF5 to the CSV format the RESuM pipeline reads, keeping the eight pipeline columns and adding a `global_event_id`.
- `generate_manifest.py` — scans `simulation_data/` and writes `file_manifest.csv` with the geometry and fidelity of each file.
- `geometry_sweep.py` — runs a list of geometries end to end (simulate, convert, write CSV).
- `root_to_csv.py` — clusters the deposits of each event (5 mm) and writes the CSV the pipeline reads (called by `geometry_sweep.py`); see [Cluster CSV](#cluster-csv-root_to_csvpy).
- `make_line_table.py` — builds a gamma-line table for `/generator/spectrumFile` from an ion-mode run.

### `simulation_data/`

Local output CSVs and the generated manifest.

This data should not be committed to version control; the files are large.

Includes `file_manifest.csv`, which dictates the detector geometry, coordinate centering, and fidelity of each input file for the pipeline.

---

## Requirements

- Geant4 (developed with 11.4.1). The physics list is `G4EmStandardPhysics_option4` + `G4DecayPhysics` + `G4RadioactiveDecayPhysics`. The radioactive-decay datasets (`G4RADIOACTIVEDATA`, `G4LEVELGAMMADATA`, `G4ENSDFSTATEDATA`) must be installed and their environment variables set (`geant4.sh` does this), because the physics list loads them at initialisation in every mode, including `mono`.
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

`CMakeLists.txt` collects the sources with a glob when `cmake` runs, so after adding or removing a `.cc` file (this version adds `options.cc`, `stacking.cc` and `event.cc`) re-run `cmake ..` in a clean build directory rather than just `make`. A binary built from the previous version has none of the new commands and must be rebuilt; copy the new `XLZD_LXe` to wherever `geometry_sweep.py` looks for it.

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

## Source modes

`/generator/mode` selects the primary particle. It can be set in the macro after `/run/initialize` and before `/run/beamOn`, and can be changed between runs.

| mode | what is fired | `E0` in the output |
| --- | --- | --- |
| `lines` (default) | one gamma per event, energy drawn from a gamma-line table | the drawn line (MeV) |
| `mono` | one 2.447 MeV gamma per event (the original source) | 2.447 |
| `ion` | a radioactive ion at rest on the skin; Geant4 decays it | 0 (decay gammas are in `LXeGammas`) |

Other commands:

```text
/generator/isotope Bi214            built-in line table (lines) or ion (ion); Bi214 or Tl208, ion mode also Pb214, Co60, K40
/generator/spectrumFile table.txt   lines: read the table from a file instead of the built-in one ("none" goes back)
/generator/ionZ 83                  ion: explicit Z and A override /generator/isotope
/generator/ionA 214
/generator/angular legacy           legacy = isotropic over 4 pi from every skin point (default, original behaviour)
/generator/angular cosine           cosine law about the inward normal (what an isotropic external flux produces)
/detector/correctDepositPosition false    true = post-step point for gammas, step midpoint for charged tracks (default: pre-step point, original behaviour)
```

**Reproducing the previous simulation.** `/generator/mode mono` with the default angular law and deposit position gives the same primaries as the previous version (checked event by event against the old generator with the same random stream).

**Lines mode.** A line table is a text file with one row per line, `E_MeV  intensity  [sampling_weight]`, `#` starts a comment. The generator draws a line with probability proportional to the sampling weight (default: the intensity) and stores `weight = (intensity share) / (sampling share)` for the event, so oversampling a rare line (for example 2447 keV) is corrected by the weight column downstream. With no sampling-weight column every weight is exactly 1. The built-in Bi-214 and Tl-208 tables are provisional values entered by hand; for production, generate the table from Geant4's own decay data:

```text
/generator/mode ion
/generator/isotope Bi214
/run/beamOn 200000
```

```bash
python3 make_line_table.py runtime/LXe_output.root -o Bi214_lines.txt
python3 make_line_table.py runtime/LXe_output.root -o Bi214_boosted.txt --boost-above 2.0 --boost-factor 20
```

then use `/generator/mode lines` with `/generator/spectrumFile Bi214_lines.txt`. A lines-mode event is a single gamma, so summing of cascade gammas (for example 609 + 1764 keV from one decay) is not produced; compare against an `ion` run to see how much it matters.

**Ion mode.** The ion is placed at rest at the skin point and decayed by `G4RadioactiveDecay`. The stacking action keeps the decay gammas and excited daughter nuclei and kills everything else created by the decay (betas, neutrinos, alphas, conversion electrons, ground-state daughters such as Po-214), so only gammas from the nuclear de-excitation reach the xenon; beta bremsstrahlung and conversion-electron X-rays are therefore not simulated. Directions are produced by the decay, so the `dirX/Y/Z` columns are 0 and `E0` is 0 in this mode; use `LXeGammas` and `nGamma`/`eGammaSum` for the emitted gammas. Expect `globalTime` in ion mode to include the parent's decay time (minutes for Bi-214, i.e. about 1e12 ns), so a coincidence window must be applied relative to the first decay gamma, not to zero. The stacking action prints each creator-process name once at the start of a run; `RadioactiveDecay` must appear there. Also check the default of `/process/had/rdm/thresholdForVeryLongDecayTime` in your Geant4 version before relying on ion mode for production.

Random seeds are not set by the program; add `/random/setSeeds a b` to the macro if runs must be reproducible or must not share a stream.

---

## Running a sweep

`geometry_sweep.py` holds a list of `(radius, halfheight, n_events, fidelity)` tuples and runs each one through the full chain. Edit the `geometries` list, then:

```bash
python3 geometry_sweep.py
```

It writes each macro, runs the simulation, converts to HDF5, and writes the CSV to `simulation_data/`. The output file name always ends in `_2447keVgamma.csv`, whatever the source mode; it is only a label (the manifest reads R and H from it). The `build_dir`, `sim_dir`, and `output_dir` paths at the top of the script point at the development machine and need adjusting for another environment.

---

## Output format

The ROOT file `LXe_output.root` holds four tables.

**`LXeDeposits`** — one row per energy-deposit step inside the LXe volume, 19 columns (18 as before plus `eventID`).

Pipeline columns (read by RESuM):

```text
sx, sy, sz    source position on the skin (mm), from the primary vertex
E0            primary energy in MeV (the drawn line in lines mode, 2.447 in mono mode, 0 in ion mode)
x, y, z       deposit position (mm)
ETPC          energy deposited at this step (MeV)
```

Additional columns (kept for later use):

```text
px, py, pz    momentum direction unit vector
stepNumber, stepLength (mm), globalTime (ns)
processName   process for this step (compt, eIoni, phot, ...)
trackID, parentID, particleType
eventID       Geant4 event number (new); identifies the event even when two events share a source position
```

**`LXeEvents`** — one row per simulated event, including events that left no deposit in the xenon: `eventID, sx, sy, sz, dirX, dirY, dirZ, E0, weight, nGamma, eGammaSum, eLXe`. `weight` is the importance weight (1 unless a line table has a sampling-weight column), `nGamma` and `eGammaSum` are the decay gammas emitted and their summed energy (ion mode), and `eLXe` is the total energy deposited in the xenon before the `/detector/energyCut`.

**`LXeGammas`** — one row per gamma emitted by radioactive decay (ion mode only): `eventID, E (MeV), time (ns)`. Input for `make_line_table.py`.

**`LXeRun`** — a single row written at the end of the run: `nEvents, radius_mm, halfHeight_mm, energyCut_keV, mode, angular, isotope, spectrumFile, depositPositionCorrected`. The radius and half-height are the detector's; the generator's values must be set to the same numbers (the sweep does this).

`prepare_simulation_csv.py` keeps only the eight pipeline columns and prepends a sequential `global_event_id`.

### Cluster CSV (`root_to_csv.py`)

`cluster_lxe_csv(root_file, csv_file)` merges the deposits of each event that lie within 5 mm of each other into one cluster and writes one row per cluster. The first 11 columns are as before (`clusterID, eventID, sx, sy, sz, x, y, z, ETPC, nDeposits, nTracks`); this version appends `E0, dirX, dirY, dirZ, weight, nGamma, eGammaSum` from `LXeEvents`. Events are now identified by the Geant4 `eventID`; ROOT files from the previous simulation have no such column and are still read, with events identified by source position as before and with `E0` from the deposits, NaN directions, weight 1 and zero gammas. The output `eventID` counts the events that have deposits, 0 to nEvents-1.

A sidecar `<csv name>.meta.json` is written next to the CSV with the number of emitted events (including those with no deposit, which are not in the CSV and are needed to normalise any efficiency), the summed weights, and the `LXeRun` settings. It is not a `.csv`, so `generate_manifest.py` ignores it. Pass `write_meta=False` to skip it. Downstream readers must tolerate the extra CSV columns.

Two properties of the data are worth knowing:

**Coordinates are centered on 0.** The LXe volume sits at the origin, so `z` runs from −halfHeight to +halfHeight. The RESuM preprocessing shifts this to a bottom-origin frame using the `z_center` value in the manifest; for this simulation's output `z_center = 0`.

**Rows are per deposit, not per event.** A single gamma Compton-scatters several times before leaving the xenon, so it produces several rows sharing the same source position. Most deposits carry a small fraction of the primary energy, because full absorption in a detector this size is rare. This is expected for the positional analysis, which uses deposit locations rather than per-event summed energy.

---

## Manifest

`generate_manifest.py` scans `simulation_data/` and writes `file_manifest.csv` with columns `filename, R, Z, z_center, fidelity`. R and Z are parsed from each filename, `z_center` is 0, and fidelity is 0 (LF) unless the geometry stem is listed in the `HF_GEOMETRIES` set at the top of the script. Add new HF geometries to that set as they are produced.
