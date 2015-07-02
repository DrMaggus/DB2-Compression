#include <string>
#include <iostream>
#include <core/global_definitions.hpp>
#include <core/base_column.hpp>
#include <core/column_base_typed.hpp>
#include <core/column.hpp>
#include <core/compressed_column.hpp>

/*this is the include for the example compressed column with empty implementation*/
#include <compression/dictionary_compressed_column.hpp>
#include <compression/RunLengthCompressionColumn.h>
#include <compression/BitVectorCompression.h>

using namespace CoGaDB;

bool unittest(boost::shared_ptr<ColumnBaseTyped<int> > ptr);
bool unittest(boost::shared_ptr<ColumnBaseTyped<float> > ptr);
bool unittest(boost::shared_ptr<ColumnBaseTyped<std::string> > ptr);

int main(){

	/*
	
	For each compression method and each data type (Varchar, int, float) 
	a column is created and tested. 
	
	*/

	std::cout << "****** Run Length Encoding ******\n\n";

	boost::shared_ptr<RunLengthCompressionColumn<std::string>> rlc_string(new RunLengthCompressionColumn<std::string>("RunLengthCompression String", VARCHAR));
	if (!unittest(rlc_string)){
		std::cout << "At least one Unittest Failed!" << std::endl;	
		return -1;	
	}
	std::cout << "Unitests Passed!\n\n" << std::endl;
	
	boost::shared_ptr<RunLengthCompressionColumn<float>> rlc_float(new RunLengthCompressionColumn<float>("RunLengthCompression Float", FLOAT));
	if (!unittest(rlc_float)){
		std::cout << "At least one Unittest Failed!" << std::endl;
		return -1;
	}
	std::cout << "Unitests Passed!\n\n" << std::endl;

	boost::shared_ptr<RunLengthCompressionColumn<int>> rlc_int(new RunLengthCompressionColumn<int>("RunLengthCompression Float", INT));
	if (!unittest(rlc_int)){
		std::cout << "At least one Unittest Failed!" << std::endl;
		return -1;
	}
	std::cout << "Unitests Passed!\n\n" << std::endl;


	std::cout << "\n****** Dictionary Encoding ******\n\n";

	boost::shared_ptr<DictionaryCompressedColumn<std::string>> dict_string(new DictionaryCompressedColumn<std::string>("Dictionary Compression String", VARCHAR));
	if (!unittest(dict_string)){
		std::cout << "At least one Unittest Failed!" << std::endl;
		return -1;
	}
	std::cout << "Unitests Passed!\n\n" << std::endl;

	boost::shared_ptr<DictionaryCompressedColumn<float>> dict_float(new DictionaryCompressedColumn<float>("Dictionary Compression Float", FLOAT));
	if (!unittest(dict_float)){
		std::cout << "At least one Unittest Failed!" << std::endl;
		return -1;
	}
	std::cout << "Unitests Passed!\n\n" << std::endl;

	boost::shared_ptr<DictionaryCompressedColumn<int>> dict_int(new DictionaryCompressedColumn<int>("Dictionary Compression Int", INT));
	if (!unittest(dict_int)){
		std::cout << "At least one Unittest Failed!" << std::endl;
		return -1;
	}
	std::cout << "Unitests Passed!\n\n" << std::endl;

	std::cout << "\n****** Bit Vector Encoding ******\n\n";
	
	boost::shared_ptr<BitVectorCompressedColumn<std::string>> vect_string(new BitVectorCompressedColumn<std::string>("Bit Vector Compression String", VARCHAR));
	if (!unittest(vect_string)){
		std::cout << "At least one Unittest Failed!" << std::endl;
		return -1;
	}
	std::cout << "Unitests Passed!\n\n" << std::endl;

	boost::shared_ptr<BitVectorCompressedColumn<float>> vect_float(new BitVectorCompressedColumn<float>("Bit Vector Compression Float", FLOAT));
	if (!unittest(vect_float)){
		std::cout << "At least one Unittest Failed!" << std::endl;
		return -1;
	}
	std::cout << "Unitests Passed!\n\n" << std::endl;
	
	boost::shared_ptr<BitVectorCompressedColumn<int>> vect_int(new BitVectorCompressedColumn<int>("Bit Vector Compression String", INT));
	if (!unittest(vect_int)){
		std::cout << "At least one Unittest Failed!" << std::endl;
		return -1;
	}
	std::cout << "Unitests Passed!\n\n" << std::endl;


	//Testing was successfull
	return 0;

}


