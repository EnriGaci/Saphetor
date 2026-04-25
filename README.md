
# Project overview and architecture summary

# Build instructions

`mkdir build`<br>
`cd build` <br>
`cmake ..` <br>
`cmake --build .` <br>

# Required environment variables for database access
## Linux
`export SQLITEDBNAME=vcf_database.db`

## Visual Studio
- Right-click your project in Solution Explorer and select Properties.
- Go to Configuration Properties → Debugging.
- In the Environment field, add your variable like this
`SQLITEDBNAME=vcf_database.db`


# How to run the application (with examples)

`build/vcf_importer --vcf PATH --threads N`

# Filesystem

- src: the source files <br>
- include: the headers <br>

# Third-Party Libraries

gtest : automatically installed through cmake

# Database Configurarion Environment Variables