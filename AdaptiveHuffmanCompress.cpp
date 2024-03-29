/* 
 * Compression application using adaptive Huffman coding
 * 
 * Usage: AdaptiveHuffmanCompress InputFile OutputFile
 * Then use the corresponding "AdaptiveHuffmanDecompress" application to recreate the original input file.
 * Note that the application starts with a flat frequency table of 257 symbols (all set to a frequency of 1),
 * collects statistics while bytes are being encoded, and regenerates the Huffman code periodically. The
 * corresponding decompressor program also starts with a flat frequency table, updates it while bytes are being
 * decoded, and regenerates the Huffman code periodically at the exact same points in time. It is by design that
 * the compressor and decompressor have synchronized states, so that the data can be decompressed properly.
 * 
 * Copyright (c) Project Nayuki
 * 
 * https://www.nayuki.io/page/reference-huffman-coding
 * https://github.com/nayuki/Reference-Huffman-coding
 */

#include <boost/iostreams/device/mapped_file.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "BitIoStream.hpp"
#include "FrequencyTable.hpp"
#include "HuffmanCoder.hpp"

using std::uint32_t;


static bool isPowerOf2(uint32_t x);


extern std::vector<char> v1;

int main(int argc, char *argv[]) {
	// Handle command line arguments
	if (argc != 3) {
		std::cerr << "Usage: " << argv[0] << " InputFile OutputFile" << std::endl;
		return EXIT_FAILURE;
	}
	const char *inputFile  = argv[1];
	const char *outputFile = argv[2];
	
	// Perform file compression
	std::ifstream in(inputFile, std::ios::binary);
	std::ofstream out(outputFile, std::ios::binary);

	boost::iostreams::mapped_file_source file;
	int numberOfBytes = 143666240;
	file.open(inputFile, numberOfBytes,0);
	unsigned char * data = (unsigned char *)file.data();

	BitOutputStream bout(out);
	try {
		
		const std::vector<uint32_t> initFreqs(257, 1);
		FrequencyTable freqs(initFreqs);
		HuffmanEncoder enc(bout);
		CodeTree tree = freqs.buildCodeTree();  // Don't need to make canonical code because we don't transmit the code tree
		enc.codeTree = &tree;
		uint32_t count = 0;  // Number of bytes read from the input file
		for(int i = 0; i < numberOfBytes; i++) {
			int symbol = (int) data[i];
			// Read and encode one byte
			if (symbol == EOF)
				break;
			if (symbol < 0 || symbol > 255)
				throw std::logic_error("Assertion error");
			enc.write(static_cast<uint32_t>(symbol));
			count++;
			
			// Update the frequency table and possibly the code tree
			freqs.increment(static_cast<uint32_t>(symbol));
			if ((count < 262144 && isPowerOf2(count)) || count % 262144 == 0)  // Update code tree
				tree = freqs.buildCodeTree();
			if (count % 262144 == 0)  // Reset frequency table
				freqs = FrequencyTable(initFreqs);
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


static bool isPowerOf2(uint32_t x) {
	return x > 0 && (x & (x - 1)) == 0;
}
