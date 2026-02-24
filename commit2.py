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
