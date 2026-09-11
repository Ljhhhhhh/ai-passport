<p align="right">
  <a href="build-and-test.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Build and Test

Use ESP-IDF 5.5.x; the reproducible environment is ESP-IDF 5.5.3.

```bash
get_idf553                                     # activate the local ESP-IDF 5.5.3 environment
idf.py -C projects/animal-hanzi-story build     # compile Animal Hanzi Story firmware
idf.py -C projects/hanzi-cards build            # compile Hanzi Cards firmware
idf.py -C projects/bsp-demo build               # compile BSP Demo firmware
idf.py -C projects/animal-hanzi-story flash monitor  # flash and open serial monitor
```

The tracked `dependencies.lock` pins Managed Component resolution. After changing an `idf_component.yml`, regenerate the lock with ESP-IDF 5.5.3, review version changes, and commit it with the manifest. An ordinary build must not leave an unexplained lock-file diff.

Firmware validation builds projects inside `projects/` using isolated build directories and project-specific `sdkconfig.defaults`. It verifies the generated merged binaries and copies verified binaries to `build/<project_name>-full.bin` (and `build/FoloToy-AI-Passport-full.bin`).

The baseline also has hardware-independent logic tests. The static gate compiles
and runs pixel-layout, animal-hanzi story/save, hanzi-cards model/assets, and IMA ADPCM/player tests with
`-Wall -Wextra -Werror`.

```bash
./tools/validate.sh --static
```

```bash
./tools/validate.sh --static                      # repository checks, workflows, links, host tests
./tools/validate.sh --firmware animal-hanzi-story # build and verify a specific project
./tools/validate.sh --firmware                    # build and verify all projects in projects/
./tools/validate.sh                               # complete gate; requires an activated ESP-IDF environment
```

CI calls the same script. Fix the shared script or environment if local and CI behavior differs; do not duplicate command sequences in workflows.

Hardware-affecting changes must also run the applicable on-device checklist in the hardware guide. Report compilation separately from physical-device validation.
