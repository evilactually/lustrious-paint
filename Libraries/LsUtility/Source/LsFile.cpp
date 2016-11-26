
//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include <windows.h>

//-------------------------------------------------------------------------------
//-- Functions ------------------------------------------------------------------
//-------------------------------------------------------------------------------

std::vector<char> GetBinaryFileContents( std::string const &filename ) {
  // char str[256];
  // GetCurrentDirectory(256, &str[0]);
  // throw std::string(str);
  try {
	  std::ifstream file;
	  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	  file.open(filename, std::ios::binary);

	  if (file.fail()) {
		  std::cout << "Could not open \"" << filename << "\" file!" << std::endl;
		  return std::vector<char>();
	  }

	  std::streampos begin, end;
	  begin = file.tellg();
	  file.seekg(0, std::ios::end);
	  end = file.tellg();

	  std::vector<char> result(static_cast<size_t>(end - begin));
	  file.seekg(0, std::ios::beg);
	  file.read(&result[0], end - begin);
	  file.close();

	  return result;
  }
  catch (std::ifstream::failure e) {
	  throw std::string(strerror(errno));
	  //throw std::string(e.what());
  }
}