"""
Worst-case flux density for a gapped E-core magnetic circuit.

Modes:
    - original
    - ab_center
    - c_center
"""

import sys
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


# ── Configuration ────────────────────────────────────────────────────────────
MODE = "c_center"  # "original" | "ab_center" | "c_center"
C_CURRENT_MODE = "sum"  # "ia" | "ib" | "sum"

B_POLARITY = 1.0
C_POLARITY = 1.0


# ── Constants ────────────────────────────────────────────────────────────────
mu0 = 4.0 * np.pi * 1e-7

# Geometry
A_e = 738e-6
l_c_outer = 266.8e-3
l_c_middle = 274e-3

# Windings
N_a = 38
N_c = 175

# Material
mu_r = 4500
mu_c = mu_r * mu0

# Default currents
I_a_peak = 10.0
I_b_peak = 10.0


# ── Helpers ──────────────────────────────────────────────────────────────────
def center_current(i_a, i_b):
    if C_CURRENT_MODE == "ia":
        return i_a
    if C_CURRENT_MODE == "ib":
        return i_b
    return i_a + i_b


def reluctances(l_g):
    """Compute reluctances and permeances."""
    g = l_g / 2.0

    R_gap_outer = g / (mu0 * (A_e / 2))
    R_gap_center = g / (mu0 * A_e)

    R_core_outer = l_c_outer / (mu_c * (A_e / 2))
    R_core_center = l_c_middle / (mu_c * A_e)

    R_a = R_gap_outer + R_core_outer
    R_b = R_a
    R_c = R_gap_center + R_core_center

    P_a = 1.0 / R_a
    P_b = 1.0 / R_b
    P_c = 1.0 / R_c

    return {
        "g": g,
        "R_a": R_a,
        "R_b": R_b,
        "R_c": R_c,
        "P_a": P_a,
        "P_b": P_b,
        "P_c": P_c,
        "P_sum": P_a + P_b + P_c,
    }


def branch_mmf(i_a, i_b):
    """Return MMFs and reluctance model."""
    if MODE == "original":
        l_g = 9.5e-3
        mmf_a = N_a * i_a
        mmf_b = B_POLARITY * N_a * i_b
        mmf_c = C_POLARITY * N_c * (i_a + i_b)

    elif MODE == "ab_center":
        l_g = 4.45e-3
        mmf_a = 0.0
        mmf_b = 0.0
        mmf_c = C_POLARITY * N_a * (i_a + i_b)

    elif MODE == "c_center":
        l_g = 9.5e-3
        mmf_a = 0.0
        mmf_b = 0.0
        mmf_c = C_POLARITY * N_c * center_current(i_a, i_b)

    else:
        raise ValueError(f"Unknown MODE: {MODE}")

    rel = reluctances(l_g)
    return mmf_a, mmf_b, mmf_c, rel


def solve(i_a, i_b):
    mmf_a, mmf_b, mmf_c, rel = branch_mmf(i_a, i_b)

    V_node = (
        mmf_a * rel["P_a"] +
        mmf_b * rel["P_b"] +
        mmf_c * rel["P_c"]
    ) / rel["P_sum"]

    phi_a = (mmf_a - V_node) * rel["P_a"]
    phi_b = (mmf_b - V_node) * rel["P_b"]
    phi_c = (mmf_c - V_node) * rel["P_c"]

    return phi_a, phi_b, phi_c, rel


def to_B(phi_a, phi_b, phi_c):
    B_a = phi_a / (A_e / 2)
    B_b = phi_b / (A_e / 2)
    B_c = phi_c / A_e
    return B_a, B_b, B_c


def report(t, i_a, i_b, label):
    phi_a, phi_b, phi_c, rel = solve(i_a, i_b)
    B_a, B_b, B_c = to_B(phi_a, phi_b, phi_c)

    abs_B = np.stack([np.abs(B_a), np.abs(B_b), np.abs(B_c)])
    pk = abs_B.max(axis=1)

    wi = np.argmax(pk)
    ti = np.argmax(abs_B[wi])

    names = ["A (left)", "B (right)", "C (center)"]

    print("=" * 70)
    print(label)
    print("=" * 70)
    print(f"MODE = {MODE}")
    print(f"g = {rel['g']*1e3:.3f} mm")
    print()

    print("Peak |B|:")
    for i, name in enumerate(names):
        print(f"  {name}: {pk[i]:.4f} T")

    print("\nWorst case:")
    print(f"  |B| = {pk.max():.4f} T ({names[wi]})")
    print(f"  t   = {t[ti]:.6e}")
    print(f"  ia  = {i_a[ti]:+.3f} A")
    print(f"  ib  = {i_b[ti]:+.3f} A")
    print(f"  Ba  = {B_a[ti]:+.4f} T")
    print(f"  Bb  = {B_b[ti]:+.4f} T")
    print(f"  Bc  = {B_c[ti]:+.4f} T")
    print("=" * 70)

    return t, (B_a, B_b, B_c)


def save_plot(t, i_a, i_b, B_tuple, path="B_waveforms.png"):
    B_a, B_b, B_c = B_tuple
    t_us = t * 1e6

    fig, axes = plt.subplots(3, 1, figsize=(10, 8), sharex=True)

    axes[0].plot(t_us, B_a)
    axes[0].plot(t_us, B_b)
    axes[0].plot(t_us, B_c)
    axes[0].set_ylabel("B (T)")

    axes[1].plot(t_us, i_a + i_b)
    axes[1].set_ylabel("i_a + i_b")

    axes[2].plot(t_us, i_a - i_b)
    axes[2].set_ylabel("i_a - i_b")
    axes[2].set_xlabel("Time (µs)")

    fig.tight_layout()
    fig.savefig(path, dpi=150)
    print(f"Saved plot -> {path}")


def load_csv(path):
    import csv
    data = []
    with open(path, encoding="utf-8-sig") as f:
        reader = csv.reader(f)
        next(reader, None)
        for r in reader:
            if len(r) >= 3:
                data.append([float(r[0]), float(r[1]), float(r[2])])

    d = np.array(data)
    return d[:, 0], d[:, 1], d[:, 2]


# ── Main ─────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    args = sys.argv[1:]
    do_plot = "--plot" in args
    csv_path = next((a for a in args if not a.startswith("--")), None)

    if csv_path:
        t, i_a, i_b = load_csv(csv_path)
        t, B_tuple = report(t, i_a, i_b, f"CSV: {csv_path}")
        if do_plot:
            save_plot(t, i_a, i_b, B_tuple)

    else:
        print("Synthetic sweep\n")
        worst = 0

        for sa in [-1, 1]:
            for sb in [-1, 1]:
                ia = sa * I_a_peak
                ib = sb * I_b_peak

                B_a, B_b, B_c = to_B(*solve(ia, ib)[:3])
                Bm = max(abs(B_a), abs(B_b), abs(B_c))

                print(f"ia={ia:+6.1f} ib={ib:+6.1f} -> max|B|={Bm:.4f} T")

                worst = max(worst, Bm)

        print(f"\nWorst-case B = {worst:.4f} T\n")