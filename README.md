## CryptoZip

CryptoZip is a C++-based data compression and decompression tool that implements the **LZ77 compression algorithm**, optimized with **KMP string matching** to find repeated sequences efficiently. Ideal for compressing large text files with high redundancy.

## 🔧 Features

- **LZ77 Compression**
- **Efficient pattern matching** using the Knuth-Morris-Pratt (KMP) algorithm
- Binary file output with `<distance, length, nextChar>` tuples
- Decompression engine to restore the original file
- Shows compression ratio and time taken
- Modular code (separated into `src/` and `logic/` directories)
- Written entirely in modern C++

## Folder Structure

```
Cryptozip/
├── .gitignore
├── makefile
├── README.md
├── src/
│   ├── main.cpp
│   ├── compressor.cpp
│   └── decompressor.cpp
├── logic/
│   └── compressor.hpp
├── compressed.lz77
└── bigfile.txt (your input)
```

## Build Instructions 

You need a C++ compiler (like `g++`) and Make.

```bash
git clone https://github.com/awaisAhmed19/CryptoZip.git
cd CryptoZip
make
````

This will generate the `main` executable.

##  Usage

### Compression

```bash
./main
```

By default, this:

* Reads from `bigfile.txt`
* Writes compressed data to `compressed.lz77`
* Logs compression stats

###  Decompression

Add the decompression function call inside `main.cpp`:

```cpp
decompressLZ77("compressed.lz77", "decompressed.txt"); 
```

After this, recompile and run to restore the original file.

## Compression Stats Example

```bash
Original Size:     235838 bytes
Compressed Size:   153121 bytes
Compression Ratio: 0.649
Time Taken:        320 ms
```

*Stats vary depending on file redundancy and window size.* 

##  Future Plans

*  Add Huffman encoding for nextChar
*  Integrate simple XOR encryption
*  Build `.czip` custom file format
*  Web-based UI using WebAssembly (long shot, but cool) 

## Contributions 

Feel free to open issues or submit pull requests. Ideas, optimizations, and feedback welcome. 

## Credits 

Made with ✨, sweat, and C++ by [@awaisAhmed19](https://github.com/awaisAhmed19). 
