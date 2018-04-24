#include "DataMemory.h"
#include "stdHeader.h"

DataMemory::DataMemory(std::map<u32, u32> data_m)
{
  data = data_m;
}

DataMemory::~DataMemory() {}
DataMemory::DataMemory() {}

void DataMemory::print_out()
{
  std::cout << " ------------- " << std::endl;
  std::cout << "| Data Memory |" << std::endl;
  std::cout << " ------------- " << std::endl;
  std::cout << "Address: 0x" << std::hex << address << std::endl;
  std::cout << "Read Data: 0x" << std::hex << read_data << std::endl;
  printf("Write Data: 0x%08x\n", write_data);
  std::cout << "Control Line - MemRead: 0x" << std::hex << control_read << std::endl;
  std::cout << "Control Line - MemWrite: 0x" << std::hex << control_write << std::endl;
  std::cout << "Memory Contents..." << std::endl;
  for(auto iter = data.begin(); iter != data.end(); ++iter)
  {
    std::cout << "0x" << std::hex << iter->first << ":" << iter->second << std::endl;
  }
  std::cout << std::endl;
}

void DataMemory::execute()
{
    if(control_read == 1)
    {
        //this is a data read
        //retrieve data at this->address
        //set it to this->read_data
        read_data = data[address];

    }
    else if(control_write == 1)
    {
        //this is a data write
        //write the data at this->address with this->write_data
        data[address] = write_data;
    }
}
