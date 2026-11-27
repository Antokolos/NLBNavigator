cmake -S . -B /workspace/scratch/paired-save-load.Lkx4pC/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DNLB_BUILD_APPS=ON -DNLB_WITH_SDL3=ON
cmake --build /workspace/scratch/paired-save-load.Lkx4pC/build --target nlb_sdl nlb_save_portion_test --parallel 4
cmake -S . -B /workspace/scratch/paired-save-load.Lkx4pC/build -G Ninja -DNLB_BUILD_TESTS=ON
cmake --build /workspace/scratch/paired-save-load.Lkx4pC/build --target nlb_sdl --parallel 4
nice -n 8 cmake --build /workspace/scratch/paired-save-load.Lkx4pC/build --target nlb_save_portion_test --parallel 1
ctest --test-dir /workspace/scratch/paired-save-load.Lkx4pC/build --output-on-failure -R '^save_portion\.'
python3 -m unittest discover -s tests/differential -p 'test_*.py'
python3 tests/differential/validate_catalog.py
git diff --check
