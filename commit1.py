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
