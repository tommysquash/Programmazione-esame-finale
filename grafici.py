# plot_sensori.py — COMMIT 1: import  COMMIT 2: preparazione  COMMIT 3: grafici
# pip install pandas matplotlib

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
CSV = r"D:\Programmazione test finale\Python\log_monitoraggio_25-07-2022.csv"
CSV  = "log_monitoraggio_25-07-2022.csv"
COLS = ["#e41a1c","#377eb8","#4daf4a","#984ea3","#ff7f00","#a65628","#f781bf","#999999","#66c2a5","#fc8d62"]

# ── COMMIT 1: lettura ────────────────────────────────────────────────────────
# Legge CSV InfluxDB (salta 4 righe header), prende _time/_value/entity_id,
# converte timestamp ISO8601, corregge accel_x (segno invertito = glitch firmware)
def load(path):
    df = pd.read_csv(path, skiprows=[0,1,2], low_memory=False)[["_time","_value","entity_id"]]
    df.columns = ["t","v","s"]
    df["v"] = pd.to_numeric(df["v"], errors="coerce")
    df.dropna(subset=["v","s"], inplace=True)
    df["t"] = pd.to_datetime(df["t"], utc=True).dt.tz_localize(None)
    df = df.sort_values("t")
    df.loc[(df["s"]=="fridge_black_accel_x") & (df["v"]>5), "v"] *= -1  # FIX outlier
    return {n: g[["t","v"]].reset_index(drop=True) for n,g in df.groupby("s")}

# ── COMMIT 2: asse ───────────────────────────────────────────────────────────
# Formatta asse X con HH:MM, tick major 4h / minor 1h, griglia doppia
# μ = (1/N)·Σxᵢ  |  σ = √((1/(N-1))·Σ(xᵢ-μ)²)   calcolati con pandas
def fmt(ax, title):
    ax.set_title(title, fontsize=9, fontweight="bold")
    ax.xaxis.set_major_formatter(mdates.DateFormatter("%H:%M"))
    ax.xaxis.set_major_locator(mdates.HourLocator(interval=4))
    ax.xaxis.set_minor_locator(mdates.HourLocator(interval=1))
    plt.setp(ax.xaxis.get_majorticklabels(), rotation=45, ha="right", fontsize=7)
    ax.tick_params(axis="y", labelsize=7)
    ax.grid(True, which="major", ls="--", alpha=0.4)
    ax.grid(True, which="minor", ls=":",  alpha=0.2)

# ── COMMIT 3: grafici ────────────────────────────────────────────────────────
# _panel disegna una curva + linea media + riquadro μ/σ su un asse dato
# transAxes posiziona il testo in coordinate 0-1 indipendenti dalla scala Y
def _panel(ax, df, color, title):
    mu, sg = df["v"].mean(), df["v"].std()
    ax.plot(df["t"], df["v"], color=color, lw=0.9)
    ax.axhline(mu, color=color, ls="--", alpha=0.6)
    ax.text(0.01, 0.97, f"μ={mu:.3f} σ={sg:.3f}", transform=ax.transAxes,
            fontsize=7, va="top", bbox=dict(fc="white", alpha=0.8, ec=color))
    fmt(ax, title)

def plot_group(data, names, title, out):
    rows = [(n, data[n]) for n in names if n in data]
    fig, axes = plt.subplots(len(rows), 1, figsize=(13, len(rows)*3))
    if len(rows)==1: axes=[axes]
    fig.suptitle(title, fontsize=12, fontweight="bold")
    for ax,(name,df),c in zip(axes, rows, COLS):
        _panel(ax, df, c, name.replace("fridge_black_","").replace("airq_black_","").replace("_"," "))
    fig.tight_layout(); fig.savefig(out, dpi=150, bbox_inches="tight"); plt.close(fig)
    print(f"[OK] {out}")

def plot_all(data, out):
    names = sorted(data.keys()); nr = (len(names)+1)//2
    fig, axes = plt.subplots(nr, 2, figsize=(16, nr*3.5))
    fig.suptitle("Panoramica sensori — 22 Luglio 2025", fontsize=12, fontweight="bold")
    for i,n in enumerate(names):
        _panel(axes.flat[i], data[n], COLS[i%len(COLS)],
               n.replace("fridge_black_","").replace("airq_black_","").replace("_"," "))
    for ax in axes.flat[len(names):]: ax.set_visible(False)
    fig.tight_layout(); fig.subplots_adjust(hspace=0.6)
    fig.savefig(out, dpi=150, bbox_inches="tight"); plt.close(fig)
    print(f"[OK] {out}")

if __name__ == "__main__":
    import os
import pandas as pd

# Questo trova la cartella dove risiede fisicamente il file .py
base_path = os.path.dirname(os.path.abspath(__file__))

# Unisce la cartella al nome del file CSV
CSV = os.path.join(base_path, "log_monitoraggio_25-07-2022.csv")

d = load(CSV)
plot_group(d, ["fridge_black_accel_x","fridge_black_accel_y","fridge_black_accel_z"],   "Accelerometri (m/s²)",  "grafico_accelerometri.png")   
plot_group(d, ["fridge_black_gyro_x","fridge_black_gyro_y","fridge_black_gyro_z"],       "Giroscopi (°/s)",       "grafico_giroscopi.png")
plot_group(d, ["airq_black_temperature","airq_black_temperature_sen55","fridge_black_internal_temperature","fridge_black_probe_temperature"], "Temperature (°C)", "grafico_temperature.png")
plot_all(d, "grafico_panoramica.png")