# logo_processor_cpp23_v2

This version improves the previous project in two ways: it uses a proper JSON parser through `nlohmann/json` and allows multiple input extensions to be configured in the same job, for example `.jpeg`, `.jpg`, and `.png`. The processing pipeline remains faithful to the original script, including diagonal calculation, proportional logo resizing, ImageMagick-based compositing, sequential file naming, and output to an absolute directory.

## Features

- Executable can be launched from any directory.
- Configurable absolute path for the logo.
- Configurable absolute input directory.
- Support for multiple configurable extensions through a JSON array.
- Configurable absolute output directory.
- Configurable base file name and starting index.
- Configurable maximum number of parallel jobs.
- Configurable logo position and margins.
- Robust JSON parser based on `nlohmann/json`.

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

## Execution

```bash
./build/logo_processor /absolute/path/config.json
```

or:

```bash
./build/logo_processor --config /absolute/path/config.json
```

## Configuration

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

## Compatibility with the original script

The attached script worked on `*.jpeg` files, calculated the diagonal, derived the logo size as `diag / 12`, applied `logo_White.png` in the bottom-right corner with offset `+10+10`, and produced numbered files such as `DRA_FUR26_0000.jpeg`. This project preserves that processing logic while making it configurable, more robust in parsing, and more flexible in input file discovery.