/* 
 * Compression application using static Huffman coding
 * 
 * Usage: HuffmanCompress InputFile OutputFile
 * Then use the corresponding "HuffmanDecompress" application to recreate the original input file.
 * Note that the application uses an alphabet of 257 symbols - 256 symbols for the byte values
 * and 1 symbol for the EOF marker. The compressed file format starts with a list of 257
 * code lengths, treated as a canonical code, and then followed by the Huffman-coded data.
 * 
 * Copyright (c) Project Nayuki
 * 
 * https://www.nayuki.io/page/reference-huffman-coding
 * https://github.com/nayuki/Reference-Huffman-coding
 */

// g++ HuffmanCompress.cpp FrequencyTable.cpp CodeTree.cpp CanonicalCode.cpp BitIoStream.cpp HuffmanCoder.cpp  -I boost/include -lboost_iostreams -std=c++11 -O1 -Wall -Wextra -fsanitize=undefined
#include <boost/iostreams/device/mapped_file.hpp>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <vector>
#include "BitIoStream.hpp"
#include "CanonicalCode.hpp"
#include "FrequencyTable.hpp"
#include "HuffmanCoder.hpp"

using std::uint32_t;

extern std::vector<char> v1;

int main(int argc, char *argv[]) {
	// Handle command line arguments
	if (argc != 3) {
		std::cerr << "Usage: " << argv[0] << " InputFile OutputFile" << std::endl;
		return EXIT_FAILURE;
	}
	const char *inputFile  = argv[1];
	const char *outputFile = argv[2];
	
	// Read input file once to compute symbol frequencies.
	// The resulting generated code is optimal for static Huffman coding and also canonical.
	boost::iostreams::mapped_file_source file;
	int numberOfBytes = 143666240;
	file.open(inputFile, numberOfBytes,0);
	unsigned char * data = (unsigned char *)file.data();
	FrequencyTable freqs(std::vector<uint32_t>(257, 0));
	for(int i = 0; i < numberOfBytes; i++) {
		int b = (int) data[i];
		if (b == EOF)
			break;
		if (b < 0 || b > 255)
			throw std::logic_error("Assertion error");
		freqs.increment(static_cast<uint32_t>(b));
	}
	freqs.increment(256);  // EOF symbol gets a frequency of 1
	CodeTree code = freqs.buildCodeTree();
	const CanonicalCode canonCode(code, freqs.getSymbolLimit());
	// Replace code tree with canonical one. For each symbol,
	// the code value may change but the code length stays the same.
	code = canonCode.toCodeTree();
	
	// Read input file again, compress with Huffman coding, and write output file
	std::ofstream out(outputFile, std::ios::binary);
	BitOutputStream bout(out);
	try {
		
		// Write code length table
		for (uint32_t i = 0; i < canonCode.getSymbolLimit(); i++) {
			uint32_t val = canonCode.getCodeLength(i);
			// For this file format, we only support codes up to 255 bits long
			if (val >= 256)
				throw std::domain_error("The code for a symbol is too long");
			// Write value as 8 bits in big endian
			for (int j = 7; j >= 0; j--)
				bout.write((val >> j) & 1);
		}
		
		HuffmanEncoder enc(bout);
		enc.codeTree = &code;
		for(int i = 0; i < numberOfBytes; i++) {
			// Read and encode one byte
			int symbol = data[i];
			if (symbol == EOF)
				break;
			if (symbol < 0 || symbol > 255)
				throw std::logic_error("Assertion error");
			enc.write(static_cast<uint32_t>(symbol));
		}
		enc.write(256);  // EOF
		bout.finish();

		FILE* out1 = fopen(outputFile, "wb");
		fwrite(&v1[0], 1, v1.size(), out1);
		fclose(out1);

		return EXIT_SUCCESS;
		
	} catch (const char *msg) {
		std::cerr << msg << std::endl;
		return EXIT_FAILURE;
	}
}
