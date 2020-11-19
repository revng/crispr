# CRISPR patcher

This script compiles the patch using CRISPR-jit, 
links the code and merges it into the original binary, 
doing dirty ELF manipulations.

## Installing

```bash
python3 setup.py bdist_wheel
pip install --user <generated_package>
```
