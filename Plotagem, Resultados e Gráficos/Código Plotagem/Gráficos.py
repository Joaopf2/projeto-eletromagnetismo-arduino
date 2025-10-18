#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Cria gráficos I×V e R×V para todos os CSVs em 'Resultado dos Testes/'
e grava PNGs em 'Graficos Gerados/' (150 dpi), incluindo o diodo.
"""

from pathlib import Path
import pandas as pd
import matplotlib
matplotlib.use("Agg")                 # backend sem janelas
import matplotlib.pyplot as plt

# ---------- DIRETÓRIOS ----------
SRC_DIR = Path("Resultado dos Testes")
OUT_DIR = Path("Graficos Gerados")
OUT_DIR.mkdir(parents=True, exist_ok=True)

FIGSIZE  = (10, 6)
DPI      = 150
SHOW_PTS = True                     # marcar pontos?

#Utils
def auto_scale(series, kind):
    """Escala: I -> mA se <1 A; R -> kΩ se >10 kΩ. Retorna (série, unidade)."""
    if kind == "I":
        return (series*1000, "mA") if series.max() < 1 else (series, "A")
    if kind == "R":
        return (series/1000, "kΩ") if series.max() > 10000 else (series, "Ω")
    return series, ""

# Função Principal

def make_plots(csv_path: Path):
    try:
        df = pd.read_csv(csv_path, engine="python", on_bad_lines="skip").dropna(how="all")
    except Exception as e:
        print(f"× {csv_path}: erro ao ler → {e}")
        return
    if df.empty:
        print(f"× {csv_path}: arquivo vazio")
        return

    # colunas esperadas: V_Rx, I, R (mesmo para diodo)
    df.columns = [c.strip() for c in df.columns]
    if len(df.columns) < 3:
        print(f"× {csv_path}: não tem 3 colunas")
        return

    v_col, i_col, r_col = df.columns[:3]

    # converte todas para float, valores não numéricos viram NaN
    for c in (v_col, i_col, r_col):
        df[c] = pd.to_numeric(df[c], errors="coerce")
    # remove linhas sem V ou I
    df = df.dropna(subset=[v_col, i_col])
    if df.empty:
        print(f"× {csv_path}: sem dados válidos")
        return

    # aplica escalas
    I_plot, I_unit = auto_scale(df[i_col], "I")
    R_plot, R_unit = auto_scale(df[r_col], "R")
    style = "o-" if SHOW_PTS else "-"

    # gráfico I×V
    fig, ax = plt.subplots(figsize=FIGSIZE, dpi=DPI)
    ax.plot(df[v_col], I_plot, style, lw=1.2, ms=3)
    ax.set_xlabel(f"{v_col}")
    ax.set_ylabel(f"I ({I_unit})")
    ax.grid(True, which="both", lw=0.4)
    plt.title(f"I × V — {csv_path.stem}")
    plt.tight_layout()
    fig.savefig(OUT_DIR / f"{csv_path.stem}_IxV.png", dpi=DPI)
    plt.close(fig)

    # gráfico R×V
    fig, ax = plt.subplots(figsize=FIGSIZE, dpi=DPI)
    ax.plot(df[v_col], R_plot, style, color="tab:orange", lw=1.2, ms=3)
    ax.set_xlabel(f"{v_col}")
    ax.set_ylabel(f"R ({R_unit})")
    ax.grid(True, which="both", lw=0.4)
    plt.title(f"R × V — {csv_path.stem}")
    plt.tight_layout()
    fig.savefig(OUT_DIR / f"{csv_path.stem}_RxV.png", dpi=DPI)
    plt.close(fig)

    print(f" {csv_path.relative_to(SRC_DIR)} → gráficos salvos")

# Execução
if __name__ == "__main__":
    for csv_file in SRC_DIR.rglob("*.csv"):
        make_plots(csv_file)
