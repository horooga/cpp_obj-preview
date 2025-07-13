Tool for generating preview for obj files

**Features:**

- File's comments
- Statistics for vertices, faces, shapes
- Materials' colors overview
- 360 degree obj model overview gif

**Requirements:**

- OpenGL package installed
- ffmpeg package installed

**Build:**

```bash
cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -S . -B build
cmake --build build
```

**Usage:**

./build/cpp_obj-preview [input.obj file]

**Example:**

[Preview example](obj-preview.md)
