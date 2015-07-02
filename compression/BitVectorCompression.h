#pragma once

#include <core/compressed_column.hpp>
#include <vector>
#include <bitset>

namespace CoGaDB{


	/*
	
	To really access just one bit we created the BitVector class,
	because the std::bitset is not serializeable.
	It uses the std::vector<unsigned char>. So for each element in the
	vector we get 8 bits to set.
	If we need more, another unsigned char is added.

	The T value is the value in the column.
	
	*/
	template<class T>
	class BitVector {
	private:
		T value;
		std::vector<unsigned char> bitvector;
	public:
		BitVector() {} //just for serialization

		BitVector(T value, unsigned int size) { //Creating a vector with size in bits
			size = size % 8 == 0 ? size / 8 : size / 8 + 1;
			for (unsigned int i = 0; i < size; i++) {
				this->bitvector.push_back(0);
			}
			this->value = value;
		}
		BitVector(T value, std::string vector) { //Creating a vector from (literally) a bitstring
			for (unsigned int i = 0; i < vector.size(); i++) {
				if (i % 8 == 0)
					this->bitvector.push_back(0);
				if (vector[i]=='1')
					(this->bitvector[i / 8]) |= (1 << (i % 8));
			}
			this->value = value;
		}
		~BitVector() {  }

		//Set one position to one
		void to_one(unsigned int position) {
			(this->bitvector[position / 8]) |= (1 << (position % 8));
		}

		//Set one position to zero
		void to_zero(unsigned int position) {
			(this->bitvector[position / 8]) &= ~(1 << (position % 8));
		}

		unsigned int at(unsigned int position) const {
			if ((((this->bitvector[position / 8]) >> (position % 8)) & 1) == 1)
				return 1;
			return 0;
		}

		//check if a byte in the vector is zero
		bool isZero(unsigned int byte) {
			return this->bitvector[byte] == 0;
		}

		//check whether the entire vector is zero
		bool isZero() {
			for (unsigned int i = 0; i < this->bitvector.size(); i++) {
				if (this->bitvector[i] != 0)
					return false;
			}
			return true;
		}

		void addByte() {
			this->bitvector.push_back(0);
		}

		void removeByte(unsigned int byte) {
			this->bitvector.erase(this->bitvector.begin()+byte);
		}

		unsigned int size() const{ //size in byte
			return this->bitvector.size();
		}

		T getValue() const { return this->value; }
		T& getValueRef()  { return this->value; }

		std::string bitstring() const {
			std::string res = "";
			for (unsigned int i = 0; i < this->bitvector.size()*8; i++){
				if (this->at(i) == 1)
					res += "1";
				else
					res += "0";
			}
			return res;
		}
		
		//serialize for easier load and store
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & this->bitvector;
			ar & this->value;
		}
	};

	template<class T>
	class BitVectorCompressedColumn : public CompressedColumn<T>{
	public:
		BitVectorCompressedColumn(const std::string& name, AttributeType db_type);
		virtual ~BitVectorCompressedColumn();

		virtual bool insert(const boost::any& new_Value);
		virtual bool insert(const T& new_value);
		template <typename InputIterator>
		bool insert(InputIterator first, InputIterator last);

		virtual bool update(TID tid, const boost::any& new_value);
		virtual bool update(PositionListPtr tid, const boost::any& new_value);

		virtual bool remove(TID tid);
		virtual bool remove(PositionListPtr tid);
		virtual bool clearContent();

		virtual const boost::any get(TID tid);
		virtual void print() const throw();
		virtual size_t size() const throw();
		virtual unsigned int getSizeinBytes() const throw();

		virtual const ColumnPtr copy() const;

		virtual bool store(const std::string& path);
		virtual bool load(const std::string& path);

		virtual T& operator[](const int index);

		//serialize for easier load and store
		template<class Archive>
		void serialize(Archive & ar, const unsigned int version)
		{
			ar & this->values;
			ar & this->column_length;
			ar & this->bytes;
			ar & this->elem_num;
		}


	private:
		std::vector<BitVector<T>> values;
		unsigned int column_length;
		unsigned int bytes;
		unsigned int elem_num;

		//Search for a certain value, if found -> index is returned
		unsigned int lookup(T value) {
			for (unsigned int i = 0; i < this->values.size(); i++) {
				if (this->values[i].getValue() == value)
					return i;
			}
			return -1;
		}

		void extend_vectors() {
			for (unsigned int i = 0; i < this->values.size(); i++) {
				this->values[i].addByte();
			}
			this->bytes++;
		}

		// This function retunes T at position tid
		T getByTID(TID tid) const{
			int index = 0;
			for (unsigned int i = 0; i < this->column_length; i++) {
				for (unsigned int j = 0; j < this->values.size(); j++) {
					if (this->values[j].at(i) == 1) {
						if (index == tid)
							return this->values[j].getValue();
					
						index++;
						break;
					}
				}
			}
			return T();
		}

		//Calculates the position in the vector by tid
		// tid != pos. in vector because we cannot remove a single bit from a byte
		int bitPosByTID(TID tid) {
			unsigned int index = 0;
			if (tid < this->elem_num) {
				for (unsigned int i = 0; i < this->column_length; i++) {
					for (unsigned int j = 0; j < this->values.size(); j++) {
						if (this->values[j].at(i) == 1) {
							if (index == tid)
								return i;

							index++;
							break;
						}
					}
				}
			}
			return -1;
		}

		//If a block (byte) is clear (zero) in all bitvectors of all values
		//we can simply remove it, because it holds no information.
		//The byte is the smallest data structure we can remove.
		void clearZeroBytes() {
			bool zero;
			for (unsigned int i = 0; i < this->bytes; i++) {
				zero = true;
				for (unsigned int j = 0; j < this->values.size(); j++) {
					if (this->values[j].isZero(i) == false)
						zero = false;
				}
				if (zero) {
					for (unsigned int k = 0; k < this->values.size(); k++)
						this->values[k].removeByte(i);
					this->column_length -= 8;
					this->bytes--;
				}
			}
		}
	};

	template<class T>
	BitVectorCompressedColumn<T>::BitVectorCompressedColumn(const std::string& name, AttributeType db_type) : CompressedColumn<T>(name, db_type){
		column_length = 0;
		bytes = 0;
		elem_num = 0;
	}

	template<class T>
	BitVectorCompressedColumn<T>::~BitVectorCompressedColumn(){

	}

	//uses other insert function
	template<class T>
	bool BitVectorCompressedColumn<T>::insert(const boost::any& new_value){
		if (new_value.empty())
			return false;
		if (typeid(T) == new_value.type())
			return this->insert(boost::any_cast<T>(new_value));

		return false;
	}

	template<class T>
	bool BitVectorCompressedColumn<T>::insert(const T& new_value){
		int index = this->lookup(new_value); //Search for new value

		if (this->column_length / 8 >= bytes) //vector is full ?
			this->extend_vectors();

		if (index != -1) // New value exists 
			this->values[index].to_one(this->column_length);
		else {
			this->values.push_back(BitVector<T>(new_value, this->bytes*8)); //add new value if it exists not
			this->values.back().to_one(this->column_length);
		}

		this->column_length++;
		this->elem_num++;

		return true;
	}

	template <typename T>
	template <typename InputIterator>
	bool BitVectorCompressedColumn<T>::insert(InputIterator first, InputIterator last){
		if (first < last) {
			for (InputIterator it = first; it != last; it++) {
				if (!this->insert(*it))
					return false;
			}
			return true;
		}
		return false;
	}

	template<class T>
	const boost::any BitVectorCompressedColumn<T>::get(TID tid){
		return boost::any(this->getByTID(tid));
	}

	template<class T>
	void BitVectorCompressedColumn<T>::print() const throw(){
		std::cout << "   Bit Vectors\n" << "_________________________\n";
		for (unsigned int i = 0; i < this->values.size(); i++) {
			std::cout << this->values[i].getValue() << "\t" << this->values[i].bitstring() << std::endl;
		}
		std::cout << "\n   Column\n" << "_________________________\n";
		for (unsigned int i = 0; i < this->elem_num; i++) {
			std::cout << i << " | " << this->getByTID(i) << std::endl;
		}
	}

	template<class T>
	size_t BitVectorCompressedColumn<T>::size() const throw(){
		return this->elem_num;
	}

	template<class T>
	const ColumnPtr BitVectorCompressedColumn<T>::copy() const{
		return ColumnPtr(new BitVectorCompressedColumn(*this));
	}

	template<class T>
	bool BitVectorCompressedColumn<T>::update(TID tid, const boost::any& obj){
		if (!obj.empty() && typeid(T) == obj.type() && tid < this->elem_num) {

			if (this->lookup(boost::any_cast<T>(obj)) == -1) //if new value does not exist -> add it
				this->values.push_back(BitVector<T>(boost::any_cast<T>(obj), this->bytes * 8));


			/*
			First we calculate the bitposition and the respective position
			for the new and the old value.
			After that we just set the value at 'bitpos' in the bitvector.

			*/

			int new_value_pos = this->lookup(boost::any_cast<T>(obj));
			unsigned int bitpos = this->bitPosByTID(tid);
			unsigned int old_value_pos = this->lookup(this->getByTID(tid));
				
			this->values[old_value_pos].to_zero(bitpos);
			this->values[new_value_pos].to_one(bitpos);

			//If the old values bit vector is zero->it doesn't appear in the
			//column.So we can delete it.

			if (this->values[old_value_pos].isZero())
				this->values.erase(this->values.begin() + old_value_pos);

			this->clearZeroBytes(); // check "empty" parts in the vectors

			return true;
		}
		return false;
	}

	template<class T>
	bool BitVectorCompressedColumn<T>::update(PositionListPtr posPtr, const boost::any& obj){
		for (unsigned int i = 0; i < posPtr->size(); i++)
		if (!this->update(posPtr->at(i), obj))
			return false;

		return true;
	}

	template<class T>
	bool BitVectorCompressedColumn<T>::remove(TID tid){
		if (tid < this->elem_num) {
			unsigned int value_pos = this->lookup(this->getByTID(tid)); //Where is the position of the value to be deleted?

			this->values[value_pos].to_zero(this->bitPosByTID(tid)); //delete value by setting the bit to 0
			this->elem_num--;

			if (this->values[value_pos].isZero()) //Delete value when it never appears in the column
				this->values.erase(this->values.begin() + value_pos);

			this->clearZeroBytes();// check "empty" parts in the vectors

			return true;
		}
		return false;
	}

	template<class T>
	bool BitVectorCompressedColumn<T>::remove(PositionListPtr posPtr){
		for (unsigned int i = 0; i < posPtr->size(); i++)
		if (!this->remove(posPtr->at(i)))
			return false;

		return true;
	}



	template<class T>
	bool BitVectorCompressedColumn<T>::clearContent(){
		this->values.clear();
		this->bytes = 0;
		this->column_length = 0;
		this->elem_num = 0;
		return true;
	}

	template<class T>
	bool BitVectorCompressedColumn<T>::store(const std::string& path_){
		std::string path(path_);
		path += "/";
		path += this->name_;

		std::ofstream ofs(path.c_str(), std::ios_base::binary | std::ios_base::out);
		boost::archive::binary_oarchive oa(ofs);
		oa << (*this);

		ofs.flush();
		ofs.close();
		return true;
	}
	template<class T>
	bool BitVectorCompressedColumn<T>::load(const std::string& path_){
		std::string path(path_);
		path += "/";
		path += this->name_;

		std::ifstream ifs(path.c_str(), std::ios_base::binary | std::ios_base::in);
		boost::archive::binary_iarchive ia(ifs);
		ia >> (*this);

		ifs.close();
		return true;
	}

	template<class T>
	T& BitVectorCompressedColumn<T>::operator[](const int index){
		return this->values[this->lookup(this->getByTID(index))].getValueRef();
	}

	template<class T>
	unsigned int BitVectorCompressedColumn<T>::getSizeinBytes() const throw(){
		return this->values.capacity()*sizeof(T)+ //size of different values
			this->bytes*this->values.size(); //size of the bitvectors
	}




}