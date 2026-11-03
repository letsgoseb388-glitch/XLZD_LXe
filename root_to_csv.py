from pathlib import Path
from tempfile import TemporaryDirectory
import csv
import json
import sqlite3
import numpy as np
import uproot
from scipy.spatial import cKDTree

# Per-event primary information, written by the simulation to the LXeEvents tree (one row per emitted event)
EVENT_INFO_COLUMNS = ["E0", "dirX", "dirY", "dirZ", "weight", "nGamma", "eGammaSum"]
EVENT_TREE_NAME = "LXeEvents"
RUN_TREE_NAME = "LXeRun"

def _stage_lxe_chunk(db, batch):
    """Save a reading chunk with consistent event IDs.

    Events are keyed by the Geant4 event number when the deposits tree has an eventID branch. Files from the
    earlier simulation have no such branch; those events are keyed by the exact source position as before.
    """
    if any(not np.isfinite(values).all() for values in batch.values()):
        raise ValueError("Input contains nonfinite values")
    if (batch["ETPC"] <= 0).any():
        raise ValueError("Expected positive per-step ETPC")
    if (batch["trackID"] != np.floor(batch["trackID"])).any():
        raise ValueError("trackID must contain integers")

    has_e0 = "E0" in batch
    event_ids = np.empty(len(batch["ETPC"]), dtype=np.int64)

    if "eventID" in batch:
        g4_ids = batch["eventID"].astype(np.int64)
        unique_ids, first_rows, inverse = np.unique(g4_ids, return_index=True, return_inverse=True)
        db_ids = np.empty(len(unique_ids), dtype=np.int64)

        # insert in order of first appearance so the output event numbering follows the file
        for k in np.argsort(first_rows):
            gid, i = int(unique_ids[k]), int(first_rows[k])
            e0 = float(batch["E0"][i]) if has_e0 else None
            db.execute("INSERT OR IGNORE INTO events(g4_id,sx,sy,sz,E0) VALUES (?,?,?,?,?)",
                       (gid, float(batch["sx"][i]), float(batch["sy"][i]), float(batch["sz"][i]), e0))
            db_ids[k] = db.execute("SELECT id FROM events WHERE g4_id=?", (gid,)).fetchone()[0]

        event_ids[:] = db_ids[np.asarray(inverse).reshape(-1)]
    else:
        source_ids = {}
        sources = zip(batch["sx"], batch["sy"], batch["sz"])

        for i, source in enumerate(sources):
            key = tuple(float(value) for value in source)

            if key not in source_ids:
                e0 = float(batch["E0"][i]) if has_e0 else None
                db.execute("INSERT OR IGNORE INTO events(sx,sy,sz,E0) VALUES (?,?,?,?)", (*key, e0))
                result = db.execute("SELECT id FROM events WHERE sx=? AND sy=? AND sz=?", key)
                source_ids[key] = result.fetchone()[0]

            event_ids[i] = source_ids[key]

    rows = zip(event_ids, batch["x"], batch["y"], batch["z"], batch["ETPC"], batch["trackID"])
    db.executemany(
        "INSERT INTO deposits VALUES (?,?,?,?,?,?)",
        ((int(eid), float(x), float(y), float(z), float(energy), int(track))
        for eid, x, y, z, energy, track in rows))
    
    return len(event_ids)

def _stage_lxe_file(db, input_path, tree_name, chunk_size):
    """Stage deposits on disk, including interleaved events."""
    required = ["sx", "sy", "sz", "x", "y", "z", "ETPC", "trackID"]
    n_deposits = 0

    with uproot.open(input_path) as root:
        tree = root[tree_name]
        missing = set(required) - set(tree.keys())
        if missing:
            raise ValueError(f"Missing branches: {sorted(missing)}")
        # eventID and E0 are written by the current simulation; older files do not have them
        branches = required + [name for name in ("eventID", "E0") if name in tree.keys()]

        for batch in tree.iterate(branches, step_size=chunk_size, library="np"):
            with db:
                n_deposits += _stage_lxe_chunk(db, batch)

    if n_deposits == 0:
        raise ValueError("Input tree contains no deposits")

    return n_deposits

def _create_schema(db, keyed_by_event_id):
    """Create the staging tables. The events key is the Geant4 event number or, for old files, the source position."""
    key_column = "g4_id INTEGER UNIQUE" if keyed_by_event_id else "g4_id INTEGER"
    key_constraint = "" if keyed_by_event_id else ", UNIQUE(sx, sy, sz)"
    db.executescript(f"""
        PRAGMA cache_size = -16384;
        CREATE TABLE events (id INTEGER PRIMARY KEY, {key_column}, sx REAL, sy REAL, sz REAL,
                             E0 REAL, dirX REAL, dirY REAL, dirZ REAL,
                             weight REAL DEFAULT 1.0, nGamma INTEGER DEFAULT 0, eGammaSum REAL DEFAULT 0.0,
                             has_info INTEGER DEFAULT 0{key_constraint});
        CREATE TABLE deposits (event_id INTEGER, x REAL, y REAL, z REAL, energy REAL, track_id INTEGER);
        CREATE INDEX deposit_event ON deposits(event_id);
    """)

def _merge_event_info(db, input_path, chunk_size):
    """Copy E0, direction, weight and decay-gamma counts from the LXeEvents tree into the staged events.

    Returns (events emitted, summed weight over all emitted events) or None if the file has no LXeEvents tree.
    Every event that has deposits must have a row there.
    """
    needed = ["eventID", *EVENT_INFO_COLUMNS]
    n_emitted = 0
    sum_weight = 0.0

    with uproot.open(input_path) as root:
        if EVENT_TREE_NAME not in root:
            return None
        tree = root[EVENT_TREE_NAME]
        missing = set(needed) - set(tree.keys())
        if missing:
            raise ValueError(f"{EVENT_TREE_NAME} is missing branches: {sorted(missing)}")

        for batch in tree.iterate(needed, step_size=chunk_size, library="np"):
            n_emitted += len(batch["eventID"])
            sum_weight += float(np.sum(batch["weight"], dtype=np.float64))
            rows = zip(*(batch[name].tolist() for name in EVENT_INFO_COLUMNS), batch["eventID"].tolist())
            with db:
                db.executemany(
                    "UPDATE events SET E0=?,dirX=?,dirY=?,dirZ=?,weight=?,nGamma=?,eGammaSum=?,has_info=1 WHERE g4_id=?",
                    rows)

    unmatched = db.execute("SELECT COUNT(*) FROM events WHERE has_info=0").fetchone()[0]
    if unmatched:
        raise ValueError(f"{unmatched} events have deposits but no row in {EVENT_TREE_NAME}")
    return n_emitted, sum_weight

def _read_run_metadata(input_path):
    """Return the single-row LXeRun tree as a dict. Metadata is optional, so problems are reported, not raised."""
    try:
        with uproot.open(input_path) as root:
            if RUN_TREE_NAME not in root:
                return {}
            columns = root[RUN_TREE_NAME].arrays(library="np")
        meta = {}
        for name, values in columns.items():
            value = values.tolist()[0] if len(values) else None
            meta[name] = value.decode() if isinstance(value, bytes) else value
        return meta
    except Exception as error:
        return {"error": f"could not read {RUN_TREE_NAME}: {error}"}

def _find_root(parent, i):
    """Find a deposit's connected component."""
    while parent[i] != i:
        parent[i] = parent[parent[i]]
        i = parent[i]
    return i

def _spatial_labels(positions, distance):
    """Connect nearby deposits without storing every neighbor pair."""
    tree = cKDTree(positions)
    parent = np.arange(len(positions))
    size = np.ones(len(positions), dtype=np.int64)

    for i, position in enumerate(positions):
        for j in tree.query_ball_point(position, distance):
            if j <= i:
                continue
            a = _find_root(parent, i)
            b = _find_root(parent, j)
            if a == b:
                continue
            if size[a] < size[b]:
                a, b = b, a
            parent[b] = a
            size[a] += size[b]

    roots = np.fromiter(
        (_find_root(parent, i) for i in range(len(positions))),
        dtype=np.int64,
    )
    return np.unique(roots, return_inverse=True)[1]

def _cluster_lxe_event(db, event_id, distance):
    """Return summed energies, centroids, deposit counts, and track counts."""
    records = db.execute(
        "SELECT x,y,z,energy,track_id FROM deposits WHERE event_id=?",
        (event_id,),
    ).fetchall()

    positions = np.array([row[:3] for row in records], dtype=float)
    energy = np.array([row[3] for row in records], dtype=float)
    tracks = np.array([row[4] for row in records], dtype=np.int64)
    del records

    labels = _spatial_labels(positions, distance)
    sums = np.bincount(labels, weights=energy)
    counts = np.bincount(labels)

    centers = np.column_stack([
        np.bincount(labels, weights=energy * positions[:, axis]) / sums
        for axis in range(3)
    ])
    track_pairs = np.unique(np.column_stack([labels, tracks]), axis=0)
    track_counts = np.bincount(track_pairs[:, 0], minlength=len(sums))

    np.testing.assert_allclose(sums.sum(), energy.sum(), rtol=1e-12, atol=0)
    return sums, centers, counts, track_counts

def _write_lxe_clusters(db, outpath, distance):
    """Write each completed event directly to the output CSV."""
    fields = ["clusterID", "eventID", "sx", "sy", "sz", "x", "y", "z", "ETPC", "nDeposits", "nTracks"]
    fields += EVENT_INFO_COLUMNS    # appended after the original columns, so existing readers see the same first 11
    n_events = 0
    n_clusters = 0
    events = db.execute("SELECT id,sx,sy,sz,E0,dirX,dirY,dirZ,weight,nGamma,eGammaSum FROM events ORDER BY id")

    def _value(v):
        return float("nan") if v is None else v   # SQLite stores NaN as NULL

    with outpath.open("w", newline="") as output:
        writer = csv.writer(output)
        writer.writerow(fields)
        for event_id, sx, sy, sz, e0, dir_x, dir_y, dir_z, weight, n_gamma, e_gamma_sum in events:
            sums, centers, counts, track_counts = _cluster_lxe_event(db, event_id, distance)
            info = [_value(e0), _value(dir_x), _value(dir_y), _value(dir_z), weight, n_gamma, e_gamma_sum]
            writer.writerows(
                [n_clusters + k, n_events, sx, sy, sz, *centers[k], total, int(counts[k]), int(track_counts[k]), *info]
                for k, total in enumerate(sums))
            n_clusters += len(sums)
            n_events += 1

    return n_events, n_clusters

def cluster_lxe_csv(input_path, outpath, distance=5.0, chunk_size=50000,
                    tree_name="LXeDeposits", write_meta=True):
    """Cluster complete events using temporary disk storage and save a CSV.

    Events are identified by the Geant4 event number (eventID branch) when the file has one, otherwise by the
    exact source position (files from the earlier simulation). Different tracks may merge.
    distance uses input coordinate units; connected chains can extend farther.
    Temporary files stay beside the input and are removed afterward.
    Memory scales with one reading chunk or one event. Output is overwritten.

    The first 11 CSV columns are unchanged; E0, dirX, dirY, dirZ, weight, nGamma and eGammaSum are appended
    (from the LXeEvents tree; old files get E0 from the deposits, NaN directions, weight 1 and zero gammas).
    eventID counts the events that have deposits, 0..nEvents-1.
    With write_meta a <outpath stem>.meta.json is written beside the CSV with the number of emitted events
    (needed to normalise efficiencies), the summed weights and the LXeRun settings.
    """
    input_path = Path(input_path).expanduser().resolve()
    outpath = Path(outpath).expanduser().resolve()
    if input_path == outpath:
        raise ValueError("outpath must differ from input_path")
    if not np.isfinite(distance) or distance <= 0:
        raise ValueError("distance must be finite and positive")
    if not isinstance(chunk_size, int) or chunk_size <= 0:
        raise ValueError("chunk_size must be a positive integer")
    outpath.parent.mkdir(parents=True, exist_ok=True)

    with uproot.open(input_path) as root:
        keyed_by_event_id = "eventID" in root[tree_name].keys()

    with TemporaryDirectory(prefix="lxe_clustering_", dir=input_path.parent) as work:
        db = sqlite3.connect(Path(work) / "events.sqlite")
        try:
            _create_schema(db, keyed_by_event_id)

            n_deposits = _stage_lxe_file(db, input_path, tree_name, chunk_size)
            event_totals = _merge_event_info(db, input_path, chunk_size) if keyed_by_event_id else None
            sum_weight_deposit = db.execute("SELECT SUM(weight) FROM events").fetchone()[0]
            n_events, n_clusters = _write_lxe_clusters(db, outpath, distance)
        finally:
            db.close()

    result = {
        "outpath": str(outpath),
        "nDeposits": n_deposits,
        "nEvents": n_events,
        "nClusters": n_clusters,
        "nEventsEmitted": None if event_totals is None else event_totals[0],
        "sumWeightsEmitted": None if event_totals is None else event_totals[1],
        "sumWeightsWithDeposit": sum_weight_deposit,
    }

    if write_meta:
        meta_path = outpath.with_name(outpath.stem + ".meta.json")
        meta = {
            "csv": outpath.name,
            "rootFile": input_path.name,
            "eventKey": "eventID" if keyed_by_event_id else "sourcePosition",
            "clusterDistance": distance,
            "nDeposits": n_deposits,
            "nEventsWithDeposit": n_events,
            "nClusters": n_clusters,
            "nEventsEmitted": result["nEventsEmitted"],
            "sumWeightsEmitted": result["sumWeightsEmitted"],
            "sumWeightsWithDeposit": sum_weight_deposit,
            "run": _read_run_metadata(input_path),
        }
        meta_path.write_text(json.dumps(meta, indent=2, sort_keys=True) + "\n")
        result["metaPath"] = str(meta_path)

    return result
