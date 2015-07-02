
/*! \example dictionary_compressed_column.hpp
 * This is an example of how to implement a compression technique in our framework. One has to inherit from an abstract base class CoGaDB::CompressedColumn and implement the pure virtual methods.
 */

#pragma once

#include <core/compressed_column.hpp>
#include <vector>

namespace CoGaDB{
	

	/*
	
	This struct holds the column and the respective dictionary.
	It is also serialized for easier load and store.
	
	*/
template<typename T>
struct DictColumn {
	std::vector<T> dict;
	std::vector<unsigned int> column;

	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & dict;
		ar & column;
	}
};


template<class T>
class DictionaryCompressedColumn : public CompressedColumn<T>{
	public:

	DictionaryCompressedColumn(const std::string& name, AttributeType db_type);
	virtual ~DictionaryCompressedColumn();

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

private:
	DictColumn<T> values;
	//Searches a value in the dictionary and return the position if existing
	inline int lookup(T value) {
		for (unsigned int i = 0; i < this->values.dict.size();  i++)
			if (this->values.dict[i] == value)
				return i;
		return -1;
	}
	//counts how often a value is in the column 
	inline unsigned int count(unsigned int dictID) {
		unsigned int c = 0;
		for (unsigned int i = 0; i < this->values.column.size(); i++){
			if (dictID == this->values.column[i])
				c++;
		}
		return c;
	}

};


/***************** Start of Implementation Section ******************/

	
	template<class T>
	DictionaryCompressedColumn<T>::DictionaryCompressedColumn(const std::string& name, AttributeType db_type) : CompressedColumn<T>(name, db_type){

	}

	template<class T>
	DictionaryCompressedColumn<T>::~DictionaryCompressedColumn(){

	}


	//uses the other insert function
	template<class T>
	bool DictionaryCompressedColumn<T>::insert(const boost::any& new_value){
		if (new_value.empty())
			return false;
		if (typeid(T) == new_value.type())
			return this->insert(boost::any_cast<T>(new_value));
		
		return false;
	}


	/*
	First, we do a lookup for the new value.
	If it already exists, we just add the index to the column.
	If not the new value is added to the dictionary.
	*/

	template<class T>
	bool DictionaryCompressedColumn<T>::insert(const T& new_value){
		int index = this->lookup(boost::any_cast<T>(new_value));
		if (index == -1) {
			this->values.dict.push_back(new_value);
			this->values.column.push_back(this->values.dict.size() - 1);
		}
		else
			this->values.column.push_back(index);
		return true;
	}

	template <typename T> 
	template <typename InputIterator>
	bool DictionaryCompressedColumn<T>::insert(InputIterator first, InputIterator last){
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
	const boost::any DictionaryCompressedColumn<T>::get(TID tid){
		if (tid < this->values.column.size()) {
			return boost::any(this->values.dict[this->values.column[tid]]);
		}
		return boost::any();
	}

	template<class T>
	void DictionaryCompressedColumn<T>::print() const throw(){
		std::cout << "| Coloumn |" << std::endl;
		std::cout << "________________________" << std::endl;
		for (unsigned int i = 0; i < this->values.column.size(); i++){
			std::cout << "| " << this->values.column[i] << " |" << std::endl;
		}
		std::cout << "| Dictionary |" << std::endl;
		std::cout << "________________________" << std::endl;
		for (unsigned int i = 0; i < this->values.dict.size(); i++){
			std::cout << "| " << i << " | " << this->values.dict[i] << " |" << std::endl;
		}
	}
	template<class T>
	size_t DictionaryCompressedColumn<T>::size() const throw(){
		return this->values.column.size();
	}
	template<class T>
	const ColumnPtr DictionaryCompressedColumn<T>::copy() const{
		return ColumnPtr(new DictionaryCompressedColumn(*this));
	}

	template<class T>
	bool DictionaryCompressedColumn<T>::update(TID tid, const boost::any& obj){
		if (tid < this->values.column.size() && typeid(T) == obj.type()) {
			int index = this->lookup(boost::any_cast<T>(obj));
			if (index == -1) { //update value does not exist in dictinary
				if (this->count(this->values.column[tid]) == 1) //if old value is present just once -> swap values
					this->values.dict[this->values.column[tid]] = boost::any_cast<T>(obj);
				else {
					this->values.dict.push_back(boost::any_cast<T>(obj)); //if old value is present more then once -> create new dict entry
					this->values.column[tid] = this->values.dict.size() - 1;
				}
			}
			else {
				unsigned int old = this->values.column[tid];

				if (this->count(old) == 1) {
					this->values.column[tid] = index; //update value is in dictionary -> put dictid into column

					for (unsigned int i = 0; i < this->values.column.size(); i++) {
						if (this->values.column[i] > old)
							this->values.column[i]--;
					}
					this->values.dict.erase(this->values.dict.begin() + old);
				}
				else
					this->values.column[tid] = index; //update value is in dictionary -> put dictid into column

			}
			return true;
		}
		return false;
	}

	template<class T>
	bool DictionaryCompressedColumn<T>::update(PositionListPtr posPtr, const boost::any& obj){
		for (unsigned int i = 0; i < posPtr->size(); i++)
			if (!this->update(posPtr->at(i), obj))
				return false;

		return true;
	}
	
	/*
	If a value just appears once in the column and is replaced it is removed from the
	dictionary. If so, the DictID for all following values in the dictionary decreases by 1.
	Ist must be corrected in the column.
	*/
	template<class T>
	bool DictionaryCompressedColumn<T>::remove(TID tid){
		if (tid < this->values.column.size()) {
			if (this->count(this->values.column[tid]) == 1) {
				for (unsigned int i = 0; i < this->values.column.size(); i++) {
					if (this->values.column[i] > this->values.column[tid])
						this->values.column[i]--;
				}
				this->values.dict.erase(this->values.dict.begin() + this->values.column[tid]);
			}
			this->values.column.erase(this->values.column.begin() + tid);
			return true;
		}
		return false;	
	}
	
	template<class T>
	bool DictionaryCompressedColumn<T>::remove(PositionListPtr posPtr){
		for (unsigned int i = 0; i < posPtr->size(); i++)
			if (!this->remove(posPtr->at(i)))
				return false;

		return true;
	}

	template<class T>
	bool DictionaryCompressedColumn<T>::clearContent(){
		this->values.column.clear();
		this->values.dict.clear();
		return true;
	}

	template<class T>
	bool DictionaryCompressedColumn<T>::store(const std::string& path_){
		std::string path(path_);
		path += "/";
		path += this->name_;

		std::ofstream ofs(path.c_str(), std::ios_base::binary | std::ios_base::out);
		boost::archive::binary_oarchive oa(ofs);
		oa << this->values;

		ofs.flush();
		ofs.close();
		return true;
	}
	template<class T>
	bool DictionaryCompressedColumn<T>::load(const std::string& path_){
		std::string path(path_);
		path += "/";
		path += this->name_;

		std::ifstream ifs(path.c_str(), std::ios_base::binary | std::ios_base::in);
		boost::archive::binary_iarchive ia(ifs);
		ia >> this->values;

		ifs.close();
		return true;
	}

	template<class T>
	T& DictionaryCompressedColumn<T>::operator[](const int index){
		return this->values.dict[this->values.column[index]];
	}

	template<class T>
	unsigned int DictionaryCompressedColumn<T>::getSizeinBytes() const throw(){
		return this->values.dict.capacity()*sizeof(T)+ //Dictionary 
			this->values.column.capacity()*sizeof(unsigned int); //Column
	}

/***************** End of Implementation Section ******************/



}; //end namespace CogaDB

