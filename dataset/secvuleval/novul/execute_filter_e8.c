execute_filter_e8(struct rar_filter *filter, struct rar_virtual_machine *vm, size_t pos, int e9also)
{
  uint32_t length = filter->initialregisters[4];
  uint32_t filesize = 0x1000000;
  uint32_t i;

  if (length > PROGRAM_WORK_SIZE || length <= 4)
    return 0;

  for (i = 0; i <= length - 5; i++)
  {
    if (vm->memory[i] == 0xE8 || (e9also && vm->memory[i] == 0xE9))
    {
      uint32_t currpos = (uint32_t)pos + i + 1;
      int32_t address = (int32_t)vm_read_32(vm, i + 1);
      if (address < 0 && currpos >= (uint32_t)-address)
        vm_write_32(vm, i + 1, address + filesize);
      else if (address >= 0 && (uint32_t)address < filesize)
        vm_write_32(vm, i + 1, address - currpos);
      i += 4;
    }
  }

  filter->filteredblockaddress = 0;
  filter->filteredblocklength = length;

  return 1;
}