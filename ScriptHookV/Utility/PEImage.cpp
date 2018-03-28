#include "PEImage.h"

namespace Utility {

	PEImage::PEImage() {

	}

	PEImage::~PEImage() {

	}

	bool PEImage::Load( const std::string & path ) {

		filePath = path;

		std::ifstream inputFile( path, std::ios::binary );
		if ( inputFile.fail() ) {
			return false;
		}
		std::vector<char> bufferTemp( ( std::istreambuf_iterator<char>( inputFile ) ), ( std::istreambuf_iterator<char>() ) );
		fileBuffer.swap( bufferTemp );

		bool parseSuccess = ParsePE();
		if ( !parseSuccess ) {
			return false;
		}

		return true;
	}

	bool PEImage::ParsePE() {

		// Get DOS header
		const IMAGE_DOS_HEADER * dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>( fileBuffer.data() );

		// Not a valid PE
		if ( dosHeader->e_magic != IMAGE_DOS_SIGNATURE ) {
			return false;
		}

		// Get NT header
		ntHeader = reinterpret_cast<const IMAGE_NT_HEADERS64*>( reinterpret_cast<const uint8_t*>(dosHeader)+dosHeader->e_lfanew );

		return true;
	}

	uint64_t PEImage::GetDirectoryAddress( int index ) {

		const IMAGE_DATA_DIRECTORY * dataDirectory = ntHeader->OptionalHeader.DataDirectory;

		return RVAToVA( dataDirectory[index].VirtualAddress );
	}

	uint64_t PEImage::RVAToVA( uint32_t rva ) const {

		const IMAGE_SECTION_HEADER * sectionHeader = reinterpret_cast<const IMAGE_SECTION_HEADER*>( ntHeader + 1 );
		for ( int i = 0; i < ntHeader->FileHeader.NumberOfSections; ++i, ++sectionHeader ) {

			if ( rva >= sectionHeader->VirtualAddress && rva <= sectionHeader->VirtualAddress + sectionHeader->Misc.VirtualSize ) {

				return reinterpret_cast<uint64_t>(fileBuffer.data())+( rva - sectionHeader->VirtualAddress + sectionHeader->PointerToRawData );
			}
		}

		return 0;
	}
}
