#include "PUP.h"
#include "zstr.hpp"
#include <string>

#include <iostream>
#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

CPUP::CPUP(std::string inputfile) :
	m_inputfile(inputfile)
	, m_reader(inputfile.c_str())
	, m_header(&m_reader)
{
	PopulateMaps();
	ProcessHeaderEntries();
}

bool CPUP::isValid()
{
	return m_header.magic == 0xEEF51454;
}

std::map<int, Entry> CPUP::GetEntriesMap()
{
	return m_entries;
}

void CPUP::PopulateMaps()
{
	{
		m_fileNames[1] = "eula.xml";//
		m_fileNames[2] = "updatemode.elf";//
		m_fileNames[3] = "emc_salina_a0.bin";//
		m_fileNames[4] = "mbr.bin";//
		m_fileNames[5] = "kernel.bin";//
		m_fileNames[6] = "unk_06.bin";//
		m_fileNames[7] = "unk_07.bin";//
		m_fileNames[8] = "unk_08.bin";//
		m_fileNames[9] = "unk_09.bin";//
		m_fileNames[10] = "CP.bin";//
		m_fileNames[11] = "titania.bls";//
		m_fileNames[12] = "version_name.xml";//
		m_fileNames[13] = "emc_salina_b0.bin";//
		m_fileNames[14] = "eap_kbl.bin";//
		m_fileNames[15] = "bd_firm_info.json";//
		m_fileNames[16] = "emc_salina_c0.bls";//
		m_fileNames[17] = "floyd_salina_c0.bls";//
		m_fileNames[18] = "usb_pdc_salina_c0.bls";//
		m_fileNames[20] = "emc_salina_d0.bls";//
		m_fileNames[21] = "eap_kbl_2.bin";//
		m_fileNames[256] = "ariel_a0.bin";//
		m_fileNames[257] = "oberon_sec_ldr_a0.bin";//
		m_fileNames[258] = "oberon_sec_ldr_b0.bin";//
		m_fileNames[259] = "oberon_sec_ldr_c0.bin";//
		m_fileNames[260] = "oberon_sec_ldr_d0.bin";//
		m_fileNames[261] = "oberon_sec_ldr_f0.bin";//
		m_fileNames[262] = "oberon_sec_ldr_e0.bin";//
		m_fileNames[752] = "qa_test_1.pkg";//
		m_fileNames[753] = "qa_test_2.pkg";//
		m_fileNames[754] = "qa_test_3.pkg";//
		
	}
	
	{
		m_deviceNames[512] = "/dev/unk_512.bin";//
		m_deviceNames[513] = "/dev/wlanbt.bin";//impossible to handle without keys
		m_deviceNames[514] = "/dev/unk_514.bin";//
		m_deviceNames[515] = "/dev/ssd0.system_b";//impossible to handle without keys
		m_deviceNames[516] = "/dev/ssd0.system_ex_b";//impossible to handle without keys
		m_deviceNames[517] = "/dev/unk_517.bin";//
		m_deviceNames[518] = "/dev/unk_518.bin";//
		m_deviceNames[519] = "/dev/ssd0.preinst";//
	}
}

std::string CPUP::PrepareOutputName(int i, std::string name)
{

		std::string output;
		std::string device =  name;
		auto pos = device.rfind("/");
		if (pos != std::string::npos)
		{
			std::string dir = device.substr(0, pos + 1);
			std::string file = device.substr(pos + 1, device.length());
			output = dir + std::to_string(i) + "_" + file;
		}
		else
		{
			output = std::to_string(i) + "_" + name;
		}
		return output;
}

void CPUP::ProcessHeaderEntries()
{
	for (int i = 0; i < m_header.entryCount; i++)
	{
		Entry entry;
		entry.Flags = m_reader.Read<uint32_t>();
		m_reader.seekg(4, std::ios_base::cur);
		entry.Offset = m_reader.Read<uint64_t>();
		entry.CompressedSize = m_reader.Read<uint64_t>();
		entry.UncompressedSize = m_reader.Read<uint64_t>();
		m_entries[i] = entry;
	}

	for (int i = 0; i < m_header.entryCount; i++)
	{
		m_tableEntries[i] = -2;
	}

	for (int i = 0; i < m_header.entryCount; i++)
	{
		Entry entry = m_entries[i];
		if (entry.IsBlocked() == false)
		{
			m_tableEntries[i] = -2;
		}
		else
		{
			if (((entry.Id() | 0x100) & 0xF00) == 0xF00)
			{
				throw std::logic_error("InvalidOperationException");
			}

			int tableIndex = -1;
			for (int j = 0; j < m_header.entryCount; j++)
			{
				if ((m_entries[j].Flags & 1) != 0)
				{
					if (m_entries[j].Id() == i)
					{
						tableIndex = j;
						break;
					}
				}
			}

			if (tableIndex < 0)
			{
				throw std::logic_error("InvalidOperationException");
			}

			if (m_tableEntries[tableIndex] != -2)
			{
				throw std::logic_error("InvalidOperationException");
			}

			m_tableEntries[tableIndex] = i;
		}
	}
	
	for (int i = 0; i < m_entries.size(); i++)
	{
		Entry* entry = &m_entries[i];
		
		int special = entry->Flags & 0xF0000000;
		if (special == 0xE0000000 || special == 0xF0000000)
		{
			continue;
		}
		
		std::string name;
		
		if (m_tableEntries[i] < 0)
		{
			if (m_fileNames.find(entry->Id()) != m_fileNames.end())
			{
				name = PrepareOutputName(entry->Id(), m_fileNames.at(entry->Id()));
			}
			else if(m_deviceNames.find(entry->Id()) != m_deviceNames.end())
			{
				name = PrepareOutputName(entry->Id(), m_deviceNames.at(entry->Id()));
			}
			else
			{
				name = "/unknown/" +  std::to_string(entry->Id()) + ".img";
			}
		}
		else
		{
			name = "/tables/" + std::to_string(entry->Id())  + "_for_" + std::to_string(m_entries[m_tableEntries[i]].Id()) + ".img";
		}
		
		entry->NamePath = name;
	}
}

void CPUP::CreateDir(std::string outputPath)
{
	std::string outputParentPath = outputPath.substr(0, outputPath.find_last_of("/\\"));
	if (!outputParentPath.empty())
	{
		mkdir(outputParentPath.c_str(), 0775);
	}
}

void CPUP::ExtractAllEntries(std::string baseoutputpath)
{
	std::cout << "Processing PUP: " << m_inputfile.c_str() << std::endl;

	for (int i = 0; i < m_entries.size(); i++)
	{
		Entry* entry = &m_entries[i];
		
		int special = entry->Flags & 0xF0000000;
		if (special == 0xE0000000 || special == 0xF0000000)
		{
			continue;
		}
		

		std::cout << "Processing Entry: " << entry->NamePath.c_str()
#ifndef NDEBUG
		<< " -" << entry->ToString()
#endif
		<< std::endl;
		std::string outputPath = baseoutputpath+ "/" + entry->NamePath;
		ExtractSingleEntry(i, entry, outputPath);
	}
}

void CPUP::ExtractSingleEntry(int index, Entry* entry, std::string outputfilepath)
{
	CreateDir(outputfilepath);
	std::ofstream output(outputfilepath.c_str(), std::ios::binary);
	if (entry->IsBlocked() == false)
	{
		//Start reading from
		m_reader.seekg(entry->Offset);
		if (entry->IsCompressed() == false)
		{
			CBinaryReader::CopyStream(&m_reader, &output, entry->CompressedSize);
		}
		else
		{
			zstr::istreambuf zbuf(m_reader.rdbuf(), 1 << 16, false);
			std::istream zlib(&zbuf);
			CBinaryReader::CopyStream(&zlib, &output, entry->UncompressedSize);
			
		}
	}
	else
	{
		if (((entry->Id() | 0x100) & 0xF00) == 0xF00)
		{
			throw std::logic_error("InvalidOperationException");
		}
		
		int tableIndex = -1;
		for (int j = 0; j < m_entries.size(); j++)
		{
			if ((m_entries[j].Flags & 1) != 0)
			{
				if (m_entries[j].Id() == index)
				{
					tableIndex = j;
					break;
				}
			}
		}
		
		if (tableIndex < 0)
		{
			throw std::logic_error("InvalidOperationException");
		}
		
		Entry tableEntry = m_entries[tableIndex];
		
		int blockSize = 1u << (int)(((entry->Flags & 0xF000) >> 12) + 12);
		int blockCount = (blockSize + entry->UncompressedSize - 1) / blockSize;
		int tailSize = entry->UncompressedSize % blockSize;
		if (tailSize == 0)
		{
			tailSize = blockSize;
		}
		
		std::map<int, CPUP::BlockInfo> blockInfos;
		if (entry->IsCompressed())
		{
			std::stringstream tableData;
			m_reader.seekg(tableEntry.Offset);
			if (tableEntry.IsCompressed() == false)
			{
				CBinaryReader::CopyStream(&m_reader, &tableData, tableEntry.CompressedSize);
			}
			else
			{
				zstr::istreambuf zbuf(m_reader.rdbuf(), 1 << 16, false);
				std::istream zlib(&zbuf);
				CBinaryReader::CopyStream(&zlib, &tableData, tableEntry.UncompressedSize);
			}
			
			tableData.seekg(32 * blockCount);
			for (int j = 0; j < blockCount; j++)
			{
				CPUP::BlockInfo blockInfo;
				blockInfo.Offset = CBinaryReader::Read<uint32_t, std::stringstream>(&tableData);
				blockInfo.Size = CBinaryReader::Read<uint32_t, std::stringstream>(&tableData);
				blockInfos[j] = blockInfo;
			}
		}
		
		m_reader.seekg(entry->Offset);
		
		int compressedRemainingSize = entry->CompressedSize;
		int uncompressedRemainingSize = entry->UncompressedSize;
		int lastIndex = blockCount - 1;
		
		for (int i = 0; i < blockCount; i++)
		{
			long compressedReadSize, uncompressedReadSize = 0;
			bool blockIsCompressed = false;
			
			if (entry->IsCompressed() == true)
			{
				CPUP::BlockInfo blockInfo = blockInfos[i];
				int unpaddedSize = (blockInfo.Size & ~0xFu) - (blockInfo.Size & 0xFu);
				
				compressedReadSize = blockSize;
				if (unpaddedSize != blockSize)
				{
					compressedReadSize = blockInfo.Size;
					if (i != lastIndex || tailSize != blockInfo.Size)
					{
						compressedReadSize &= ~0xFu;
						blockIsCompressed = true;
					}
				}
				
				if (blockInfo.Offset != 0)
				{
					int blockOffset = entry->Offset + blockInfo.Offset;
					m_reader.seekg(blockOffset);
					output.seekp(i * blockSize);
				}
			}
			else
			{
				compressedReadSize = compressedRemainingSize;
				if (blockSize < compressedReadSize)
				{
					compressedReadSize = blockSize;
				}
			}
			
			if (blockIsCompressed)
			{
				{
					std::stringstream temp;
					CBinaryReader::CopyStream<std::ifstream, std::stringstream>(&m_reader, &temp, compressedReadSize - (blockInfos[i].Size & 0xF));
					temp.seekg(0);
					
					uncompressedReadSize = uncompressedRemainingSize;
					if (blockSize < uncompressedReadSize)
					{
						uncompressedReadSize = blockSize;
					}
					
					zstr::istreambuf zbuf(temp.rdbuf(), 1 << 16, false);
					std::istream zlib(&zbuf);
					CBinaryReader::CopyStream(&zlib, &output, uncompressedReadSize);
				}
			}
			else
			{
				CBinaryReader::CopyStream(&m_reader, &output, compressedReadSize);
			}
			
			compressedRemainingSize -= compressedReadSize;
			uncompressedRemainingSize -= uncompressedReadSize;
		}
	}
}
