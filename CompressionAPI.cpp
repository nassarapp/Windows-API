#include <Windows.h>
#include <iostream>
#include <compressapi.h>
#include <stdio.h>
#pragma comment (lib, "Cabinet.lib")



int main()
{
	using namespace std;

	COMPRESSOR_HANDLE  CompressorHandle = NULL;
	char uncompressed[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char compressed[1024];

	size_t CompressedDataSize;


	if (!CreateCompressor(COMPRESS_ALGORITHM_MSZIP, NULL, &CompressorHandle))
	{
		cout << "Error gaining compressor handle " << GetLastError() << endl;
		return -1;
	}

	
	if (!Compress(CompressorHandle, uncompressed, sizeof(uncompressed), compressed, sizeof(compressed), &CompressedDataSize))
	{
		cout << "Error with Compress " << GetLastError() << endl;
		return -1;
	}

	cout << "uncompressed data " << uncompressed << endl;
	//cout << "compressed data " << compressed << endl;
	printf("%p\n", compressed);
	getchar();

	cout << "Size of compressed data " << sizeof(compressed) << endl;

	DECOMPRESSOR_HANDLE  DeCompressorHandle = NULL;

	if (!CreateDecompressor(COMPRESS_ALGORITHM_MSZIP, NULL, &DeCompressorHandle))
	{
		cout << "Error in creating decompressor handle " << GetLastError() << endl;
	}

	char decompressed[256];
	size_t DeCompressedDataSize;

	if (!Decompress(DeCompressorHandle, compressed, sizeof(compressed), decompressed, sizeof(decompressed), &DeCompressedDataSize))
	{
		cout << "Error in Decompression " << GetLastError() << endl;
	}

	cout << "Decompressed data " << decompressed << endl;

	CloseCompressor(CompressorHandle);
	CloseDecompressor(DeCompressorHandle);

	

	return 0;
}