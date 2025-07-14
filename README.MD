Tool for generating preview for obj files

**Features:**

- 360 degree obj model overview gif
- Materials' colors preview
- Config file for preview settings
- File's comments preview
- Statistics for vertices, faces, shapes

**Config file format:**

- ~/.config/cpp_obj-preview.conf locattion
- key=value pairs

| Key | Value |
| - | - |
| view-cmd | Command for auto opening saved .md file |
| save-dir(current dir by default) | Directory for saving .md and .gif files |
| overwrite-flag(false by default) | Flag for ffmpeg .gif overwriting (1, true) |

**Requirements:**

- OpenGL package installed
- ffmpeg package installed

**Build:**

```bash
cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -S . -B build
cmake --build build
```

**Usage:**

    exe [input.obj | mode]

    modes:

        - clean - deletes .md and .gif files from save-dir

**Example:**

[Preview example](obj-preview.md)

**Some details:**

- .mtl file should be in the same directory as input.obj file
