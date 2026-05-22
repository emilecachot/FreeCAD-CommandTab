# Addon Manager Submission

This repository is prepared to be published as a FreeCAD workbench addon, but the
final GitHub repository URL must be known before submitting it to the official
index.

## Repository Checklist

1. Create the public GitHub repository
   `https://github.com/emilecachot/FreeCAD-CommandTab`.
1. Add the GitHub topics `freecad` and `addon`.
1. Push the source tree, including:
   - `package.xml`
   - `README.md`
   - `Resources/icons/FreecadNew.svg`
   - `freecad_commandtab/native/bin/linux/qt6/libfreecad_commandtab_native_backend.so`
   - `freecad_commandtab/native/bin/macos/qt6/libfreecad_commandtab_native_backend.dylib`
   - `freecad_commandtab/native/bin/windows/qt6/freecad_commandtab_native_backend.dll`
1. Do not push local/generated folders such as `build/`, `dist/`, `.qt/`,
   `.venv*/`, `.pytest_cache/`, or `.ruff_cache/`.
1. Do not push Windows build by-products such as `.pdb`, `.lib`, `.exp`,
   `.dll.a`, or old MinGW runtime DLLs.
1. Update these fields in `package.xml`:
   - `<maintainer>`
   - `<url type="repository">`
   - `<url type="readme">`
   - `<url type="website">`, if you publish documentation elsewhere

The Addon Manager reads the README in its own UI, so keep it plain Markdown.
Avoid HTML-only layout or browser-specific rendering.

## Current Addon Index

For FreeCAD 1.0 and newer, request inclusion in the current addon index:

- Repository: `https://github.com/FreeCAD/Addons`
- Data file: `Data/Index.json`

Example entry to add:

```json
"FreeCAD-CommandTab": {
  "repository": "https://github.com/emilecachot/FreeCAD-CommandTab",
  "git_ref": "main",
  "branch_display_name": "main",
  "zip_url": "https://github.com/emilecachot/FreeCAD-CommandTab/archive/refs/heads/main.zip",
  "curated": true
}
```

Open a pull request against `FreeCAD/Addons` with that entry once the public
repository is ready.

## Legacy Index

For compatibility with older FreeCAD versions, the legacy index still exists at
`https://github.com/FreeCAD/FreeCAD-addons`. Its submission notes ask for a PR
that updates `.gitmodules` and `AddonCatalog.json`. Treat that as optional
legacy coverage; FreeCAD 1.0 and newer use `FreeCAD/Addons`.
