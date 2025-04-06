#include <ac_int.h>
#include <assert.h> /* assert */
#include <elfFile.h>
#include <soc_config.h>
#include <vector>

void setByte(ac_int<32, false> &mem, unsigned addr, ac_int<8, true> value) {
  mem.set_slc((addr % 4) * 8, value);
}

int main(int argc, char **argv) {
  char *binaryFile = argv[1];
  std::vector<ac_int<32, false>> mem;
  mem.reserve(DRAM_SIZE);
  unsigned heapAddress = 0;
  //****************************************************************************
  // Populate memory using ELF file
  ElfFile elfFile(binaryFile);
  for (auto const &section : elfFile.sectionTable) {
    if (section.address != 0) {
      for (unsigned i = 0; i < section.size; i++) {
        unsigned addr = section.address + i;
        setByte(mem[addr >> 2], addr, elfFile.content[section.offset + i]);
      }
      // update the size of the heap
      if (section.name != ".text") {
        if (section.address + section.size > heapAddress)
          heapAddress = section.address + section.size;
      }
    }
  }
  for (int oneWord = (ROMRAM >> 2); oneWord < heapAddress; oneWord++) {
    printf("%08x %08x\n", (unsigned int)mem[oneWord], oneWord * 4);
  }
  return 1;
}
