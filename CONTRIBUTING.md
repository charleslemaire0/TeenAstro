# Contributing to TeenAstro

Thank you for your interest in contributing to the TeenAstro project! Whether you are reporting bugs, improving documentation, submitting firmware improvements, or enhancing the control applications, your help is appreciated.

---

## 1. Development Workflow

1. **Fork and Branch**: Create a feature or fix branch from `Release_1.6` (or `master` / development branch):
   ```bash
   git checkout -b feature/my-new-feature
   # or
   git checkout -b fix/my-bug-fix
   ```
2. **Coding Standards**:
   - Follow clean, idiomatic C++ for embedded code.
   - Use bounded buffers and safe string copies (`strncpy`, `snprintf`) to avoid memory corruption on microcontrollers.
   - For mathematical calculations, maintain precision and make sure angles are properly normalized.
   - Keep shared protocol definitions in [`libraries/TeenAstroCommandDef/`](libraries/TeenAstroCommandDef/README.md) so both MainUnit and clients stay in sync.
3. **Version Numbering**:
   - Firmware releases follow the two-digit format `X.Y` (e.g. `1.6`). Do not introduce three-segment release versions (e.g. `1.6.1`). See [VERSIONING.md](VERSIONING.md) for details.

---

## 2. Testing Requirements

Before opening a pull request, ensure all desktop math and logic tests pass:

```bash
# Run all unit tests
pio test -d tests

# Or run via Python test runner
python tests/run_all_tests.py
```

If you introduce new coordinate transformations, math helpers, or guiding features, please add corresponding unit tests under `tests/`.

---

## 3. Submitting Pull Requests

- Give your Pull Request a clear and descriptive title.
- Explain the **Problem**, **Root Cause**, **Fix implemented**, and **Verification steps** in the PR description.
- Ensure all firmware targets compile cleanly:
  ```bash
  pio run -d TeenAstroMainUnit
  pio run -d TeenAstroSHC
  pio run -d TeenAstroServer
  pio run -d TeenAstroFocuser
  ```

---

## 4. Community & Support

For discussions, troubleshooting, and hardware questions, join our community forum:
- [TeenAstro Groups.io Forum & Wiki](https://groups.io/g/TeenAstro/wiki/home)
