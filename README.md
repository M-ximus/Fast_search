# Fast_search
Search engine for Acronis

Sometimes we need to find huge strings in the big data set. This program can solve this problem)))

`make` - compile all project without debug info

`make debug` - it's needed only for debug

`/templates` - folder with small database with `.txt` books and project files for tests and benchmarks

`./search.out file_name path_to_database_folder` - start format

Small benchmark - `perf stat ./search.out Rabin.d templates/` - 0.06 seconds
