# logo_processor_cpp23_v2

Questa versione migliora il progetto precedente in due punti: usa un parser JSON reale tramite `nlohmann/json` e consente di configurare più estensioni di input nello stesso job, per esempio `.jpeg`, `.jpg` e `.png`. La pipeline resta aderente allo script originale, con calcolo della diagonale, resize del logo proporzionale, composizione via ImageMagick, naming progressivo e output su directory assoluta. [file:1]

## Funzionalità

- Eseguibile lanciabile da qualsiasi directory.
- Path assoluto del logo configurabile.
- Directory assoluta di input configurabile.
- Supporto a più estensioni configurabili tramite array JSON.
- Directory assoluta di output configurabile.
- Nome base file e indice iniziale configurabili.
- Numero massimo di job paralleli configurabile.
- Posizione e margini del logo configurabili.
- Parser JSON robusto basato su `nlohmann/json`. [file:1]

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

## Esecuzione

```bash
./build/logo_processor /percorso/assoluto/config.json
```

oppure:

```bash
./build/logo_processor --config /percorso/assoluto/config.json
```

## Configurazione

```json
{
  "logo_path": "/ABSOLUTE/PATH/logo_White.png",
  "input": {
    "directory": "/ABSOLUTE/PATH/input_images",
    "extensions": [".jpeg", ".jpg", ".png"]
  },
  "output": {
    "directory": "/ABSOLUTE/PATH/output_images",
    "base_name": "DRA_FUR26_",
    "start_index": 0,
    "extension": ".jpeg"
  },
  "processing": {
    "logo_diagonal_divisor": 12.0,
    "margin_x": 10,
    "margin_y": 10,
    "gravity": "southeast",
    "max_parallel_jobs": 10
  },
  "tools": {
    "magick_command": "magick"
  }
}
```

## Compatibilità con lo script originale

Lo script allegato lavorava su file `*.jpeg`, calcolava la diagonale, derivava la dimensione del logo come `diag / 12`, applicava `logo_White.png` in basso a destra con offset `+10+10` e produceva file numerati come `DRA_FUR26_0000.jpeg`. Questo progetto conserva quella logica operativa ma la rende configurabile, più robusta nel parsing e più flessibile nell'acquisizione dei file di input. [file:1]
