---
# SimpleOfficeSuite

## Description


I decided to make a simple office suite that just worked, looked nice, and didnt have a billion options to play with, like Microsoft Word or Google Docs.

---

### Build Commands

```bash
# Clone the repository
git clone [https://github.com/TwizzlerTheRizzler/SimpleOffice.git](https://github.com/TwizzlerTheRizzler/SimpleOffice.git)
cd SimpleOffice

# Generate build files
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Compile the project
cmake --build build --config Release
